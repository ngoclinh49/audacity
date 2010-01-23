/**********************************************************************

  Audacity: A Digital Audio Editor

  GUIPrefs.cpp

  Brian Gunlogson
  Joshua Haberman

**********************************************************************/



#include <wx/defs.h>
#include <wx/checkbox.h>
#include <wx/intl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "GUIPrefs.h"
#include "../Prefs.h"
#include "../Audacity.h"
#include "../EditToolBar.h"


GUIPrefs::GUIPrefs(wxWindow * parent):
PrefsPanel(parent)
{
   // Scrolling
   bool autoscroll, spectrogram, editToolBar;
   gPrefs->Read("/GUI/AutoScroll", &autoscroll, false);
   gPrefs->Read("/GUI/UpdateSpectrogram", &spectrogram, true);
   gPrefs->Read("/GUI/EnableEditToolBar", &editToolBar, true);

   topSizer = new wxBoxSizer( wxVERTICAL );
   
   //Auto-scrolling preference
   mAutoscroll = new wxCheckBox(this, -1, _("Autoscroll while playing"));
   mAutoscroll->SetValue(autoscroll);
   topSizer->Add(mAutoscroll, 0, wxGROW|wxALL, 2);

   //Spectrogram preference
   mSpectrogram = new wxCheckBox(this, -1, _("Update spectrogram while playing"));
   mSpectrogram->SetValue(spectrogram);
   topSizer->Add(mSpectrogram, 0, wxGROW|wxALL, 2);

   //Enable Edit toolbar preference

   mEditToolBar = new wxCheckBox(this, -1, _("Enable Edit Toolbar"));
   mEditToolBar->SetValue(editToolBar);
   topSizer->Add(mEditToolBar, 0, wxGROW|wxALL, 2);


   // Locale
   wxString lang = gPrefs->Read("/Locale/Language", "en");
   mLocale = new wxTextCtrl(this, -1, lang);
   mLocaleLabel = new wxStaticText(this, -1, _("Language:"));

   wxFlexGridSizer *localeSizer = new wxFlexGridSizer( 0, 2, 0, 0 );
   localeSizer->Add(mLocaleLabel, 0, wxALIGN_LEFT|wxALL|wxALIGN_CENTER_VERTICAL, 2);
   localeSizer->Add(mLocale, 1, wxGROW|wxALL|wxALIGN_CENTER_VERTICAL, 2 );
   topSizer->Add(localeSizer, 0, wxGROW|wxALL, 2);
   
   // Finish layout
   outSizer = new wxBoxSizer( wxVERTICAL );
   outSizer->Add(topSizer, 0, wxGROW|wxALL, TOP_LEVEL_BORDER);

   SetAutoLayout(true);
   SetSizer(outSizer);

   outSizer->Fit(this);
   outSizer->SetSizeHints(this);
}

bool GUIPrefs::Apply()
{
   gPrefs->Write("/GUI/AutoScroll", mAutoscroll->GetValue());
   gPrefs->Write("/GUI/UpdateSpectrogram", mSpectrogram->GetValue());

   //-------------------------------------------------------------
   //---------------------- Edit toolbar loading/unloading
   gPrefs->Write("/GUI/EnableEditToolBar", mEditToolBar->GetValue());
   bool enable = mEditToolBar->GetValue();
 
   if(gEditToolBarStub && !enable) {
      gEditToolBarStub->UnloadAll();    //Unload all docked EDIT toolbars
      delete gEditToolBarStub;
      gEditToolBarStub = NULL;
   }
   else if(!gEditToolBarStub && enable)
      {  
         gEditToolBarStub =  new ToolBarStub(gParentWindow, EditToolBarID);
         gEditToolBarStub->LoadAll();
      }
   //----------------------End of Edit toolbar loading/unloading
   //---------------------------------------------------------------

   gPrefs->Write("/Locale/Language", mLocale->GetValue());


   return true;
}

GUIPrefs::~GUIPrefs()
{
}
