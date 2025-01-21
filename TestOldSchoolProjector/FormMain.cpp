//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <FMX.Platform.Win.hpp>

#include <memory>
#include <filesystem>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <limits>
#include <cstring>

#include "FormMain.h"

using std::make_unique;
using std::filesystem::directory_iterator;
using std::filesystem::is_directory;
using std::memcmp;
using std::clamp;
using std::numeric_limits;
using std::memcpy;

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
TForm1 *Form1;
//---------------------------------------------------------------------------

__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
	Application->OnIdle = &IdleEvent;
	std::wstring Path = LR"=(C:\Users\Giuliano\Desktop\SlideShow)=";
	//std::wstring Path = LR"=(.)=";
	if ( is_directory( Path ) ) {
		for ( decltype( auto ) Entry : directory_iterator( Path ) ) {
			if ( Entry.path().extension() == L".jpg" ) {
				entries_.emplace_back( Entry.path().c_str() );
			}
		}
	}
	if ( !entries_.empty() ) {
		LoadImage( idx_ );
	}
	BuildSuono( GetVolumeSuono() );
}
//---------------------------------------------------------------------------

void __fastcall TForm1::IdleEvent( TObject* Sender, bool &Done )
{
    tbarVolume->Enabled = !IsImageChanging();
    Done = true;
}
//---------------------------------------------------------------------------

float TForm1::GetVolumeSuono() const
{
    return static_cast<float>( Volume ) / 100.0F;
}
//---------------------------------------------------------------------------

int TForm1::GetVolume() const
{
    return volume_;
}
//---------------------------------------------------------------------------

void TForm1::SetVolume( int Value )
{
    BuildSuono( GetVolumeSuono() );
    auto Vol = clamp( Value, 0, 100 );
    if ( volume_ != Vol ) {
        volume_ = Vol;
        tbarVolume->Value = Vol;
        BuildSuono( GetVolumeSuono() );
    }
}
//---------------------------------------------------------------------------

static constexpr LPCTSTR Suono = _D( "suono" );

void TForm1::BuildSuono( float Volume )
{
    ReadWaveFromResource( Suono, Volume );
}
//---------------------------------------------------------------------------

struct WAVHEADER {
    char riff[4];               // "RIFF"
    uint32_t fileSize;          // Dimensione del file meno 8 byte
    char wave[4];               // "WAVE"
    char fmtChunkMarker[4];     // "fmt "
    uint32_t fmtChunkSize;      // Dimensione del fmt chunk
    uint16_t audioFormat;       // 1 = PCM
    uint16_t numChannels;       // Numero di canali
    uint32_t sampleRate;        // Frequenza di campionamento
    uint32_t byteRate;          // Byte per secondo
    uint16_t blockAlign;        // Byte per campione (numChannels * bitsPerSample/8)
    uint16_t bitsPerSample;     // Bit per campione
    char dataChunkMarker[4];    // "data"
    uint32_t dataChunkSize;     // Dimensione del data chunk
};
//---------------------------------------------------------------------------

static bool ValidateWavHeader( WAVHEADER const & header )
{
    return memcmp( header.riff, "RIFF", 4 ) == 0 &&
           memcmp( header.wave, "WAVE", 4 ) == 0 &&
           memcmp( header.fmtChunkMarker, "fmt ", 4 ) == 0 &&
           memcmp( header.dataChunkMarker, "data", 4 ) == 0 &&
           header.audioFormat == 1 &&   // PCM non compresso
           header.numChannels == 2 &&   // Stereo
           header.bitsPerSample == 16;  // 16 bit
}
//---------------------------------------------------------------------------

static void AdjustVolume( int16_t* Samples, size_t SampleCount, float Volume )
{
    for ( size_t Idx {} ; Idx < SampleCount ; ++Idx ) {
        Samples[Idx] =
            std::clamp(
                static_cast<int32_t>(Samples[Idx] * Volume ),
                static_cast<int32_t>( numeric_limits<int16_t>::min() ),
                static_cast<int32_t>( numeric_limits<int16_t>::max() )
            );
    }
}
//---------------------------------------------------------------------------

