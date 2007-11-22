/**********************************************************************

  Audacity: A Digital Audio Editor

  GUIPrefs.h

  Brian Gunlogson
  Joshua Haberman
  James Crook

**********************************************************************/

#ifndef __AUDACITY_GUI_PREFS__
#define __AUDACITY_GUI_PREFS__

#include <wx/defs.h>
#include <wx/string.h>

#include "PrefsPanel.h"

class wxWindow;
class ShuttleGui;

class GUIPrefs : public PrefsPanel 
{
public:
   GUIPrefs(wxWindow * parent);
   ~GUIPrefs();
   virtual bool Apply();

private:
   void Populate();
   void PopulateOrExchange( ShuttleGui & S );

   wxArrayString mLangCodes;
   wxArrayString mLangNames;

#ifdef EXPERIMENTAL_SAVE_DEFAULT_VIEW
   int mDefaultViewMode;
#endif //EXPERIMENTAL_SAVE_DEFAULT_VIEW
};

#endif

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: 57018e2b-d264-4f93-bfa7-06752ebf631e

