//---------------------------------------------------------------------------

#ifndef FormMainH
#define FormMainH

//---------------------------------------------------------------------------

#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.ExtCtrls.hpp>
#include <FMX.Layouts.hpp>
#include <FMX.Types.hpp>
#include <FMX.Ani.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Effects.hpp>
#include <FMX.Filter.Effects.hpp>
#include <FMX.Objects.hpp>
#include <FMX.ActnList.hpp>
#include <System.Actions.hpp>

#include <vector>

//---------------------------------------------------------------------------

class TForm1 : public TForm
{
__published:	// IDE-managed Components
    TButton *Button1;
    TImageViewer *ImageViewer2;
    TFloatAnimation *FloatAnimation2;
    TLayout *Layout1;
    TRectangle *Rectangle1;
    TFloatAnimation *FloatAnimation1;
    TStyleBook *StyleBook1;
    TActionList *ActionList1;
    TAction *actNext;
    TTrackBar *tbarVolume;
    TLabel *Label1;
    TTimer *Timer1;
    void __fastcall FloatAnimation2Finish(TObject *Sender);
    void __fastcall actNextExecute(TObject *Sender);
    void __fastcall EnabledIfThereArePictures(TObject *Sender);
    void __fastcall tbarVolumeChange(TObject *Sender);
    void __fastcall Timer1Timer(TObject *Sender);
private:	// User declarations
    std::vector<String> entries_;
    size_t idx_ {};
    std::vector<int8_t> sound_;
    int volume_ { 100 };
    size_t phase_ {};

    void __fastcall IdleEvent( TObject* Sender, bool &Done );
    void LoadImage( size_t Index );
    void BuildSuono( float Volume = 1.0F );
    void PlaySuono() const;
    void ReadWaveFromResource( String Name, float Volume, String Type = _D( "WAVE" ) );
    float GetVolumeSuono() const;
    int GetVolume() const;
    void SetVolume( int Volume );
    bool IsImageChanging() const;
public:		// User declarations
    __fastcall TForm1(TComponent* Owner);
    __property int Volume = { read = GetVolume, write = SetVolume };
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
