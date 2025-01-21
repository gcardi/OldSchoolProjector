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

//---------------------------------------------------------------------------

using TNumEvent =
    void __fastcall (__closure *)( System::TObject* Sender, int Num, bool Val );

class TfrmPanel : public TfrmPanelBase
{
__published:	// IDE-managed Components
private:	// User declarations
    //static constexpr auto TabsNodeName = _D( "Tabs" );

    void RestoreProperties();
    void SaveProperties() const;

public:		// User declarations
    using inherited = TfrmPanelBase;

    __fastcall TfrmPanel( TComponent* Owner,
                          FMXWinDisplayDev const * Display,
                          StoreOpts StoreOptions,
                          Anafestica::TConfigNode* const RootNode = nullptr );
    __fastcall ~TfrmPanel();

};
//---------------------------------------------------------------------------
//extern PACKAGE TfrmPanel *frmPanel;
//---------------------------------------------------------------------------
#endif