void TForm1::ReadWaveFromResource( String Name, float Volume, String Type )
{
    {
        auto SoundRes =
            make_unique<TResourceStream>(
                reinterpret_cast<THandle>( HInstance ), Name.c_str(), Type.c_str()
            );
        //SoundRes->SaveToFile( _D( "sound.wav" ) );
        sound_.resize( SoundRes->Size );
        memcpy( sound_.data(), SoundRes->Memory, SoundRes->Size );
    }
    auto WaveData = static_cast<int8_t*>( sound_.data() );
    auto WaveHeader = reinterpret_cast<WAVHEADER const *>( WaveData );
    if ( !ValidateWavHeader( *WaveHeader ) ) {
        throw Exception(
            _D( "Resource \"%s\" has wrong format" ), ARRAYOFCONST(( Name ))
        );
    }
    auto Samples = reinterpret_cast<int16_t*>( WaveData + sizeof *WaveHeader );
    auto SampleCount = WaveHeader->dataChunkSize / sizeof( int16_t );
    AdjustVolume( Samples, SampleCount, Volume );
}
//---------------------------------------------------------------------------

static void PlayResource( String Name )
{
    HRSRC ResInfo;
    HANDLE HRes;

    // Find the WAVE resource.
    if ( ResInfo = ::FindResource( HInstance, Name.c_str(), _D( "WAVE" ) ); ResInfo == nullptr ) {
        throw Exception( _D( "WAVE Resource \"%s\" not found" ), ARRAYOFCONST(( Name )) );
    }

    // Load the WAVE resource.
    if ( HRes = ::LoadResource( HInstance, ResInfo ); HRes == nullptr ) {
        throw Exception( _D( "Can't load WAVE Resource \"%s\"" ), ARRAYOFCONST(( Name )) );
    }

    // Lock the WAVE resource and play it.
    // https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-lockresource
    // LockResource does not actually lock memory; it is just used to obtain
    // a pointer to the memory containing the resource data.
    // The name of the function comes from versions prior to Windows XP,
    // when it was used to lock a global memory block allocated by LoadResource.
    if ( auto Res = ::LockResource( HRes ); Res != nullptr ) {
        ::sndPlaySound( static_cast<LPCTSTR>( Res ), SND_MEMORY | SND_ASYNC | SND_NODEFAULT );
        //UnlockResource( HRes );
    }
    else {
        throw Exception( _D( "Can't lock WAVE Resource \"%s\"" ), ARRAYOFCONST(( Name )) );
    }

    // Free the WAVE resource and return success or failure.

    ::FreeResource( HRes );
}
//---------------------------------------------------------------------------

static void Play( std::vector<int8_t> const & Data )
{
    ::sndPlaySound(
        reinterpret_cast<LPCTSTR>( Data.data() ),
        SND_MEMORY | SND_ASYNC | SND_NODEFAULT
    );
}
//---------------------------------------------------------------------------

void TForm1::LoadImage( size_t Index )
{
    ImageViewer2->Bitmap->LoadFromFile( entries_[Index] );
}
//---------------------------------------------------------------------------



void __fastcall TForm1::FloatAnimation2Finish(TObject *Sender)
{
    FloatAnimation2->Inverse = !FloatAnimation2->Inverse;
    ++phase_;
    if ( ++FloatAnimation2->Tag & 1 ) {
        idx_ = ( idx_ + 1 ) % entries_.size();
        LoadImage( idx_ );
        FloatAnimation2->Start();
        FloatAnimation1->Start();
    }
}
//---------------------------------------------------------------------------void __fastcall TForm1::actCloseExecute(TObject *Sender)

void TForm1::PlaySuono() const
{
    Play( sound_ );
    //PlayResource( Suono );
}
//---------------------------------------------------------------------------void __fastcall TForm1::actCloseExecute(TObject *Sender)

void __fastcall TForm1::actNextExecute(TObject *Sender)
{
    PlaySuono();
    ++phase_;
    FloatAnimation2->StopValue = ImageViewer2->Width * 2;
    FloatAnimation2->Start();
    FloatAnimation1->Start();
}
//---------------------------------------------------------------------------

bool TForm1::IsImageChanging() const
{
    return phase_ % 3;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::EnabledIfThereArePictures(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = !entries_.empty() && !IsImageChanging();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::tbarVolumeChange(TObject *Sender)
{
    Volume = tbarVolume->Value;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
    Caption = phase_;
}
//---------------------------------------------------------------------------

