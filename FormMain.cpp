//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <System.IOUtils.hpp>

#include <FMX.Filter.Effects.hpp>
#include <FMX.DialogService.Sync.hpp>

#include <filesystem>
#include <algorithm>
#include <set>
#include <string>
#include <cwctype>

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
        // Accept every file the installed WIC codecs can decode - not just
        // JPG/PNG. Add the Canon/Nikon (or Microsoft) RAW codec pack and .cr2 /
        // .nef / ... start being picked up here too, with no code change.
        std::set<std::wstring> const SupportedExt = GetWICSupportedExtensions();
        auto Inserter = [this, &SupportedExt]( auto const & Entry )
        {
            auto Path = Entry.path();
            std::wstring Ext = Path.extension().wstring();
            for ( wchar_t& C : Ext ) { C = static_cast<wchar_t>( towlower( C ) ); }
			if ( SupportedExt.count( Ext ) > 0 ) {
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
        [this]( System::Word Key, System::WideChar KeyChar,
                System::Classes::TShiftState Shift ) {
            return TryPresenterShortcut( Key, KeyChar, Shift );
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

// Central command handler for USB/Bluetooth presenters. Presenters enumerate as
// a plain HID keyboard and send whatever keys the target app expects, so we match
// the de-facto PowerPoint slideshow keys plus the synonyms different models emit:
//   Next    : Page Down / Right / Down / Space / Enter
//   Previous: Page Up / Left / Up / Backspace
//   Start   : F5
//   Stop    : Esc / F6
//   Black   : 'B' or '.'   (toggles the black-screen overlay on/off)
// Letters/punctuation are matched on KeyChar (layout independent), navigation on
// the virtual key. Each command guards its own applicability via the live panel
// state (not the actions' Enabled, which may be stale while minimized to tray).
bool TfrmMain::TryPresenterShortcut( System::Word Key, System::WideChar KeyChar,
                                     System::Classes::TShiftState Shift )
{
    namespace vk = System::Uitypes;

    // Real presenters send bare keys; ignore Ctrl/Alt combos so we never clash
    // with the app's own Alt+Fx panel shortcuts.
    if ( Shift.Contains( System::Classes::ssCtrl ) ||
         Shift.Contains( System::Classes::ssAlt ) ) {
        return false;
    }

    System::WideChar const Ch =
        ( KeyChar >= L'A' && KeyChar <= L'Z' ) ? KeyChar + 32 : KeyChar;

    bool const Next = Key == vk::vkNext  || Key == vk::vkRight ||
                      Key == vk::vkDown  || Key == vk::vkSpace ||
                      Key == vk::vkReturn;
    bool const Prev = Key == vk::vkPrior || Key == vk::vkLeft ||
                      Key == vk::vkUp    || Key == vk::vkBack;

    if ( Next || Prev ) {
        // Needs loaded pictures and an idle (not mid-animation) projector.
        if ( !ProjectorPanel || entries_.empty() || !ProjectorPanel->IsIdle() ) {
            return false;
        }
        ProjectorPanel->ChangePicture( Prev );
        return true;
    }

    if ( Key == vk::vkF5 ) {                 // Start (only when not running)
        if ( GetPanel() ) { return false; }
        Start();
        return true;
    }

    if ( Key == vk::vkEscape || Key == vk::vkF6 ) {   // Stop (only when running)
        if ( !GetPanel() ) { return false; }
        Stop();
        return true;
    }

    if ( Ch == L'b' || Ch == L'.' ) {        // Blank: toggle the black overlay
        if ( !ProjectorPanel ) { return false; }
        actPanelBlackout->Execute();          // toggles the overlay + the switch
        return true;
    }

    return false;
}
//---------------------------------------------------------------------------

bool TfrmMain::IsInputControlFocused() const
{
    _di_IControl const Ctl = Focused;
    if ( !Ctl ) { return false; }
    for ( TFmxObject* Obj = dynamic_cast<TFmxObject*>( Ctl->GetObject() );
          Obj; Obj = Obj->Parent ) {
        if ( dynamic_cast<TCustomEdit*>( Obj ) ||
             dynamic_cast<TComboBox*>( Obj )   ||
             dynamic_cast<TTrackBar*>( Obj ) ) {
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------------

// Route the main window's keys through the presenter handler as well, so a
// clicker works whether the control window or the full-screen projector has the
// focus. Runs after the inherited dispatch (which fires the ActionList's own
// F5/F6/PgUp/PgDn shortcuts and lets the focused control consume its keys first)
// and never overrides typing in an edit/combo/track bar.
void __fastcall TfrmMain::KeyDown( System::Word &Key, System::WideChar &KeyChar,
                                   System::Classes::TShiftState Shift )
{
    inherited::KeyDown( Key, KeyChar, Shift );
    if ( ( Key == 0 && KeyChar == 0 ) || IsInputControlFocused() ) {
        return;
    }
    if ( TryPresenterShortcut( Key, KeyChar, Shift ) ) {
        Key = 0;
        KeyChar = 0;
    }
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

void __fastcall TfrmMain::actPanelBlackoutExecute(TObject *Sender)
{
    // Toggle the projector's black overlay. Read the live panel state (rather
    // than the action's Checked) so UI clicks and the presenter key behave the
    // same regardless of AutoCheck, then mirror it back onto Checked/the switch.
    if ( auto Panel = ProjectorPanel ) {
        bool const NewVal = !Panel->Blackout;
        Panel->Blackout = NewVal;
        actPanelBlackout->Checked = NewVal;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPanelBlackoutUpdate(TObject *Sender)
{
    // Only meaningful while projecting; keep the switch in sync with the panel
    // and reset it once projection stops (the state is transient, not stored).
    auto& Act = static_cast<TAction&>( *Sender );
    auto Panel = ProjectorPanel;
    Act.Enabled = Panel != nullptr;
    Act.Checked = Panel && Panel->Blackout;
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


