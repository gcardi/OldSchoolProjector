//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <memory>
#include <tuple>

#include <anafestica/FileVersionInfo.h>

#include <System.DateUtils.hpp>

#include "FMXFormAppMain.h"

#include "CmdLineOptions.h"
#include "CmdLineParser.h"

#include "AppUtils.h"

using std::make_unique;
using std::min;
using std::get;

using Anafestica::TFileVersionInfo;
using Anafestica::TConfigNode;

using AppUtils::ToIdx;
using AppUtils::GetConfigBaseNode;

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"

//TfrmPanelAppMain *frmPanelAppMain;

//---------------------------------------------------------------------------

namespace {

// The Display dropdown starts with these fixed "Window <ratio>" entries; the
// enumerated monitors are appended after them. The order here must match the
// Items.Strings order in FMXFormAppMain.fmx.
struct WindowRatioDef {
    wchar_t const * Suffix;   // label suffix and persisted id tail
    float Ratio;              // width / height
};

const WindowRatioDef WindowRatios[] = {
    { L"16:9",  16.0f /  9.0f },
    { L"4:3",    4.0f /  3.0f },
    { L"16:10", 16.0f / 10.0f },
};

constexpr int WindowItemCount =
    static_cast<int>( sizeof( WindowRatios ) / sizeof( WindowRatios[0] ) );

wchar_t const WindowDeviceIDPrefix[] = L"WINDOW:";

float WindowAspectRatioForIndex( int Idx )
{
    return ( Idx >= 0 && Idx < WindowItemCount ) ?
        WindowRatios[Idx].Ratio : WindowRatios[0].Ratio;
}

String WindowDeviceIDForIndex( int Idx )
{
    int const I = ( Idx >= 0 && Idx < WindowItemCount ) ? Idx : 0;
    return String( WindowDeviceIDPrefix ) + WindowRatios[I].Suffix;
}

// Index of the "Window <ratio>" item whose id matches Val, or 0 if none.
int WindowItemIndexForDeviceID( String Val )
{
    for ( int I = 0; I < WindowItemCount; ++I ) {
        if ( SameText( Val, WindowDeviceIDForIndex( I ) ) ) {
            return I;
        }
    }
    return 0;
}

bool IsWindowDeviceID( String Val )
{
    String const Prefix( WindowDeviceIDPrefix );
    return Val.SubString( 1, Prefix.Length() ) == Prefix;
}

} // namespace

//---------------------------------------------------------------------------

LRESULT WINAPI NewSubclassproc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                                UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
    switch ( uMsg ) {
        case WM_SYSCOMMAND:
            if ( wParam == SC_MINIMIZE ) {
                reinterpret_cast<TfrmPanelAppMain*>( dwRefData )->MinimizeToTrayBar();
                return 0;
            }
            break;
    }
    return ::DefSubclassProc( hWnd, uMsg, wParam, lParam );
}
//---------------------------------------------------------------------------

__fastcall TfrmPanelAppMain::TfrmPanelAppMain(TComponent* Owner)
    : TfrmPanelAppMain(
        Owner, StoreOpts::OnlyPos, &GetConfigBaseNode( GetConfigRootNode() )
      )
{
}
//---------------------------------------------------------------------------

__fastcall TfrmPanelAppMain::TfrmPanelAppMain( TComponent* Owner, StoreOpts StoreOptions,
                                               Anafestica::TConfigNode* const RootNode )
    : TConfigRegistryForm( Owner, StoreOpts::OnlyPos, RootNode )
{
    Init();
}
//---------------------------------------------------------------------------

