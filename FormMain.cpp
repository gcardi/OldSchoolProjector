//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <FMX.Filter.Effects.hpp>
#include <FMX.DialogService.Sync.hpp>

#include "FormMain.h"
#include "FormConfig.h"
#include "AppUtils.h"
#include "DataModStyleRes.h"

using Anafestica::TConfigNode;

using std::make_unique;
using std::clamp;

using AppUtils::GetConfigBaseNode;

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "FMXFormAppMain"
#pragma resource "*.fmx"
TfrmMain *frmMain;
//---------------------------------------------------------------------------

__fastcall TfrmMain::TfrmMain(TComponent* Owner)
    : TfrmPanelAppMain(Owner)
{
    // build buttons
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
    auto PnlVignetting = PanelVignetting;
    PanelNode.GetItem( _D( "Vignetting" ), PnlVignetting );
    PanelVignetting = PnlVignetting;
    auto MSndVol = MechSndVol;
    PanelNode.GetItem( _D( "MechanicalSoundVolume" ), MSndVol );
    MechSndVol = MSndVol;
}
//---------------------------------------------------------------------------

void TfrmMain::SaveProperties() const
{
    TConfigNode& BaseNode = GetConfigBaseNode( GetConfigRootNode() );
    TConfigNode& PanelNode = BaseNode.GetSubNode( _D( "Panel" ) );
    SaveValue( PanelNode, _D( "Vignetting" ), PanelVignetting );
    SaveValue( PanelNode, _D( "MechanicalSoundVolume" ), MechSndVol );
}
//---------------------------------------------------------------------------

void TfrmMain::Start()
{
    inherited::Start();
    CreatePanel(
        GetSelectedDisplay(), PanelClipping, PanelScaling, PanelKeepAspectRatio
    );
}
//---------------------------------------------------------------------------

void TfrmMain::Stop()
{
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
          , MechSndVol
          , Display
          , Display ? StoreOpts::None : StoreOpts::All
//          , &GetConfigRootNode().GetSubNode( _D( "Panel" ) )
          , &GetConfigBaseNode( GetConfigRootNode() ).GetSubNode( _D( "Panel" ) )
        );

    panel_->Show();
    panel_->MonitorClipping = Clipping;
    panel_->MonitorScaling = Scaling;
    panel_->MaintainAspectRatio = KeepAspectRatio;
    panel_->Vignetting = PanelVignetting;
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
    ProjectorPanel->Prior();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPicturePriorUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = ProjectorPanel && !ProjectorPanel->Images.empty() &&
                  ProjectorPanel->IsIdle();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPictureNextExecute(TObject *Sender)
{
    ProjectorPanel->Next();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPictureNextUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = ProjectorPanel && !ProjectorPanel->Images.empty() &&
                  ProjectorPanel->IsIdle();
}
//---------------------------------------------------------------------------

void TfrmMain::SetPanelVignetting( bool Val )
{
    actPanelVignetting->Checked = Val;
    panelVignetting_ = Val;
    if ( ProjectorPanel ) {
        ProjectorPanel->Vignetting = Val;
    }
}
//---------------------------------------------------------------------------

int TfrmMain::GetMechSoundVol() const
{
    return trackbarMechSndVol->Value;
}
//---------------------------------------------------------------------------

void TfrmMain::SetMechSndVol( int Val )
{
    Val = clamp( Val, 0, 100 );
    //if ( Val != GetMechSoundVol() ) {
        trackbarMechSndVol->Value = Val;
        if ( ProjectorPanel ) {
            ProjectorPanel->MechSoundVolume = Val;
        }
    //}
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPanelVignettingExecute(TObject *Sender)
{
    panelVignetting_ = !panelVignetting_;
    actPanelVignetting->Checked = panelVignetting_;
    ProjectorPanel->Vignetting = PanelVignetting;
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
    SetMechSndVol( trackbarMechSndVol->Value );
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::IdleEvent( TObject* Sender, bool &Done )
{
    trackbarMechSndVol->Enabled = !ProjectorPanel || ProjectorPanel->IsIdle();
    Done = true;
}
//---------------------------------------------------------------------------


