//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <Vcl.Graphics.hpp>       // TWICImage (used only for its WIC factory)
#include <Winapi.Wincodec.hpp>
#include <System.Win.ComObj.hpp>  // OleCheck
#include <objbase.h>
#include <propidl.h>              // PROPVARIANT / PropVariantClear

#include <algorithm>
#include <cwctype>

#include "ThumbnailMaker.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

using std::max;
using std::min;

//---------------------------------------------------------------------------
namespace {

// EXIF orientation (1..8) -> WIC flip/rotate. WIC rotations are clockwise.
// Orientations 5 and 7 (transpose/transverse) are extremely rare; the mapping
// is best-effort. 1,2,3,4,6,8 are the ones seen in practice.
WICBitmapTransformOptions ExifToTransform( unsigned Orientation )
{
    switch ( Orientation ) {
        case 2:  return WICBitmapTransformFlipHorizontal;
        case 3:  return WICBitmapTransformRotate180;
        case 4:  return WICBitmapTransformFlipVertical;
        case 5:  return static_cast<WICBitmapTransformOptions>(
                     WICBitmapTransformRotate90 | WICBitmapTransformFlipHorizontal );
        case 6:  return WICBitmapTransformRotate90;
        case 7:  return static_cast<WICBitmapTransformOptions>(
                     WICBitmapTransformRotate270 | WICBitmapTransformFlipHorizontal );
        case 8:  return WICBitmapTransformRotate270;
        default: return WICBitmapTransformRotate0;   // 1 or unknown
    }
}
//---------------------------------------------------------------------------

WICBitmapTransformOptions ReadOrientation( _di_IWICBitmapFrameDecode Frame )
{
    _di_IWICMetadataQueryReader Reader;
    if ( FAILED( Frame->GetMetadataQueryReader( &Reader ) ) || !Reader ) {
        return WICBitmapTransformRotate0;   // e.g. PNG/BMP: no metadata
    }

    // The orientation tag (274) sits under different roots per container.
    static wchar_t const* const Paths[] = {
        L"/app1/ifd/{ushort=274}",   // JPEG
        L"/ifd/{ushort=274}"         // TIFF and TIFF-based RAW
    };

    for ( auto const* Path : Paths ) {
        PROPVARIANT Value;
        PropVariantInit( &Value );
        HRESULT const Hr = Reader->GetMetadataByName( Path, &Value );
        unsigned Orientation = 0;
        if ( SUCCEEDED( Hr ) && Value.vt == VT_UI2 ) {
            Orientation = Value.uiVal;
        }
        PropVariantClear( &Value );
        if ( Orientation ) {
            return ExifToTransform( Orientation );
        }
    }
    return WICBitmapTransformRotate0;
}
//---------------------------------------------------------------------------

bool RotationSwapsSides( WICBitmapTransformOptions Transform )
{
    unsigned const Rot = Transform & 0x03;   // low bits hold the rotation
    return Rot == WICBitmapTransformRotate90 || Rot == WICBitmapTransformRotate270;
}
//---------------------------------------------------------------------------

// RAII deleter for raw COM out-params, so a unique_ptr Releases them on every
// exit path (no try/__finally). Works for any IUnknown-derived interface.
struct ComReleaser {
    template <class T> void operator()( T* P ) const { if ( P ) { P->Release(); } }
};
template <class T>
using ComPtr = std::unique_ptr<T, ComReleaser>;

} // namespace
//---------------------------------------------------------------------------