__fastcall TfrmPanelAppMain::~TfrmPanelAppMain()
{
    try {
        Destroy();
    }
    catch ( ... ) {
    }
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::Init()
{
    ::SetWindowSubclass(
        Fmx::Platform::Win::WindowHandleToPlatform( Handle )->Wnd,
        &NewSubclassproc,
        0,
        reinterpret_cast<DWORD_PTR>( this )
    );

    trayIcon_ = std::move( make_unique<TTrayIcon>( nullptr, Handle ) );
    trayIcon_->Icon = _D( "MAINICON" );
    trayIcon_->Hint = _D( "Tabellone" );
    trayIcon_->OnDblClick = &TrayIconDblClick;
    trayIcon_->OnRClick = &TrayIconRClick;

    displays_.reserve( Screen->DisplayCount );

    EnumFMXDisplays( back_inserter( displays_ ) );

    // Default selection: the primary monitor (falls back to the first
    // "Window" item if none reports itself as primary).
    int PrimaryComboIdx = 0;

    for( auto const & Disp : displays_ ) {
        auto const & FMXDisp = get<ToIdx( FMXWinDisplayDevField::FMXDisplay )>( Disp );

        auto const & GDisp =
            get<ToIdx( FMXWinDisplayDevField::Display )>( Disp );

        int const ComboIdx = comboboxPanelTopLeftScreen->Items->AddObject(
            Format(
                _D( "%s (%s)" ),
                ARRAYOFCONST((
                    GDisp.DeviceString
                  , GDisp.DeviceName
                ))
            ),
            reinterpret_cast<TObject*>(
                const_cast<FMXWinDisplayDevCont::pointer>( &Disp )
            )
        );

        if ( FMXDisp.Primary ) {
            PrimaryComboIdx = ComboIdx;
        }
    }

    comboboxPanelTopLeftScreen->ItemIndex = PrimaryComboIdx;

    SetupCaption();

    RestoreProperties();
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::Destroy()
{
    try {
        Stop();
    }
    catch ( ... ) {
    }
    SaveProperties();
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::SetupCaption()
{
    TFileVersionInfo Info( ParamStr( 0 ) );
    Caption =
        Format(
            _D( "%s, %s" ),
            ARRAYOFCONST((
                Info.FileDescription
              , Info.ProductVersion
            ))
        );
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::tmrAutoStartTimer(TObject *Sender)
{
    TTimer& Timer = dynamic_cast<TTimer&>( *Sender );
    Timer.Enabled = false;
    if ( AutoStart ) {
        Start();
        if ( AutoMinimizeOnTray ) {
            MinimizeToTrayBar();
        }
    }
}
//---------------------------------------------------------------------------

String TfrmPanelAppMain::GetDisplayDeviceID() const
{
    if ( FMXWinDisplayDev const * const Display = GetSelectedDisplay() ) {
        return get<ToIdx( FMXWinDisplayDevField::Display )>( *Display ).DeviceID;
    }
    // Windowed: persist which "Window <ratio>" item is selected.
    return WindowDeviceIDForIndex( comboboxPanelTopLeftScreen->ItemIndex );
}
//---------------------------------------------------------------------------

float TfrmPanelAppMain::GetSelectedAspectRatio() const
{
    if ( FMXWinDisplayDev const * const Display = GetSelectedDisplay() ) {
        TDisplay const & FMXDisp =
            get<ToIdx( FMXWinDisplayDevField::FMXDisplay )>( *Display );
        float const H = static_cast<float>( FMXDisp.Bounds.Height() );
        float const W = static_cast<float>( FMXDisp.Bounds.Width() );
        return H > 0.0f ? W / H : WindowRatios[0].Ratio;
    }
    return WindowAspectRatioForIndex( comboboxPanelTopLeftScreen->ItemIndex );
}
//---------------------------------------------------------------------------

template<typename T>
class DisplayDeviceIDIsType {
public:
    explicit DisplayDeviceIDIsType( T const & ID ) : id_( ID ) {}

    template<typename D>
    bool operator()( D const & Display ) const {
        return SameText(
            get<ToIdx( FMXWinDisplayDevField::Display )>( Display ).DeviceID,
            id_
        );
    }
private:
    T id_;
};

template<typename T>
inline DisplayDeviceIDIsType<T> DisplayDeviceIDIs( T const & ID ) {
    return DisplayDeviceIDIsType<T>( ID );
}

void TfrmPanelAppMain::SetDisplayDeviceID( String Val )
{
    if ( IsWindowDeviceID( Val ) ) {
        comboboxPanelTopLeftScreen->ItemIndex = WindowItemIndexForDeviceID( Val );
        return;
    }

    FMXWinDisplayDevCont::const_iterator ci =
        find_if(
            displays_.begin(), displays_.end(), DisplayDeviceIDIs( Val )
        );
    if ( ci != displays_.end() ) {
        comboboxPanelTopLeftScreen->ItemIndex =
            WindowItemCount +
            distance(
                static_cast<FMXWinDisplayDevCont const &>( displays_ ).begin(),
                ci
            );
    }
    else {
        comboboxPanelTopLeftScreen->ItemIndex = 0;
    }
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::RestoreProperties()
{
    // Put code here to restore attribute(s)
    TConfigNode& BaseNode = GetConfigBaseNode( GetConfigRootNode() );

    //RESTORE_VALUE( BaseNode, _D( "DisplayDeviceID" ), DisplayDeviceID );
    String DispDevId = DisplayDeviceID;
    RestoreValue( BaseNode, _D( "DisplayDeviceID" ), DispDevId );
    DisplayDeviceID = DispDevId;

//    AutoStart =
        //BaseNode.GetItem( _D( "AutoStart" ), AutoStart ).operator bool();
    bool AutoS = AutoStart;
    BaseNode.GetItem( _D( "AutoStart" ), AutoS );
    AutoStart = AutoS;

//    AutoMinimizeOnTray =
//        BaseNode.GetItem( _D( "AutoMinimizeOnTray" ), AutoMinimizeOnTray ).operator bool();
    bool AutoMinOnTray = AutoMinimizeOnTray;
    BaseNode.GetItem( _D( "AutoMinimizeOnTray" ), AutoMinOnTray );
    AutoMinimizeOnTray = AutoMinOnTray;

    TConfigNode& PanelNode = BaseNode.GetSubNode( _D( "Panel" ) );

    //PanelClipping =
    //    PanelNode.GetItem( _D( "Clipping" ), PanelClipping ).operator bool();
    bool PnlClipping = PanelClipping;
    PanelNode.GetItem( _D( "Clipping" ), PnlClipping );
    PanelClipping = PnlClipping;

    //PanelScaling =
    //    PanelNode.GetItem( _D( "Scaling" ), PanelScaling ).operator bool();
    bool PnlScaling = PanelScaling;
    PanelNode.GetItem( _D( "Scaling" ), PnlScaling );
    PanelScaling = PnlScaling;

    //PanelKeepAspectRatio =
    //    PanelNode.GetItem( _D( "KeepAspectRatio" ), PanelKeepAspectRatio ).operator bool();
    bool PnlKeepAspectRatio = PanelKeepAspectRatio;
    PanelNode.GetItem( _D( "KeepAspectRatio" ), PnlKeepAspectRatio );
    bool PanelKeepAspectRatio = PnlKeepAspectRatio;
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::SaveProperties() const
{
    // Put code here to restore attribute(s)
    TConfigNode& BaseNode = GetConfigBaseNode( GetConfigRootNode() );

    //SAVE_VALUE( BaseNode, _D( "DisplayDeviceID" ), DisplayDeviceID );
    SaveValue( BaseNode, _D( "DisplayDeviceID" ), DisplayDeviceID );

    //SAVE_VALUE( BaseNode, _D( "AutoStart" ), AutoStart );
    SaveValue( BaseNode, _D( "AutoStart" ), AutoStart );

    //SAVE_VALUE( BaseNode, _D( "AutoMinimizeOnTray" ), AutoMinimizeOnTray );
    SaveValue( BaseNode, _D( "AutoMinimizeOnTray" ), AutoMinimizeOnTray );

    TConfigNode& PanelNode =
        BaseNode.GetSubNode( _D( "Panel" ) );
    //SAVE_VALUE( PanelNode, _D( "Clipping" ), PanelClipping );
    SaveValue( BaseNode, _D( "Clipping" ), PanelClipping );
    //SAVE_VALUE( PanelNode, _D( "Scaling" ), PanelScaling );
    SaveValue( BaseNode, _D( "Scaling" ), PanelScaling );
    //SAVE_VALUE( PanelNode, _D( "KeepAspectRatio" ), PanelKeepAspectRatio );
    SaveValue( BaseNode, _D( "KeepAspectRatio" ), PanelKeepAspectRatio );
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelMonoscopeExecute(TObject *Sender)
{
    if ( auto Panel = GetPanel() ) {
        Panel->Monoscope = actPanelMonoscope->Checked;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelMonoscopeUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = GetPanel();
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelClippingExecute(TObject *Sender)
{
    panelClipping_ = !panelClipping_;
    actPanelClipping->Checked = panelClipping_;
    if ( auto Panel = GetPanel() ) {
        Panel->MonitorClipping = PanelClipping;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelClippingUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    auto Panel = GetPanel();
    Act.Enabled = !Panel || !Panel->WindowMode;
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelScalingExecute(TObject *Sender)
{
    panelScaling_ = !panelScaling_;
    actPanelScaling->Checked = panelScaling_;
    if ( TfrmPanelBase* const Panel = GetPanel() ) {
        Panel->MonitorScaling = PanelScaling;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelScalingUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    TfrmPanelBase* const Panel = GetPanel();
    Act.Enabled = !Panel || !Panel->WindowMode;
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelKeepAspectRatioExecute(TObject *Sender)
{
    panelKeepAspectRatio_ = !panelKeepAspectRatio_;
    actPanelKeepAspectRatio->Checked = panelKeepAspectRatio_;
    if ( auto Panel = GetPanel() ) {
        Panel->MaintainAspectRatio = PanelKeepAspectRatio;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelKeepAspectRatioUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    auto Panel = GetPanel();
    Act.Enabled = !Panel || !Panel->WindowMode;
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::SetPanelClipping( bool Val )
{
    actPanelClipping->Checked = Val;
    panelClipping_ = Val;
    if ( auto Panel = GetPanel() ) {
        Panel->MonitorClipping = Val;
    }
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::SetPanelScaling( bool Val )
{
    actPanelScaling->Checked = Val;
    panelScaling_ = Val;
    if ( auto Panel = GetPanel() ) {
        Panel->MonitorScaling = Val;
    }
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::SetPanelKeepAspectRatio( bool Val )
{
    actPanelKeepAspectRatio->Checked = Val;
    panelKeepAspectRatio_ = Val;
    if ( auto Panel = GetPanel() ) {
        Panel->MaintainAspectRatio = Val;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actFileStartExecute(TObject *Sender)
{
    Start();
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actFileStopExecute(TObject *Sender)
{
    Stop();
}
//---------------------------------------------------------------------------


FMXWinDisplayDev const * TfrmPanelAppMain::GetSelectedDisplay() const
{
    int Idx = comboboxPanelTopLeftScreen->ItemIndex;
    if ( Idx >= 0 ) {
        return reinterpret_cast<FMXWinDisplayDev*>(
            comboboxPanelTopLeftScreen->Items->Objects[Idx]
        );
    }
    throw Exception( _D( "No display selected" ) );
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::Start()
{
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::Stop()
{
    actPanelMonoscope->Checked = false;
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelShowExecute(TObject *Sender)
{
    if ( auto Panel = GetPanel() ) {
        Panel->Visible = true;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelShowUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    auto Panel = GetPanel();
    Act.Enabled = Panel && !Panel->Visible;
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelHideExecute(TObject *Sender)
{
    if ( auto Panel = GetPanel() ) {
        Panel->Visible = false;
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelHideUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    auto Panel = GetPanel();
    Act.Enabled = Panel && Panel->Visible;
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::EnabledIfNotRunning(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = !GetPanel();
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::EnabledIfRunning(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = GetPanel();
}
//---------------------------------------------------------------------------

String __fastcall TfrmPanelAppMain::GetInstanceNameEvtHandler( System::TObject* Sender )
{
    return instanceName_;
}
//---------------------------------------------------------------------------

bool __fastcall TfrmPanelAppMain::ReportMessageHandler( System::TObject* Sender,
                                                String Message, bool Error )
{
    TMsgDlgButtons Buttons;

    Buttons = Buttons << TMsgDlgBtn::mbClose;

    int const Ret = MessageDlg(
        Message,
        Error ? TMsgDlgType::mtError : TMsgDlgType::mtInformation,
        Buttons,
        0
    );

    return Ret == mrClose || Ret == mrOk;
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actFileConfigExecute(TObject *Sender)
{
    Config();
}
//---------------------------------------------------------------------------

HWND FindMainWIndow()
{
    HWND hWnd = NULL;
    DWORD Pid = 0;
    DWORD CurrentPid = ::GetCurrentProcessId();

    do {
        hWnd = ::FindWindowEx( NULL, hWnd, _D( "TFMAppClass" ), NULL );
        if( hWnd ) {
            ::GetWindowThreadProcessId( hWnd, &Pid );
            if ( CurrentPid == Pid ) {
                break;
            }
            else {
                ::Sleep( 1 );
            }
        }
    }
    while ( hWnd );
    return hWnd;
}
//---------------------------------------------------------------------------

void HideAppOnTaskbar()
{
    ::ShowWindow( FindMainWIndow(), SW_HIDE );
}
//---------------------------------------------------------------------------

void ShowAppOnTaskbar()
{
    ::ShowWindow( FindMainWIndow(), SW_SHOW );
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::MinimizeToTrayBar()
{
    HideAppOnTaskbar();
    Hide();
    trayIcon_->Visible = true;
}
//---------------------------------------------------------------------------

void TfrmPanelAppMain::RestoreFromTrayBar()
{
    ShowAppOnTaskbar();
    ::SetWindowPos(
        Fmx::Platform::Win::WindowHandleToPlatform( Handle )->Wnd,
        HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW
    );
    trayIcon_->Visible = false;
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::TrayIconDblClick( System::TObject* Sender )
{
    RestoreFromTrayBar();
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::TrayIconRClick( System::TObject* Sender )
{
    TPoint Pt;

    ::GetCursorPos( &Pt );

    // Senn� il menu popup rimane sotto la traybar
    ::SetWindowPos(
        Fmx::Platform::Win::WindowHandleToPlatform( Handle )->Wnd,
        HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE //| SWP_SHOWWINDOW
    );

    popupmenuTrayIcon->Popup( Pt.x, Pt.y );
    popupmenuTrayIcon->BringToFront();
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actViewOpenMainWindowExecute(TObject *Sender)
{
    RestoreFromTrayBar();
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actViewOpenMainWindowUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    Act.Enabled = trayIcon_->Visible;
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelAutoFitExecute(TObject *Sender)
{
    if ( auto Panel = GetPanel() ) {
        Panel->AutoFit();
    }
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actPanelAutoFitUpdate(TObject *Sender)
{
    auto& Act = static_cast<TAction&>( *Sender );
    auto Panel = GetPanel();
    Act.Enabled = Panel && Panel->WindowMode;
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::actFileExitExecute(TObject *Sender)
{
    Close();
}
//---------------------------------------------------------------------------

String TfrmPanelAppMain::CreateInstanceName()
{
    GUID Id;
    CheckOSError( CreateGUID( Id ) );
    return Sysutils::GUIDToString( Id );
}
//---------------------------------------------------------------------------

void __fastcall TfrmPanelAppMain::ActionList1Update(TBasicAction *Action, bool &Handled)
{
    comboboxPanelTopLeftScreen->Enabled = !GetPanel();
}
//---------------------------------------------------------------------------

