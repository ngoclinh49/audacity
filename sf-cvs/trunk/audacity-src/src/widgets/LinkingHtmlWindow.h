/**********************************************************************

  Audacity: A Digital Audio Editor

  LinkingHtmlWindow.h

  Vaughan Johnson
  Dominic Mazzoni

  utility fn and 
  descendant of wxHtmlWindow that opens links in the user's 
  default browser
  
**********************************************************************/

#ifndef __AUDACITY_LINKINGHTMLWINDOW__
#define __AUDACITY_LINKINGHTMLWINDOW__

#include <wx/dialog.h>
#include <wx/html/htmlwin.h>
#include <wx/frame.h>

void OpenInDefaultBrowser(const wxHtmlLinkInfo& link);

class LinkingHtmlWindow : public wxHtmlWindow 
{
 public:
   LinkingHtmlWindow(wxWindow *parent, wxWindowID id = -1, 
                     const wxPoint& pos = wxDefaultPosition, 
                     const wxSize& size = wxDefaultSize, 
                     long style = wxHW_SCROLLBAR_AUTO);
   virtual void OnLinkClicked(const wxHtmlLinkInfo& link);
   //virtual void OnSetTitle(const wxString& title);

};

class BrowserFrame : public wxFrame
{
public:

   void OnForward(wxCommandEvent & event);
   void OnBackward(wxCommandEvent & event);
   void OnClose(wxCommandEvent & event);

   wxHtmlWindow * mpHtml;
   DECLARE_EVENT_TABLE()
};



#endif // __AUDACITY_LINKINGHTMLWINDOW__
