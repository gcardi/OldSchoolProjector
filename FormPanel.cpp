//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <FMX.Platform.Win.hpp>

//#include <algorithm>
//#include <memory>
//#include <filesystem>
//#include <cassert>
//#include <iterator>

#include "FormPanel.h"
#include "DataModStyleRes.h"

using Fmx::Platform::Win::WindowHandleToPlatform;

using std::make_unique;
//using std::swap;
//using std::filesystem::directory_iterator;
//using std::filesystem::recursive_directory_iterator;
//#include <filesystem>
//using std::filesystem::is_directory;
//using std::transform;
//using std::begin;
//using std::end;
//using std::towlower;

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "FMXFormPanelBase"
#pragma resource "*.fmx"
//TfrmPanel *frmPanel;
//---------------------------------------------------------------------------

__fastcall TfrmPanel::TfrmPanel( TComponent* Owner,
                                 int MechSoundVolume,
                                 bool FanNoise,
                                 int NoiseSoundVol,
//                                 String PicturesPath,
                                 bool RecursivePicturesSearch,
                                 FMXWinDisplayDev const * Display,
                                 StoreOpts StoreOptions,
                                 Anafestica::TConfigNode* const RootNode )
    : TfrmPanelBase( Owner, Display, StoreOptions, RootNode )
    , mechSoundVolume_{ MechSoundVolume }
    , fanNoise_{ FanNoise }
    , noiseSoundVolume_{ NoiseSoundVol }
    , player_ { new WavePlayer{ WindowHandleToPlatform( Handle )->Wnd } }
    , playerNoise_ { new WavePlayer{ WindowHandleToPlatform( Handle )->Wnd } }
//    , picturesPath_{ PicturesPath }
//    , recursivePicturesSearch_{ RecursivePicturesSearch }
{
    mechSoundVolume_ = MechSoundVolume;
    noiseSoundVolume_ = NoiseSoundVolume;
    RestoreProperties();
    LoadMechanicalSound();
//    LoadPictures();

    // Black-screen overlay: a solid black rectangle filling the whole projector
    // surface, hidden until the "black screen" command turns it on. Owned by the
    // form; HitTest off so it never eats input; kept above every other layer.
    blackout_ = new TRectangle( this );
    blackout_->Parent = layoutMain;
    blackout_->Align = TAlignLayout::Client;
    blackout_->HitTest = false;
    blackout_->Stroke->Kind = TBrushKind::None;
    blackout_->Fill->Kind = TBrushKind::Solid;
    blackout_->Fill->Color = claBlack;
    blackout_->Visible = false;
    blackout_->BringToFront();
}
//---------------------------------------------------------------------------

__fastcall TfrmPanel::~TfrmPanel()
{
    try {
        SaveProperties();
    }
    catch ( ... ) {
    }
}
//---------------------------------------------------------------------------

void TfrmPanel::RestoreProperties()
{
    // Put code here to restore attribute(s)
}
//---------------------------------------------------------------------------

void TfrmPanel::ApplyCanvasSize( float W, float H )
{
    inherited::ApplyCanvasSize( W, H );

    // Layout1 is the sliding "film frame"; ImageViewer2 shows the picture and
    // letterboxes it against its black background. Rectangle1 (vignette flash)
    // and Layout2 (vignette overlay) are Align=Client and follow on their own.
    Layout1->Size->Width = W;
    Layout1->Size->Height = H;
    ImageViewer2->Size->Width = W;
    ImageViewer2->Size->Height = H;
}
//---------------------------------------------------------------------------

