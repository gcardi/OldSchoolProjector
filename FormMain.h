//---------------------------------------------------------------------------

#ifndef FormMainH
#define FormMainH

//---------------------------------------------------------------------------

#include <FMX.ActnList.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.Edit.hpp>
#include <FMX.Layouts.hpp>
#include <FMX.ListBox.hpp>
#include <FMX.Menus.hpp>
#include <FMX.Objects.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
#include <System.Actions.hpp>
#include <System.Classes.hpp>
#include "FMXFormAppMain.h"

#include <vector>
#include <memory>

#include "FormPanel.h"

//---------------------------------------------------------------------------

class TfrmMain : public TfrmPanelAppMain
{
__published:	// IDE-managed Components
    TTimer *tmrPolling;
    TMenuItem *MenuItem1;
    TMenuItem *MenuItem2;
    TMenuItem *MenuItem3;
    TMenuItem *MenuItem4;
    TMenuItem *MenuItem5;
    TMenuItem *MenuItem6;
    TMenuItem *MenuItem7;
    TMenuItem *MenuItem9;
    TMenuItem *MenuItem10;
    TMenuItem *MenuItem11;
    TMenuItem *MenuItem12;
    TMenuItem *MenuItem13;
    TMenuItem *MenuItem16;
    TMenuItem *MenuItem17;
    TMenuItem *MenuItem18;
    TMenuItem *MenuItem19;
    TMenuItem *MenuItem34;
    TLine *Line1;
    TMenuBar *MenuBar1;
    TButton *Button4;
    TAction *actPictureNext;
    TButton *Button6;
    TAction *actPicturePrior;
    TSwitch *Switch1;
    TLabel *Label2;
    TAction *actPanelVignetting;
    TTrackBar *TrackBar1;
    void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
    void __fastcall actPictureNextExecute(TObject *Sender);
    void __fastcall actPictureNextUpdate(TObject *Sender);
    void __fastcall actPicturePriorExecute(TObject *Sender);
    void __fastcall actPicturePriorUpdate(TObject *Sender);
    void __fastcall actPanelVignettingExecute(TObject *Sender);
    void __fastcall actPanelVignettingUpdate(TObject *Sender);


private:	// User declarations
    using PanelType = TfrmPanel;

    std::unique_ptr<TfrmPanel> panel_;
    bool panelVignetting_ {};

    void CreatePanel( FMXWinDisplayDev const * Display, bool Clipping,
                      bool Scaling, bool KeepAspectRatio );
    void DestroyPanel();
    void SetPanelVignetting( bool Val );
protected:
    virtual void RestoreProperties();
    virtual void SaveProperties() const;
    virtual void Start();
    virtual void Stop();
    virtual TfrmPanelBase* GetPanel() { return panel_.get(); }
    virtual void Config();

public:		// User declarations
    using inherited = TfrmPanelAppMain;

    __fastcall TfrmMain(TComponent* Owner);
    __fastcall ~TfrmMain();
    __property bool PanelVignetting = {
        read = panelVignetting_, write = SetPanelVignetting
    };
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmMain *frmMain;
//---------------------------------------------------------------------------
#endif
