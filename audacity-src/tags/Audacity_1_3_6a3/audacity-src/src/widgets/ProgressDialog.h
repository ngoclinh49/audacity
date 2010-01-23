/**********************************************************************

  Audacity: A Digital Audio Editor

  ProgressDialog.h
  
  Leland Lucius

*************************************************************************/

#ifndef __AUDACITY_WIDGETS_PROGRESSDIALOG__
#define __AUDACITY_WIDGETS_PROGRESSDIALOG__

#include "../Audacity.h"

#include <wx/defs.h>
#include <wx/dialog.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/utils.h>

////////////////////////////////////////////////////////////
/// ProgressDialog Class
////////////////////////////////////////////////////////////

class ProgressDialog:public wxDialog
{

 public:

   ProgressDialog(const wxString & title, const wxString & message = wxEmptyString);
   virtual ~ProgressDialog();

   bool Show(bool show = true);

   bool Update(int value, const wxString & message = wxEmptyString);
   bool Update(double current, double total, const wxString & message = wxEmptyString);
   bool Update(wxULongLong_t current, wxULongLong_t total, const wxString & message = wxEmptyString);
   bool Update(wxLongLong current, wxLongLong total, const wxString & message = wxEmptyString);
   bool Update(int current, int total, const wxString & message = wxEmptyString);
   void SetMessage(const wxString & message);

 private:
   void OnCancel(wxCommandEvent & e);

 private:
   wxGauge *mGauge;
   wxStaticText *mMessage;
   wxStaticText *mElapsed;
   wxStaticText *mRemaining;
   wxWindowDisabler *mDisable;

   wxLongLong_t mStartTime;
   wxLongLong_t mLastTime;
   wxLongLong_t mLastUpdate;
   int mLastValue;

   bool mCancel;
   
   DECLARE_EVENT_TABLE();
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
// arch-tag: 2f4ec75c-bdb7-4889-96d1-5d00abc41027
