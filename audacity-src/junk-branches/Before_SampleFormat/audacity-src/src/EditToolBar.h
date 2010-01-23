/**********************************************************************

  Audacity: A Digital Audio Editor

  
  EditToolbar.h
 
  Dominic Mazzoni
  Shane T. Mueller
 
  This class, which is a child of Toolbar, creates the
  window containing interfaces to commonly-used edit
  functions that are otherwise only available through
  menus. The window can be embedded within a normal project
  window, or within a ToolbarFrame that is managed by a
  global ToolBarStub called gControlToolBarStub.

  All of the controls in this window were custom-written for
  Audacity - they are not native controls on any platform -
  however, it is intended that the images could be easily
  replaced to allow "skinning" or just customization to
  match the look and feel of each platform.

**********************************************************************/

#ifndef __AUDACITY_EDIT_TOOLBAR__
#define __AUDACITY_EDIT_TOOLBAR__

#include <wx/brush.h>
#include <wx/pen.h>
#include <wx/minifram.h>
#include <wx/object.h>

#include "ToolBar.h"


class AButton;
class ASlider;
class EditToolBar;
class ToolBar;
class ToolBarFrame;


class wxImage;
class wxSize;
class wxPoint;

enum {
   ETBCopyID,
   ETBCutID,
   ETBPasteID,
   ETBTrimID,
   ETBSilenceID,

   ETBUndoID,
   ETBRedoID,
   
   ETBZoomInID,
   ETBZoomOutID,
   ETBZoomSelID,
   ETBZoomFitID,

};



class EditToolBar:public ToolBar {

   DECLARE_DYNAMIC_CLASS(EditToolBar)

 public:
   EditToolBar() {
   };
   EditToolBar(wxWindow * parent, wxWindowID id,
                  const wxPoint & pos, const wxSize & size);
   EditToolBar(wxWindow * parent);
   virtual ~ EditToolBar();
   void InitializeEditToolBar();

   virtual void OnPaint(wxPaintEvent & event);
   virtual void OnKeyEvent(wxKeyEvent & event);


   void OnCopy();
   void OnCut();
   void OnPaste();
   void OnTrim();
   void OnSilence();
   
   void OnUndo();
   void OnRedo();
   
   void OnZoomIn();
   void OnZoomOut();
   void OnZoomSel();
   void OnZoomFit();



 private:


   void MakeButtons();
   AButton * MakeButton(wxImage* up, wxImage* down, wxImage* hilite,
                        char const ** foreground,
                        char const ** alpha,
                        int id, int left);
   AButton *mCopy;
   AButton *mCut;
   AButton *mPaste;
   AButton *mTrim;
   AButton *mSilence;
   
   AButton *mUndo;
   AButton *mRedo;

   AButton *mZoomIn;
   AButton *mZoomOut;
   AButton *mZoomSel;
   AButton *mZoomFit;

   wxBitmap *mBackgroundBitmap;
   int mBackgroundWidth;
   int mBackgroundHeight;

   DECLARE_EVENT_TABLE()
};

#endif
