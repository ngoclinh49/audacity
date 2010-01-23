/**********************************************************************

  Audacity: A Digital Audio Editor

  ProgressDialog.cpp
  
  Leland Lucius

*******************************************************************//**

\file ProgressDialog.cpp

  Implements ProgressDialog

*//*******************************************************************//**

\class ProgressDialog
\brief Custom progress dialog with Audacity specific semantics.

*//**********************************************************************/

#include "../Audacity.h"

#include <wx/defs.h>
#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/datetime.h>
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/intl.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stopwatch.h>
#include <wx/window.h>

#include "ProgressDialog.h"
#include "../Prefs.h"
#include "../ShuttleGui.h"

////////////////////////////////////////////////////////////
/// Methods for ProgressDialog
////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(ProgressDialog, wxDialog)
   EVT_BUTTON(wxID_CANCEL, ProgressDialog::OnCancel)
END_EVENT_TABLE()  

//
// Constructor
//
ProgressDialog::ProgressDialog(const wxString & title, const wxString & message)
: wxDialog(wxGetTopLevelParent(FindFocus()),
           wxID_ANY,
           title,
           wxDefaultPosition,
           wxDefaultSize,
           wxDEFAULT_DIALOG_STYLE |
           wxFRAME_FLOAT_ON_PARENT),
   mDisable(NULL)
{
   wxBoxSizer *v;
   wxWindow *w;
   wxSize ds;

   v = new wxBoxSizer(wxVERTICAL);

   mMessage = new wxStaticText(this,
                               wxID_ANY,
                               message,
                               wxDefaultPosition,
                               wxDefaultSize,                                  
                               wxALIGN_LEFT);
   v->Add(mMessage, 0, wxEXPAND | wxALL, 10);
   ds.y += mMessage->GetSize().y + 20;

   //
   //
   //
   mGauge = new wxGauge(this,
                        wxID_ANY,
                        1000,
                        wxDefaultPosition,
                        wxDefaultSize,
                        wxGA_HORIZONTAL);
   v->Add(mGauge, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
   ds.y += mGauge->GetSize().y + 10;

   //
   //
   //
   wxFlexGridSizer *g = new wxFlexGridSizer(2, 2, 10, 10);

   w = new wxStaticText(this,
                        wxID_ANY,
                        _("Elapsed Time:"),
                        wxDefaultPosition,
                        wxDefaultSize,                                  
                        wxALIGN_RIGHT);
   g->Add(w, 0, wxALIGN_RIGHT);

   mElapsed = new wxStaticText(this,
                               wxID_ANY,
                               wxT("00:00:00"),
                               wxDefaultPosition,
                               wxDefaultSize,                                  
                               wxALIGN_LEFT);
   g->Add(mElapsed, 0, wxALIGN_LEFT);
   ds.y += mElapsed->GetSize().y + 10;
   
   //
   //
   //
   w = new wxStaticText(this,
                        wxID_ANY,
                        _("Remaining Time:"),
                        wxDefaultPosition,
                        wxDefaultSize,                                  
                        wxALIGN_RIGHT);
   g->Add(w, 0, wxALIGN_RIGHT);

   mRemaining = new wxStaticText(this,
                                 wxID_ANY,
                                 wxT("00:00:00"),
                                 wxDefaultPosition,
                                 wxDefaultSize,                                  
                                 wxALIGN_LEFT);
   g->Add(mRemaining, 0, wxALIGN_LEFT);

   v->Add(g, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM, 10);
   ds.y += mRemaining->GetSize().y + 10;

   w = new wxButton(this, wxID_CANCEL, _("Cancel"));
   v->Add(w, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 10);
   ds.y += w->GetSize().y + 10;
   SetSizerAndFit(v);

   wxClientDC dc(this);
   dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
   long widthText = 0;
   dc.GetTextExtent(message, &widthText, NULL, NULL, NULL, NULL);
   ds.x = (wxCoord) wxMax(wxMax(3 * widthText / 2, 4 * ds.y / 3), 300);
   SetClientSize(ds);

   Centre(wxCENTER_FRAME | wxBOTH);

   mStartTime = wxGetLocalTimeMillis().GetValue();
   mLastUpdate = mStartTime;
   mCancel = false;

   Show(false);

   // Even though we won't necessarily show the dialog due to the the 500ms
   // delay, we MUST disable other windows/menus anyway since we run the risk
   // of allowing other tasks to run before this one is complete.
   mDisable = new wxWindowDisabler(this);
}

//
// Destructor
//
ProgressDialog::~ProgressDialog()
{
   if (IsShown())
   {
      Show(false);

      bool bBeepOnCompletion;
      gPrefs->Read(wxT("/GUI/BeepOnCompletion"), &bBeepOnCompletion, false);
      if (bBeepOnCompletion)
      {
         wxBell();
      }
   }

   if (mDisable)
   {
      delete mDisable;
   }
}

//
// Show/Hide the dialog
//
// At least on the Mac, deleting the WindowDisabler before continuing to the
// base class is VERY important since menu items can remain in the disabled
// state.  This has to do with the Mac not honoring the Enable() if the
// application is still in a modal state.
//
// An example is generating a tone in an empty project.  The Export menus
// will not get enabled.
//
bool
ProgressDialog::Show(bool show)
{
   if (!show)
   {
      if (mDisable)
      {
         delete mDisable;
         mDisable = NULL;
      }
   }
   else
   {
      if (!mDisable)
      {
         mDisable = new wxWindowDisabler(this);
      }
   }

    return wxDialog::Show(show);
}

//
// Update the time and, optionally, the message
//
bool
ProgressDialog::Update(int value, const wxString & message)
{
   if (mCancel)
   {
      return false;
   }

   SetMessage(message);

   if (value <= 0)
   {
      value = 1;
   }

   if (value > 1000)
   {
      value = 1000;
   }

   wxLongLong_t now = wxGetLocalTimeMillis().GetValue();
   wxLongLong_t elapsed = now - mStartTime;
   wxLongLong_t estimate = elapsed * 1000ll / value;
   wxLongLong_t remains = (estimate + mStartTime) - now;

   if (!IsShown() && elapsed > 500)
   {
      Show(true);
   }

   if (value != mLastValue)
   {
      mGauge->SetValue(value);
      mLastValue = value;
   }

   // Only update if a full second has passed.
   if (now - mLastUpdate > 1000)
   {
      wxTimeSpan tsElapsed(0, 0, 0, elapsed);
      wxTimeSpan tsRemains(0, 0, 0, remains);

      mElapsed->SetLabel(tsElapsed.Format(wxT("%H:%M:%S")));
      mRemaining->SetLabel(tsRemains.Format(wxT("%H:%M:%S")));

      mLastUpdate = now;
   }

   wxYieldIfNeeded();

   return true;
}

//
// Update the time and, optionally, the message
//
bool
ProgressDialog::Update(wxULongLong_t current, wxULongLong_t total, const wxString & message)
{
   return Update((int)(current * 1000 / total), message);
}

//
// Update the time and, optionally, the message
//
bool
ProgressDialog::Update(wxLongLong current, wxLongLong total, const wxString & message)
{
   return Update((int)(current.GetValue() * 1000ll / total.GetValue()), message);
}

//
// Update the time and, optionally, the message
//
bool
ProgressDialog::Update(int current, int total, const wxString & message)
{
   return Update((int)(current *  ((double)(1000.0 / total))), message);
}

//
// Update the time and, optionally, the message
//
bool
ProgressDialog::Update(double current, double total, const wxString & message)
{
   return Update((int)(current * 1000.0 / total), message);
}

//
// Update the message text
//
void
ProgressDialog::SetMessage(const wxString & message)
{
   if (!message.IsEmpty())
   {
      mMessage->SetLabel(message);
      wxYieldIfNeeded();
   }
}

void
ProgressDialog::OnCancel(wxCommandEvent & e)
{
   FindWindowById(wxID_CANCEL, this)->Disable();
   mCancel = true;
}

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
