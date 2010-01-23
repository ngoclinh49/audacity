/**********************************************************************

  Audacity: A Digital Audio Editor

  KeyConfigPrefs.h

  Brian Gunlogson

**********************************************************************/

#ifndef __AUDACITY_KEY_CONFIG_PREFS__
#define __AUDACITY_KEY_CONFIG_PREFS__

#include "PrefsPanel.h"
#include "../Project.h"

class wxChoice;
class wxCommandEvent;
class wxStaticText;
class wxListCtrl;
class wxListEvent;
class wxWindow;

class KeyConfigPrefs:public PrefsPanel {

 public:
   KeyConfigPrefs(wxWindow * parent);
   ~KeyConfigPrefs();
   bool Apply();

 private:
   void OnItemSelected(wxListEvent &event);

   wxListCtrl *mCommandsList;
   wxListCtrl *mKeysList;

   int mCommandSelected;

   AudacityProject *mAudacity;

 public:
   DECLARE_EVENT_TABLE();
};

#endif
