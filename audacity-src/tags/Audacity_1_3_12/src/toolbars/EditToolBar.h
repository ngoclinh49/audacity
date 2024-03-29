/**********************************************************************

  Audacity: A Digital Audio Editor

  
  EditToolbar.h
 
  Dominic Mazzoni
  Shane T. Mueller
  Leland Lucius

**********************************************************************/

#ifndef __AUDACITY_EDIT_TOOLBAR__
#define __AUDACITY_EDIT_TOOLBAR__

#include <wx/defs.h>

#include "ToolBar.h"
#include "../Theme.h"
#include "../Experimental.h"

class wxCommandEvent;
class wxDC;
class wxImage;
class wxWindow;

class AButton;

enum {
   ETBCutID,
   ETBCopyID,
   ETBPasteID,
   ETBTrimID,
   ETBSilenceID,

   ETBUndoID,
   ETBRedoID,

#ifdef EXPERIMENTAL_LINKING
   ETBLinkID,
#endif

   ETBZoomInID,
   ETBZoomOutID,

   #if 0 // Disabled for version 1.2.0 since it doesn't work quite right...
   ETBZoomToggleID,
   #endif

   ETBZoomSelID,
   ETBZoomFitID,

   ETBNumButtons
};

class EditToolBar:public ToolBar {

 public:

   EditToolBar();
   virtual ~EditToolBar();

   void Create(wxWindow *parent);

   void OnButton(wxCommandEvent & event);

   void Populate();
   void Repaint(wxDC *dc) {};
   void EnableDisableButtons();
   void UpdatePrefs();

 private:

   AButton *AddButton(teBmps eFore, teBmps eDisabled,
      int id, const wxChar *label, bool toggle = false);

   void AddSeparator();

   void MakeButtons();

   void RegenerateTooltips();
   
   AButton *mButtons[ETBNumButtons];

   wxImage *upImage;
   wxImage *downImage;
   wxImage *hiliteImage;

 public:

   DECLARE_CLASS(EditToolBar);
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
// arch-tag: b06d2006-b412-41e0-90fe-fa8330420e0d

