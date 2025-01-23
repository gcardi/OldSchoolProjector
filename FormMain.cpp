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
    bool PnlVignetting = PanelVignetting;
    PanelNode.GetItem( _D( "Vignetting" ), PnlVignetting );
    PanelVignetting = PnlVignetting;
}
//---------------------------------------------------------------------------

void TfrmMain::SaveProperties() const
{
    TConfigNode& BaseNode = GetConfigBaseNode( GetConfigRootNode() );
    TConfigNode& PanelNode = BaseNode.GetSubNode( _D( "Panel" ) );
    SaveValue( PanelNode, _D( "Vignetting" ), PanelVignetting );
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
          , Display
          , Display ? StoreOpts::None : StoreOpts::All
//          , &GetConfigRootNode().GetSubNode( _D( "Panel" ) )
          , &GetConfigBaseNode( GetConfigRootNode() ).GetSubNode( _D( "Panel" ) )
        );

    panel_->Show();
    panel_->MonitorClipping = Clipping;
    panel_->MonitorScaling = Scaling;
    panel_->MaintainAspectRatio = KeepAspectRatio;
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
    panel_->Prior();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPicturePriorUpdate(TObject *Sender)
{
    TAction& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = GetPanel() && !panel_->Images.empty() && panel_->IsIdle();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPictureNextExecute(TObject *Sender)
{
    panel_->Next();
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPictureNextUpdate(TObject *Sender)
{
    TAction& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = GetPanel() && !panel_->Images.empty() && panel_->IsIdle();
}
//---------------------------------------------------------------------------

void TfrmMain::SetPanelVignetting( bool Val )
{
    actPanelVignetting->Checked = Val;
    panelVignetting_ = Val;
    if ( auto Panel = dynamic_cast<TfrmPanel*>( GetPanel() ) ) {
        Panel->Vignetting = Val;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPanelVignettingExecute(TObject *Sender)
{
    panelVignetting_ = !panelVignetting_;
    actPanelVignetting->Checked = panelVignetting_;
    if ( auto Panel = dynamic_cast<TfrmPanel*>( GetPanel() ) ) {
        Panel->Vignetting = PanelVignetting;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmMain::actPanelVignettingUpdate(TObject *Sender)
{
/*
    TAction& Act = static_cast<TAction&>( *Sender );
    TfrmPanelBase* const Panel = GetPanel();
    Act.Enabled = !Panel || !Panel->WindowMode;
*/
}
//---------------------------------------------------------------------------

