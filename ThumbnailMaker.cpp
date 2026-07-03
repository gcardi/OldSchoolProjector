//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <Vcl.Graphics.hpp>
#include <Winapi.Wincodec.hpp>
#include <System.Win.ComObj.hpp>

#include <algorithm>

#include "ThumbnailMaker.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

using std::max;
using std::min;

//---------------------------------------------------------------------------

std::unique_ptr<Fmx::Graphics::TBitmap>
MakeThumbnail( System::UnicodeString FileName, int MaxW, int MaxH )
{
    std::unique_ptr<Vcl::Graphics::TWICImage> Wic(
        new Vcl::Graphics::TWICImage()
    );
    Wic->LoadFromFile( FileName );

    int const SrcW = Wic->Width;
    int const SrcH = Wic->Height;
    if ( SrcW <= 0 || SrcH <= 0 ) {
        return {};
    }

    // Fit within the requested box, preserving aspect, never upscaling.
    double const Scale =
        min( 1.0, min( static_cast<double>( MaxW ) / SrcW,
                       static_cast<double>( MaxH ) / SrcH ) );
    int const DstW = max( 1, static_cast<int>( SrcW * Scale ) );
    int const DstH = max( 1, static_cast<int>( SrcH * Scale ) );

    _di_IWICImagingFactory Factory = Wic->ImagingFactory;

    // High-quality downscale...
    _di_IWICBitmapScaler Scaler;
    OleCheck( Factory->CreateBitmapScaler( &Scaler ) );
    OleCheck(
        Scaler->Initialize(
            Wic->Handle, DstW, DstH, WICBitmapInterpolationModeFant
        )
    );

    // ...then convert to premultiplied BGRA, the layout an FMX TBitmap expects
    // on Windows (this also forces opaque images to alpha = 255).
    _di_IWICFormatConverter Converter;
    OleCheck( Factory->CreateFormatConverter( &Converter ) );
    OleCheck(
        Converter->Initialize(
            Scaler, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
            nullptr, 0.0, WICBitmapPaletteTypeCustom
        )
    );

    auto Bmp = std::make_unique<Fmx::Graphics::TBitmap>( DstW, DstH );
    Fmx::Graphics::TBitmapData Data;
    if ( Bmp->Map( TMapAccess::Write, Data ) ) {
        // RAII: Unmap on every exit path, exceptions included.
        struct TMapGuard {
            Fmx::Graphics::TBitmap& Bmp;
            Fmx::Graphics::TBitmapData& Data;
            ~TMapGuard() { Bmp.Unmap( Data ); }
        } MapGuard { *Bmp, Data };

        WICRect Rect { 0, 0, DstW, DstH };
        OleCheck(
            Converter->CopyPixels(
                &Rect, Data.Pitch, Data.Pitch * DstH,
                static_cast<BYTE*>( Data.Data )
            )
        );
    }
    return Bmp;
}
//---------------------------------------------------------------------------
