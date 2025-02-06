//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <System.IOUtils.hpp>

#include <FMX.Filter.Effects.hpp>
#include <FMX.DialogService.Sync.hpp>

#include <filesystem>
#include <algorithm>

#include "FormMain.h"
#include "FormConfig.h"
#include "AppUtils.h"
#include "DataModStyleRes.h"
#include "BrowseFolder.h"

using Anafestica::TConfigNode;

using std::make_unique;
using std::clamp;
using std::filesystem::is_directory;
using std::filesystem::directory_iterator;
using std::filesystem::recursive_directory_iterator;
using std::min;

using AppUtils::GetConfigBaseNode;

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "FMXFormAppMain"
#pragma link "FMX.SVGIconImage"
#pragma link "FMX.SVGIconImageList"
#pragma resource "*.fmx"
TfrmMain *frmMain;
//---------------------------------------------------------------------------

__fastcall TfrmMain::TfrmMain(TComponent* Owner)
    : TfrmPanelAppMain(Owner)
{
    ShowFileInfo( {} );
    PicturesPath = System::Ioutils::TPath::GetPicturesPath();
	Application->OnIdle = &IdleEvent;
    RestoreProperties();
}
//---------------------------------------------------------------------------

__fastcall TfrmMain::~TfrmMain()
{
    try {
        SaveProperties();
    }
    catch( ... ) {
    }
}
//---------------------------------------------------------------------------

void TfrmMain::RestoreProperties()
{
    TConfigNode& BaseNode = GetConfigBaseNode( GetConfigRootNode() );
    TConfigNode& PanelNode = BaseNode.GetSubNode( _D( "Panel" ) );
    RESTORE_PROPERTY( PanelNode, Vignetting );
    RESTORE_PROPERTY( PanelNode, MechanicalSoundVolume );
    RESTORE_PROPERTY( PanelNode, FanNoise );
    RESTORE_PROPERTY( PanelNode, NoiseSoundVolume );
    RESTORE_PROPERTY( PanelNode, PicturesPath );
    RESTORE_PROPERTY( PanelNode, RecursivePicturesSearch );
}
//---------------------------------------------------------------------------

void TfrmMain::SaveProperties() const
{
    TConfigNode& BaseNode = GetConfigBaseNode( GetConfigRootNode() );
    TConfigNode& PanelNode = BaseNode.GetSubNode( _D( "Panel" ) );
    SAVE_PROPERTY( PanelNode, Vignetting );
    SAVE_PROPERTY( PanelNode, MechanicalSoundVolume );
    SAVE_PROPERTY( PanelNode, FanNoise );
    SAVE_PROPERTY( PanelNode, NoiseSoundVolume );
    SAVE_PROPERTY( PanelNode, PicturesPath );
    SAVE_PROPERTY( PanelNode, RecursivePicturesSearch );
}
//---------------------------------------------------------------------------

