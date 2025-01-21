//---------------------------------------------------------------------------

#include <fmx.h>
#pragma hdrstop

#include <algorithm>
#include <memory>

#include <cassert>

#include "FormPanel.h"
#include "DataModStyleRes.h"

using std::make_unique;
using std::swap;

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "FMXFormPanelBase"
#pragma link "FMXFormPanelBase"
#pragma resource "*.fmx"
//TfrmPanel *frmPanel;
//---------------------------------------------------------------------------

/*
__fastcall TfrmPanel::TfrmPanel( TComponent* Owner,
                                 FMXWinDisplayDev const * Display,
                                 Anafestica::TConfigNode* const RootNode )
    : TfrmPanel( Owner, Display, StoreOpts::All, RootNode )
{
}
*/
//---------------------------------------------------------------------------

__fastcall TfrmPanel::TfrmPanel( TComponent* Owner,
                                 FMXWinDisplayDev const * Display,
                                 StoreOpts StoreOptions,
                                 Anafestica::TConfigNode* const RootNode )
    : TfrmPanelBase( Owner, Display, StoreOptions, RootNode )
{
    RestoreProperties();
}
//---------------------------------------------------------------------------

__fastcall TfrmPanel::~TfrmPanel()
{
    try {
        SaveProperties();
    }
    catch ( ... ) {
    }
}
//---------------------------------------------------------------------------

void TfrmPanel::RestoreProperties()
{
    // Put code here to restore attribute(s)
}
//---------------------------------------------------------------------------

void TfrmPanel::SaveProperties() const
{
    // Put code here to save attribute(s)
}
//---------------------------------------------------------------------------


