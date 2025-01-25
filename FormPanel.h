//---------------------------------------------------------------------------

#ifndef FormPanelH
#define FormPanelH
//---------------------------------------------------------------------------

#include <FMX.Controls.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.Layouts.hpp>
#include <FMX.Objects.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
#include <System.Classes.hpp>
#include "FMXFormPanelBase.h"
#include <FMX.Ani.hpp>
#include <FMX.ExtCtrls.hpp>
#include <FMX.Effects.hpp>
#include <FMX.Filter.Effects.hpp>

#include <memory>

#include "WavePlayer.h"

//---------------------------------------------------------------------------

class TfrmPanel : public TfrmPanelBase
{
__published:	// IDE-managed Components
    TLayout *Layout1;
    TImageViewer *ImageViewer2;
    TFloatAnimation *FloatAnimation2;
    TRectangle *Rectangle1;
    TFloatAnimation *FloatAnimation1;
    TLayout *Layout2;
    TNormalBlendEffect *NormalBlendEffect1;
    void __fastcall FloatAnimation2Finish(TObject *Sender);
private:	// User declarations
    using ImageFileNameCont = std::vector<String>;

    std::unique_ptr<WavePlayer> player_;
    size_t phase_ {};
    ImageFileNameCont entries_;
    size_t idx_ {};
    bool backward_ {};
    int mechSoundVolume_ { 50 };
    String picturesPath_;
    bool recursivePicturesSearch_ {};

    void RestoreProperties();
    void SaveProperties() const;
    void LoadImage( size_t Index );
    ImageFileNameCont& GetImages();
    bool GetVignetting() const;
    void SetVignetting( bool Val );
    void PlayMechanicalSound();
    void SetMechSoundVolume( int Val );
    void LoadWave();
    void LoadPictures();
public:		// User declarations
    using inherited = TfrmPanelBase;

    __fastcall TfrmPanel( TComponent* Owner, int MechSoundVolume,
                          String PicturesPath,
                          bool RecursivePicturesSearch,
                          FMXWinDisplayDev const * Display,
                          StoreOpts StoreOptions,
                          Anafestica::TConfigNode* const RootNode = nullptr );
    __fastcall ~TfrmPanel();
    void Next();
    void Prior();
    __property ImageFileNameCont& Images = { read = GetImages };
    bool IsIdle() const;
    __property bool Vignetting = { read = GetVignetting, write = SetVignetting };
    __property int MechSoundVolume = {
        read = mechSoundVolume_, write = SetMechSoundVolume
    };
};
//---------------------------------------------------------------------------
//extern PACKAGE TfrmPanel *frmPanel;
//---------------------------------------------------------------------------
#endif