void TfrmMain::LoadPictures()
{
    idx_ = {};
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
        if ( RecursivePicturesSearch ) {
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
}
//---------------------------------------------------------------------------

void TfrmMain::LoadPicture( size_t Idx )
{
	if ( Idx < entries_.size() ) {
        auto FileName = entries_[Idx];
        auto Bmp = make_unique<TBitmap>( FileName );
        panel_->LoadPicture( *Bmp );
        ShowFileInfo( Idx );
	}
}
//---------------------------------------------------------------------------

void TfrmMain::Start()
{
    inherited::Start();
    idx_ = {};
    CreatePanel(
        GetSelectedDisplay(), PanelClipping, PanelScaling, PanelKeepAspectRatio
    );
    LoadPictures();
    LoadPicture( idx_ );
}
//---------------------------------------------------------------------------

void TfrmMain::Stop()
{
    entries_.clear();
    ShowFileInfo( {} );
    if ( GetPanel() ) {
        tmrPolling->Enabled = false;
        inherited::Stop();
        DestroyPanel();
    }
}
//---------------------------------------------------------------------------

void TfrmMain::CreatePanel( FMXWinDisplayDev const * Display, bool Clipping,
                            bool Scaling, bool KeepAspectRatio )
{
    panel_ =
        make_unique<PanelType>(
            nullptr
          , MechanicalSoundVolume
          , FanNoise
          , NoiseSoundVolume
          , RecursivePicturesSearch
          , Display
          , Display ? StoreOpts::None : StoreOpts::All
          , &GetConfigBaseNode( GetConfigRootNode() ).GetSubNode( _D( "Panel" ) )
        );

    panel_->OnLoadPicture = &OnLoadPicture;
    panel_->Show();
    panel_->MonitorClipping = Clipping;
    panel_->MonitorScaling = Scaling;
    panel_->MaintainAspectRatio = KeepAspectRatio;
    panel_->Vignetting = Vignetting;
}
//---------------------------------------------------------------------------

void TfrmMain::DestroyPanel()
{
    panel_.reset();
}
//---------------------------------------------------------------------------

void TfrmMain::Config()
{
    auto Form = make_unique<TfrmConfig>( nullptr );
    Form->AutoStart = AutoStart;
    Form->AutoMinimizeOnTray = AutoMinimizeOnTray;
    if ( Form->ShowModal() == mrOk ) {
        AutoStart = Form->AutoStart;
        AutoMinimizeOnTray = Form->AutoMinimizeOnTray;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::FormCloseQuery(TObject *Sender, bool &CanClose)
{
    Stop();
    CanClose = true;
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPicturePriorExecute(TObject *Sender)
{
    ProjectorPanel->ChangePicture( true );
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPictureNextExecute(TObject *Sender)
{
    ProjectorPanel->ChangePicture( false );
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPictureChangeUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = ProjectorPanel && !entries_.empty() && ProjectorPanel->IsIdle();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::OnLoadPicture( TObject *Sender, bool Backward )
{
    if ( Backward ) {
        idx_ = ( idx_ ? idx_ : entries_.size() ) - 1;
    }
    else {
        idx_ = ( idx_ + 1 ) % entries_.size();
    }
    LoadPicture( idx_ );
}
//---------------------------------------------------------------------------

void TfrmMain::SetVignetting( bool Val )
{
    actPanelVignetting->Checked = Val;
    vignetting_ = Val;

    if ( ProjectorPanel ) {
        //ProjectorPanel->Vignetting = Val;
        ProjectorPanel->Vignetting = true;
    }
}
//---------------------------------------------------------------------------

int TfrmMain::GetMechanicalSoundVolume() const
{
    return trackbarMechSndVol->Value;
}
//---------------------------------------------------------------------------

void TfrmMain::SetMechanicalSoundVolume( int Val )
{
    Val = clamp( Val, 0, 100 );
    trackbarMechSndVol->Value = Val;
    if ( ProjectorPanel ) {
        ProjectorPanel->MechSoundVolume = Val;
    }
}
//---------------------------------------------------------------------------

String TfrmMain::GetPicturesPath() const
{
    return Trim( edtPicturesPath->Text );
}
//---------------------------------------------------------------------------

void TfrmMain::SetPicturesPath( String Val )
{
    picturesPath_ = Trim( Val );
    edtPicturesPath->Text = Val;
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPanelVignettingExecute(TObject *Sender)
{
    vignetting_ = !vignetting_;
    actPanelVignetting->Checked = vignetting_;
    if ( ProjectorPanel ) {
        ProjectorPanel->Vignetting = vignetting_;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPanelVignettingUpdate(TObject *Sender)
{
/*
    auto& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = !ProjectorPanel || !ProjectorPanel->WindowMode;
*/
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::trackbarMechSndVolChange(TObject *Sender)
{
//
    SetMechanicalSoundVolume( trackbarMechSndVol->Value );
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::IdleEvent( TObject* Sender, bool &Done )
{
    trackbarMechSndVol->Enabled = !ProjectorPanel || ProjectorPanel->IsIdle();
    edtPicturesPath->Enabled = !ProjectorPanel;
    Done = true;
}
//---------------------------------------------------------------------------

bool TfrmMain::GetRecursivePicturesSearch() const
{
    return actPanelRecursivePicturesSearch->Checked;
}
//---------------------------------------------------------------------------

void TfrmMain::SetRecursivePicturesSearch( bool Val )
{
    actPanelRecursivePicturesSearch->Checked = Val;
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPanelRecursivePicturesSearchExecute(TObject *Sender)

{
//
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPanelRecursivePicturesSearchUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = !ProjectorPanel;
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actFileBrowsePicturesPathExecute(TObject *Sender)
{
    if ( auto NewPath = BrowseFolder( PicturesPath ); !NewPath.IsEmpty() ) {
        PicturesPath = NewPath;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actFileBrowsePicturesPathUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = !ProjectorPanel;
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::trackbarNoiseSndVolChange(TObject *Sender)
{
    SetNoiseSoundVolume( trackbarNoiseSndVol->Value );
}
//---------------------------------------------------------------------------

int TfrmMain::GetNoiseSoundVolume() const
{
    return trackbarNoiseSndVol->Value;
}
//---------------------------------------------------------------------------

void TfrmMain::SetNoiseSoundVolume(int Val)
{
    Val = clamp( Val, 0, 100 );
    trackbarNoiseSndVol->Value = Val;
    tmrChangeSoundVol->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::tmrChangeSoundVolTimer(TObject *Sender)
{
    if ( ProjectorPanel ) {
        ProjectorPanel->NoiseSoundVolume = NoiseSoundVolume;
    }
    tmrChangeSoundVol->Enabled = false;
}
//---------------------------------------------------------------------------

bool TfrmMain::GetFanNoise() const
{
    return switchFanNoise->IsChecked;
}
//---------------------------------------------------------------------------

void TfrmMain::SetFanNoise( bool Val )
{
    switchFanNoise->IsChecked = Val;
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPanelFanNoiseExecute(TObject *Sender)
{
    if ( ProjectorPanel ) {
        ProjectorPanel->FanNoise = FanNoise;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPanelFanNoiseUpdate(TObject *Sender)
{
//
}
//---------------------------------------------------------------------------

void TfrmMain::ShowFileInfo( size_t Idx )
{
    if ( auto Size = entries_.size(); Idx < Size ) {
        lblPictureInfo->Text =
            Format( _D( "Picture %d of %d " ), ARRAYOFCONST(( Idx, Size )) );
    }
    else {
        lblPictureInfo->Text = {};
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::Button4Paint(TObject *Sender, TCanvas *Canvas, const TRectF &ARect)
{
    auto& Btn = static_cast<TButton&>( *Sender );
    auto DstRect = ARect;
    DstRect.Inflate( -8, -8 );
    auto Size = min( DstRect.Width(), DstRect.Height() );
    DstRect = TRectF( DstRect.TopLeft(), Size, Size );
    auto SrcRect = TRectF( TPointF{}, Size, Size );
    auto Bmp = SVGIconImageList1->Bitmap( SrcRect.Size, Btn.Tag );
    Canvas->DrawBitmap( Bmp, SrcRect, DstRect, Btn.Enabled ? 1.0F : 0.5F );
}
//---------------------------------------------------------------------------


