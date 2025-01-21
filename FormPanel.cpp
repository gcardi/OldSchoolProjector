//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <FMX.Platform.Win.hpp>

#include <algorithm>
#include <memory>
#include <filesystem>
#include <cassert>

#include "FormPanel.h"
#include "DataModStyleRes.h"

using Fmx::Platform::Win::WindowHandleToPlatform;

using std::make_unique;
using std::swap;
using std::filesystem::directory_iterator;
using std::filesystem::is_directory;

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "FMXFormPanelBase"
#pragma resource "*.fmx"
//TfrmPanel *frmPanel;
//---------------------------------------------------------------------------

/*
__fastcall TfrmPanel::TfrmPanel( TComponent* Owner,
                                 FMXWinDisplayDev const * Display,
                                 Anafestica::TConfigNode* const RootNode )
    : TfrmPanel( Owner, Display, StoreOpts::All, RootNode )
{
}
*/
//---------------------------------------------------------------------------

__fastcall TfrmPanel::TfrmPanel( TComponent* Owner,
                                 FMXWinDisplayDev const * Display,
                                 StoreOpts StoreOptions,
                                 Anafestica::TConfigNode* const RootNode )
    : TfrmPanelBase( Owner, Display, StoreOptions, RootNode )
    , player_ { new WavePlayer{ WindowHandleToPlatform( Handle )->Wnd } }
{
    RestoreProperties();

    player_->LoadWaveFromResource( HInstance, _D( "sound" ) );

	//std::wstring Path = LR"=(C:\Users\Giuliano\Desktop\SlideShow)=";
	std::wstring Path = LR"=(.)=";
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
        idx_ = ( idx_ + 1 ) % entries_.size();
        LoadImage( idx_ );
        FloatAnimation2->Start();
        FloatAnimation1->Start();
    }
}
//---------------------------------------------------------------------------

void TfrmPanel::LoadImage( size_t Index )
{
    auto FileName = entries_[Index];
    auto Bmp = make_unique<TBitmap>( FileName );
    //ImageViewer2->Bitmap->LoadFromFile( entries_[Index] );
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
//---------------------------------------------------------------------------

TfrmPanel::ImageFileNameCont& TfrmPanel::GetImages()
{
    return entries_;
}
//---------------------------------------------------------------------------

void TfrmPanel::Next()
{
    player_->Play( false );
    ++phase_;
    FloatAnimation2->StopValue = ImageViewer2->Width * 2;
    FloatAnimation2->Start();
    FloatAnimation1->Start();
}
//---------------------------------------------------------------------------