void TfrmPanel::SaveProperties() const
{
    // Put code here to save attribute(s)
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanel::FloatAnimation2Finish(TObject *Sender)
{
    FloatAnimation2->Inverse = !FloatAnimation2->Inverse;
    ++phase_;
    if ( ++FloatAnimation2->Tag & 1 ) {
        // ONLOADIMAGE
        if ( onLoadPicture_ ) {
            onLoadPicture_( this, backward_ );
        }
        FloatAnimation2->Start();
        FloatAnimation1->Start();
    }
}
//---------------------------------------------------------------------------

/*
void TfrmPanel::LoadPictures()
{
	//std::wstring Path = LR"=(C:\Users\Giuliano\Desktop\SlideShow)=";
	//std::wstring Path = LR"=(C:\Users\Giuliano\Desktop\Lesso\SLIDE)=";
    std::wstring Path = picturesPath_.c_str();
	if ( is_directory( Path ) ) {
        auto Inserter = [this]( auto const & Entry )
        {
            auto Path = Entry.path();
            String Ext = String{ Path.extension().c_str() };
			if ( SameText( Ext, _D( ".jpg" ) ) || SameText( Ext, _D( ".png" ) ) ) {
				entries_.emplace_back( Path.c_str() );
			}
		};
        if ( recursivePicturesSearch_ ) {
            for ( decltype( auto ) Entry : recursive_directory_iterator( Path ) ) {
                Inserter( Entry );
            }
        }
        else {
    		for ( decltype( auto ) Entry : directory_iterator( Path ) ) {
                Inserter( Entry );
            }
        }
	}
	if ( !entries_.empty() ) {
		LoadImage( idx_ );
	}
}
*/
//---------------------------------------------------------------------------

void TfrmPanel::SetNoiseSoundVolume( int Val )
{
    if ( noiseSoundVolume_ != Val ) {
        auto PlayerNoise = make_unique<WavePlayer>( WindowHandleToPlatform( Handle )->Wnd );
        noiseSoundVolume_ = Val;
        LoadNoiseSound( *PlayerNoise );
        if ( fanNoise_ ) {
            PlayerNoise->Play( true );
        }
        else {
            PlayerNoise->Stop();
        }
        playerNoise_ = std::move( PlayerNoise );
    }
}
//---------------------------------------------------------------------------

/*
void TfrmPanel::LoadImage( size_t Index )
{
    auto FileName = entries_[Index];
    auto Bmp = make_unique<TBitmap>( FileName );
    ImageViewer2->Bitmap->Clear( {} );
    auto WFactor = static_cast<float>( ImageViewer2->Width ) / Bmp->Width;
    auto HFactor = static_cast<float>( ImageViewer2->Height ) / Bmp->Height;
    if ( HFactor <= WFactor ) {
        // H
        ImageViewer2->BitmapScale = HFactor;
    }
    else {
        // W
        ImageViewer2->BitmapScale = WFactor;
    }
    ImageViewer2->Bitmap->Assign( Bmp.get() );
}
*/
void TfrmPanel::LoadPicture( TBitmap& Bmp )
{
    // "Contain" fit: show the whole picture, scaled to fit the canvas with its
    // aspect preserved and centered on black bands, so portrait photos are
    // shown in full instead of being cropped.
    float const CanvasW = ImageViewer2->Width;
    float const CanvasH = ImageViewer2->Height;
    float const CanvasAR = CanvasW / CanvasH;
    float const SrcAR = static_cast<float>( Bmp.Width ) / Bmp.Height;

    // Build a canvas-aspect bitmap large enough to hold the picture, then cap it
    // to a safe max size: a wide canvas plus a tall photo can push a full-
    // resolution letterbox past the texture limit ("Bitmap size too big").
    int OutW, OutH;
    if ( SrcAR > CanvasAR ) {           // relatively wider -> bands top/bottom
        OutW = Bmp.Width;
        OutH = static_cast<int>( Bmp.Width / CanvasAR + 0.5f );
    }
    else {                              // relatively taller -> bands left/right
        OutH = Bmp.Height;
        OutW = static_cast<int>( Bmp.Height * CanvasAR + 0.5f );
    }

    constexpr int MaxDim = 4096;        // safe upper bound for FMX bitmaps
    int const Larger = OutW > OutH ? OutW : OutH;
    float const Fit =
        Larger > MaxDim ? static_cast<float>( MaxDim ) / Larger : 1.0f;

    OutW = static_cast<int>( OutW * Fit + 0.5f );
    OutH = static_cast<int>( OutH * Fit + 0.5f );
    if ( OutW < 1 ) { OutW = 1; }
    if ( OutH < 1 ) { OutH = 1; }

    int DrawW = static_cast<int>( Bmp.Width * Fit + 0.5f );
    int DrawH = static_cast<int>( Bmp.Height * Fit + 0.5f );
    if ( DrawW < 1 ) { DrawW = 1; }
    if ( DrawH < 1 ) { DrawH = 1; }
    int const OfsX = ( OutW - DrawW ) / 2;
    int const OfsY = ( OutH - DrawH ) / 2;

    auto Out = make_unique<TBitmap>( OutW, OutH );
    if ( Out->Canvas->BeginScene() ) {
        // RAII: EndScene on every exit path, exceptions included.
        struct TSceneGuard {
            TCanvas& Canvas;
            ~TSceneGuard() { Canvas.EndScene(); }
        } SceneGuard { *Out->Canvas };

        Out->Canvas->Clear( claBlack );
        Out->Canvas->DrawBitmap(
            &Bmp,
            TRectF( 0, 0, Bmp.Width, Bmp.Height ),
            TRectF( OfsX, OfsY, OfsX + DrawW, OfsY + DrawH ),
            1.0f, true
        );
    }

    ImageViewer2->Bitmap->Clear( {} );
    ImageViewer2->BitmapScale = CanvasW / OutW;   // Out shares the canvas AR
    ImageViewer2->Bitmap->Assign( Out.get() );
}
//---------------------------------------------------------------------------

bool TfrmPanel::GetVignetting() const
{
    return NormalBlendEffect1->Enabled;
}
//---------------------------------------------------------------------------

void TfrmPanel::SetVignetting( bool Val )
{
    NormalBlendEffect1->Enabled = Val;
}
//---------------------------------------------------------------------------

bool TfrmPanel::GetBlackout() const
{
    return blackout_ && blackout_->Visible;
}
//---------------------------------------------------------------------------

void TfrmPanel::SetBlackout( bool Val )
{
    if ( !blackout_ ) { return; }
    if ( Val ) {
        blackout_->BringToFront();   // stay above any later-added layer
    }
    blackout_->Visible = Val;
}
//---------------------------------------------------------------------------

/*
TfrmPanel::ImageFileNameCont& TfrmPanel::GetImages()
{
    return entries_;
}
*/
//---------------------------------------------------------------------------

bool TfrmPanel::IsIdle() const
{
    return ( phase_ % 3 ) == 0;
}
//---------------------------------------------------------------------------

void TfrmPanel::LoadMechanicalSound()
{
    if ( player_ ) {
        player_->LoadWaveFromResource(
            HInstance, _D( "sound" ),
            static_cast<float>( mechSoundVolume_ ) / 100.0F
        );
    }
}
//---------------------------------------------------------------------------

void TfrmPanel::LoadNoiseSound( WavePlayer& Player )
{
    Player.LoadWaveFromResource(
        HInstance, _D( "noise" ),
        static_cast<float>( noiseSoundVolume_ ) / 100.0F
    );
}
//---------------------------------------------------------------------------

void TfrmPanel::SetMechSoundVolume( int Val )
{
    if ( mechSoundVolume_ != Val ) {
        mechSoundVolume_ = Val;
        LoadMechanicalSound();
    }
}
//---------------------------------------------------------------------------

void TfrmPanel::PlayMechanicalSound()
{
    if ( player_ ) {
        player_->Play( false );
    }
}
//---------------------------------------------------------------------------

void TfrmPanel::PlayNoiseSound()
{
    if ( playerNoise_ ) {
        if ( fanNoise_ ) {
            playerNoise_->Play( true );
        }
        else {
            playerNoise_->Stop();
        }
    }
}
//---------------------------------------------------------------------------

/*
void TfrmPanel::Next()
{
    PlayMechanicalSound();
    ++phase_;
    backward_ = false;
    FloatAnimation2->StopValue = ImageViewer2->Width * 2;
    FloatAnimation2->Start();
    FloatAnimation1->Start();
}
//---------------------------------------------------------------------------

void TfrmPanel::Prior()
{
    PlayMechanicalSound();
    ++phase_;
    backward_ = true;
    FloatAnimation2->StopValue = ImageViewer2->Width * 2;
    FloatAnimation2->Start();
    FloatAnimation1->Start();
}
//---------------------------------------------------------------------------
*/
void TfrmPanel::ChangePicture( bool Backward )
{
    backward_ = Backward;
    PlayMechanicalSound();
    ++phase_;
    FloatAnimation2->StopValue = ImageViewer2->Width * 2;
    FloatAnimation2->Start();
    FloatAnimation1->Start();
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanel::KeyDown( System::Word &Key, System::WideChar &KeyChar,
                                    System::Classes::TShiftState Shift )
{
    // Give the host first crack at the key (e.g. the presenter shortcuts) so the
    // projector window reacts even when the main window is minimized to the tray.
    if ( onPictureKey_ && onPictureKey_( Key, KeyChar, Shift ) ) {
        Key = 0;
        KeyChar = 0;
        return;
    }
    inherited::KeyDown( Key, KeyChar, Shift );
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanel::FormShow(TObject *Sender)
{
    LoadNoiseSound( *playerNoise_ );
    PlayNoiseSound();
}
//---------------------------------------------------------------------------

bool TfrmPanel::GetFanNoise() const
{
    return fanNoise_;
}
//---------------------------------------------------------------------------

void TfrmPanel::SetFanNoise( bool Val )
{
    if ( fanNoise_ != Val ) {
        fanNoise_ = Val;
        PlayNoiseSound();
    }
}
//---------------------------------------------------------------------------