std::unique_ptr<Fmx::Graphics::TBitmap>
LoadImageOriented( System::UnicodeString FileName, int MaxW, int MaxH )
{
    // An empty TWICImage is just a convenient handle to the WIC factory.
    std::unique_ptr<Vcl::Graphics::TWICImage> Probe(
        new Vcl::Graphics::TWICImage()
    );
    _di_IWICImagingFactory Factory = Probe->ImagingFactory;

    _di_IWICBitmapDecoder Decoder;
    OleCheck(
        Factory->CreateDecoderFromFilename(
            FileName.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnDemand, &Decoder
        )
    );
    _di_IWICBitmapFrameDecode Frame;
    OleCheck( Decoder->GetFrame( 0, &Frame ) );

    UINT SrcW = 0;
    UINT SrcH = 0;
    OleCheck( Frame->GetSize( &SrcW, &SrcH ) );
    if ( SrcW == 0 || SrcH == 0 ) {
        return {};
    }

    WICBitmapTransformOptions const Transform = ReadOrientation( Frame );

    // Convert to premultiplied BGRA first, so the scaler/rotator work on a
    // known format and the FMX bitmap can copy the pixels directly.
    _di_IWICFormatConverter Converter;
    OleCheck( Factory->CreateFormatConverter( &Converter ) );
    OleCheck(
        Converter->Initialize(
            Frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
            nullptr, 0.0, WICBitmapPaletteTypeCustom
        )
    );

    _di_IWICBitmapSource Source = Converter;

    // Optional downscale (thumbnails). The fit is computed on the *oriented*
    // size, but we scale the pre-rotation image by the same factor.
    if ( MaxW > 0 && MaxH > 0 ) {
        bool const Swap = RotationSwapsSides( Transform );
        int const OutW = Swap ? SrcH : SrcW;
        int const OutH = Swap ? SrcW : SrcH;
        double const Scale =
            min( 1.0, min( static_cast<double>( MaxW ) / OutW,
                           static_cast<double>( MaxH ) / OutH ) );
        if ( Scale < 1.0 ) {
            int const DstW = max( 1, static_cast<int>( SrcW * Scale ) );
            int const DstH = max( 1, static_cast<int>( SrcH * Scale ) );
            _di_IWICBitmapScaler Scaler;
            OleCheck( Factory->CreateBitmapScaler( &Scaler ) );
            OleCheck(
                Scaler->Initialize(
                    Source, DstW, DstH, WICBitmapInterpolationModeFant
                )
            );
            Source = Scaler;
        }
    }

    // Auto-rotate according to the EXIF orientation.
    if ( Transform != WICBitmapTransformRotate0 ) {
        _di_IWICBitmapFlipRotator Rotator;
        OleCheck( Factory->CreateBitmapFlipRotator( &Rotator ) );
        OleCheck( Rotator->Initialize( Source, Transform ) );
        Source = Rotator;
    }

    UINT DstW = 0;
    UINT DstH = 0;
    OleCheck( Source->GetSize( &DstW, &DstH ) );

    auto Bmp = std::make_unique<Fmx::Graphics::TBitmap>(
        static_cast<int>( DstW ), static_cast<int>( DstH )
    );
    Fmx::Graphics::TBitmapData Data;
    if ( Bmp->Map( TMapAccess::Write, Data ) ) {
        // RAII: Unmap on every exit path, exceptions included.
        struct TMapGuard {
            Fmx::Graphics::TBitmap& Bmp;
            Fmx::Graphics::TBitmapData& Data;
            ~TMapGuard() { Bmp.Unmap( Data ); }
        } MapGuard { *Bmp, Data };

        WICRect Rect { 0, 0, static_cast<INT>( DstW ), static_cast<INT>( DstH ) };
        OleCheck(
            Source->CopyPixels(
                &Rect, Data.Pitch, Data.Pitch * DstH,
                static_cast<BYTE*>( Data.Data )
            )
        );
    }
    return Bmp;
}
//---------------------------------------------------------------------------

std::set<std::wstring> GetWICSupportedExtensions()
{
    std::set<std::wstring> Result;

    // An empty TWICImage is just a convenient handle to the WIC factory.
    std::unique_ptr<Vcl::Graphics::TWICImage> Probe(
        new Vcl::Graphics::TWICImage()
    );
    _di_IWICImagingFactory Factory = Probe->ImagingFactory;
    if ( !Factory ) {
        return Result;
    }

    // Walk every installed decoder and collect the file extensions it claims.
    IEnumUnknown* RawEnum = nullptr;
    if ( FAILED( Factory->CreateComponentEnumerator(
             WICDecoder, WICComponentEnumerateDefault, &RawEnum ) ) || !RawEnum ) {
        return Result;
    }
    ComPtr<IEnumUnknown> Enum( RawEnum );

    IUnknown* RawElement = nullptr;
    ULONG Fetched = 0;
    while ( Enum->Next( 1, &RawElement, &Fetched ) == S_OK && Fetched == 1 ) {
        ComPtr<IUnknown> Element( RawElement );
        RawElement = nullptr;

        IWICBitmapCodecInfo* RawInfo = nullptr;
        if ( FAILED( Element->QueryInterface(
                 __uuidof( IWICBitmapCodecInfo ), reinterpret_cast<void**>( &RawInfo ) ) )
             || !RawInfo ) {
            continue;
        }
        ComPtr<IWICBitmapCodecInfo> Info( RawInfo );

        // Two-call idiom: ask for the required length, then fetch. The list is a
        // comma-separated string such as ".jpg,.jpeg,.jpe,.jfif".
        UINT Needed = 0;
        if ( FAILED( Info->GetFileExtensions( 0, nullptr, &Needed ) ) || Needed == 0 ) {
            continue;
        }
        std::wstring Buf( Needed, L'\0' );
        UINT Actual = 0;
        if ( FAILED( Info->GetFileExtensions( Needed, &Buf[0], &Actual ) ) ) {
            continue;
        }
        Buf.resize( Actual > 0 ? Actual - 1 : 0 );   // drop the trailing NUL

        std::wstring Ext;
        auto Flush = [&Result, &Ext]() {
            if ( !Ext.empty() ) {
                for ( wchar_t& C : Ext ) { C = static_cast<wchar_t>( towlower( C ) ); }
                Result.insert( Ext );
                Ext.clear();
            }
        };
        for ( wchar_t const C : Buf ) {
            if ( C == L',' ) { Flush(); } else { Ext.push_back( C ); }
        }
        Flush();
    }

    return Result;
}
//---------------------------------------------------------------------------
