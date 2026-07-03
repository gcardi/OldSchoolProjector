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
#include <FMX.Dialogs.hpp>
#include "FMX.SVGIconImage.hpp"
#include "FMX.SVGIconImageList.hpp"
#include <FMX.ImgList.hpp>
#include <System.ImageList.hpp>

#include <vector>
#include <memory>

#include "FormPanel.h"
#include "ThumbnailStrip.h"

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
    TAction *actPictureNext;
    TAction *actPicturePrior;
    TSwitch *switchVignetting1;
    TLabel *Label2;
    TAction *actPanelVignetting;
    TTrackBar *trackbarMechSndVol;
    TLabel *Label4;
    TEdit *edtPicturesPath;
    TLabel *Label5;
    TSwitch *switchRecursivePicturesSearch1;
    TLabel *Label6;
    TButton *Button9;
    TAction *actPanelRecursivePicturesSearch;
    TAction *actFileBrowsePicturesPath;
    TMenuItem *MenuItem14;
    TMenuItem *MenuItem15;
    TMenuItem *MenuItem20;
    TMenuItem *MenuItem21;
    TMenuItem *MenuItem22;
    TTrackBar *trackbarNoiseSndVol;
    TLabel *Label7;
    TTimer *tmrChangeSoundVol;
    TSwitch *switchFanNoise;
    TLabel *Label10;
    TAction *actPanelFanNoise;
    TMenuItem *MenuItem38;
    TLabel *lblFileName;
    TLabel *lblPictureInfo;
    TButton *Button4;
    TSVGIconImageList *SVGIconImageList1;
    TButton *Button6;
    TThumbnailStrip *frameThumbs;
    void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
    void __fastcall actPictureNextExecute(TObject *Sender);
    void __fastcall actPicturePriorExecute(TObject *Sender);
    void __fastcall actPanelVignettingExecute(TObject *Sender);
    void __fastcall actPanelVignettingUpdate(TObject *Sender);
    void __fastcall trackbarMechSndVolChange(TObject *Sender);
    void __fastcall actPanelRecursivePicturesSearchExecute(TObject *Sender);
    void __fastcall actPanelRecursivePicturesSearchUpdate(TObject *Sender);
    void __fastcall actFileBrowsePicturesPathUpdate(TObject *Sender);
    void __fastcall actFileBrowsePicturesPathExecute(TObject *Sender);
    void __fastcall trackbarNoiseSndVolChange(TObject *Sender);
    void __fastcall tmrChangeSoundVolTimer(TObject *Sender);
    void __fastcall actPanelFanNoiseExecute(TObject *Sender);
    void __fastcall actPanelFanNoiseUpdate(TObject *Sender);
    void __fastcall actPictureChangeUpdate(TObject *Sender);
    void __fastcall Button4Paint(TObject *Sender, TCanvas *Canvas, const TRectF &ARect);




private:	// User declarations
    using PanelType = TfrmPanel;
    using ImageFileNameCont = std::vector<String>;

    std::unique_ptr<TfrmPanel> panel_;
    bool vignetting_ {};
    String picturesPath_;
    ImageFileNameCont entries_;
    size_t idx_ {};

    void CreatePanel( FMXWinDisplayDev const * Display, bool Clipping,
                      bool Scaling, bool KeepAspectRatio );
    void DestroyPanel();
    void SetVignetting( bool Val );
    void IdleEvent( TObject* Sender, bool &Done );
    TfrmPanel* GetProjectorPanel() {
        return static_cast<TfrmPanel*>( GetPanel() );
    }
    int GetMechanicalSoundVolume() const;
    void SetMechanicalSoundVolume( int Val );
    int GetNoiseSoundVolume() const;
    void SetNoiseSoundVolume( int Val );

    __property TfrmPanel* ProjectorPanel = { read = GetProjectorPanel };

    String GetPicturesPath() const;
    void SetPicturesPath( String Val );
    bool GetRecursivePicturesSearch() const;
    void SetRecursivePicturesSearch( bool Val );
    bool GetFanNoise() const;
    void SetFanNoise( bool Val );
    void __fastcall OnLoadPicture( TObject *Sender, bool Backward );

    // Thumbnail strip callbacks (the frame is filesystem/cache agnostic).
    void __fastcall ThumbRequest( TObject* Sender, int Index, TBitmap*& Bmp );
    void __fastcall ThumbPick( TObject* Sender, int Index );
    void __fastcall ThumbVisibleRange( TObject* Sender, int First, int Last );

    void LoadPicture( size_t Idx );
    void LoadPictures();
    void ShowFileInfo( size_t Idx );
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
    __property bool Vignetting = {
        read = vignetting_, write = SetVignetting
    };
    __property int MechanicalSoundVolume = {
        read = GetMechanicalSoundVolume, write = SetMechanicalSoundVolume
    };
    __property int NoiseSoundVolume = {
        read = GetNoiseSoundVolume, write = SetNoiseSoundVolume
    };
    __property String PicturesPath = {
        read = GetPicturesPath, write = SetPicturesPath
    };
    __property bool RecursivePicturesSearch = {
        read = GetRecursivePicturesSearch, write = SetRecursivePicturesSearch
    };
    __property bool FanNoise = { read = GetFanNoise, write = SetFanNoise };
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmMain *frmMain;
//---------------------------------------------------------------------------
#endif
