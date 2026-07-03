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
#include "ThumbnailMaker.h"

using Anafestica::TConfigNode;

using std::make_unique;
using std::clamp;
using std::filesystem::is_directory;
using std::filesystem::directory_iterator;
using std::filesystem::recursive_directory_iterator;
using std::min;

using AppUtils::GetConfigBaseNode;

// Thumbnails are produced at this size and scaled to the slot when drawn, so
// the cache is independent of the current strip geometry.
static constexpr int ThumbTargetSize = 256;

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "FMXFormAppMain"
#pragma link "FMX.SVGIconImage"
#pragma link "FMX.SVGIconImageList"
#pragma link "ThumbnailStrip"
#pragma resource "*.fmx"
TfrmMain *frmMain;
//---------------------------------------------------------------------------

__fastcall TfrmMain::TfrmMain(TComponent* Owner)
    : TfrmPanelAppMain(Owner)
{
    loader_ = std::make_unique<TThumbnailLoader>( ThumbTargetSize );

    thumbTimer_ = std::make_unique<TTimer>( static_cast<TComponent*>( nullptr ) );
    thumbTimer_->Interval = 66;   // ~15 fps poll while thumbnails are loading
    thumbTimer_->Enabled = false;
    thumbTimer_->OnTimer = &ThumbPollTimer;

    frameThumbs->OnRequestThumbnail = &ThumbRequest;
    frameThumbs->OnPick = &ThumbPick;
    frameThumbs->OnVisibleRangeChanged = &ThumbVisibleRange;
    // TFrame does not publish Anchors for streaming, so set it here: keep the
    // strip stretched between the Prior/Next buttons as the form is resized.
    frameThumbs->Anchors = TAnchors()
        << TAnchorKind::akLeft << TAnchorKind::akRight << TAnchorKind::akBottom;

    ShowFileInfo( {} );
    PicturesPath = System::Ioutils::TPath::GetPicturesPath();
	Application->OnIdle = &IdleEvent;
    RestoreProperties();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::ThumbRequest( TObject* /*Sender*/, int Index,
                                        TBitmap*& Bmp )
{
    // Non-blocking: return whatever the worker has already produced. Missing
    // thumbnails are requested via OnVisibleRangeChanged and drawn as
    // placeholders until they arrive.
    Bmp = loader_ ? loader_->Get( Index ) : nullptr;
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::ThumbPick( TObject* /*Sender*/, int Index )
{
    if ( !GetPanel() ) {
        return;   // only meaningful while projecting
    }
    if ( Index < 0 || static_cast<size_t>( Index ) >= entries_.size() ) {
        return;
    }
    if ( !ProjectorPanel->IsIdle() ) {
        return;   // ignore clicks during a slide transition, like Next/Prev
    }

    int const Cur = static_cast<int>( idx_ );
    if ( Index == Cur ) {
        return;   // already the projected slide
    }

    // Any clicked slide (adjacent or far) changes like Next/Prev: mechanical
    // sound (if its volume is up) and the sliding animation, sliding in the
    // direction of the target. pendingTarget_ makes OnLoadPicture jump straight
    // to Index instead of stepping by one.
    pendingTarget_ = Index;
    ProjectorPanel->ChangePicture( Index < Cur );   // backward if before current
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::ThumbVisibleRange( TObject* /*Sender*/, int First,
                                             int Last )
{
    if ( loader_ ) {
        loader_->EnsureRange( First, Last );
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::ThumbPollTimer( TObject* /*Sender*/ )
{
    // Repaint the strip only when the worker has produced new thumbnails.
    if ( loader_ && loader_->ConsumeReady() ) {
        frameThumbs->RefreshThumbnails();
    }
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
    entries_.clear();
    // Pick up the path currently in the edit field: it may have been typed
    // directly, which does not go through SetPicturesPath()/picturesPath_.
    picturesPath_ = GetPicturesPath();
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
        // Load full-size with EXIF auto-orientation (same path as thumbnails).
        auto Bmp = LoadImageOriented( FileName, 0, 0 );
        if ( Bmp ) {
            panel_->LoadPicture( *Bmp );
        }
        ShowFileInfo( Idx );
        frameThumbs->SelectedIndex = static_cast<int>( Idx );
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
    // Feed the worker before setting Count: setting Count fires
    // OnVisibleRangeChanged, which asks the loader to preload the first window.
    loader_->SetEntries( entries_ );
    thumbTimer_->Enabled = true;
    frameThumbs->ThumbAspectRatio = GetSelectedAspectRatio();
    frameThumbs->Count = static_cast<int>( entries_.size() );
    LoadPicture( idx_ );
}
//---------------------------------------------------------------------------

void TfrmMain::Stop()
{
    entries_.clear();
    thumbTimer_->Enabled = false;
    loader_->SetEntries( {} );
    frameThumbs->Count = 0;
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
    panel_->OnPictureKey =
        [this]( System::Word Key, System::Classes::TShiftState Shift ) {
            return TryPictureShortcut( Key, Shift );
        };
    panel_->Show();
    panel_->AspectRatio = GetSelectedAspectRatio();
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

// Build a TShortCut (same encoding as the IDE stores) from a key + modifiers.
static System::Classes::TShortCut MakeShortCut(
    System::Word Key, System::Classes::TShiftState Shift )
{
    unsigned Sc = Key;
    if ( Shift.Contains( System::Classes::ssShift ) ) { Sc |= 0x2000; }
    if ( Shift.Contains( System::Classes::ssCtrl ) )  { Sc |= 0x4000; }
    if ( Shift.Contains( System::Classes::ssAlt ) )   { Sc |= 0x8000; }
    return static_cast<System::Classes::TShortCut>( Sc );
}
//---------------------------------------------------------------------------

bool TfrmMain::TryPictureShortcut( System::Word Key,
                                   System::Classes::TShiftState Shift )
{
    // Same guard as the actions' OnUpdate, checked directly (the action list
    // may not be updating while the main window is minimized to the tray).
    if ( !ProjectorPanel || entries_.empty() || !ProjectorPanel->IsIdle() ) {
        return false;
    }

    System::Classes::TShortCut const Sc = MakeShortCut( Key, Shift );
    if ( Sc == 0 ) {
        return false;
    }
    if ( Sc == actPicturePrior->ShortCut ) {
        ProjectorPanel->ChangePicture( true );
        return true;
    }
    if ( Sc == actPictureNext->ShortCut ) {
        ProjectorPanel->ChangePicture( false );
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::OnLoadPicture( TObject *Sender, bool Backward )
{
    if ( pendingTarget_ >= 0 ) {
        // A thumbnail click asked to jump straight to this slide (possibly far).
        idx_ = static_cast<size_t>( pendingTarget_ );
        pendingTarget_ = -1;
    }
    else if ( Backward ) {
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
            Format( _D( "Picture %d of %d " ), ARRAYOFCONST(( Idx + 1, Size )) );
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


