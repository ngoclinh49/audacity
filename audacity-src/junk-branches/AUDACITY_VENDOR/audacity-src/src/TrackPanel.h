/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackPanel.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_TRACK_PANEL__
#define __AUDACITY_TRACK_PANEL__

#include <wx/timer.h>
#include <wx/window.h>

#include "ViewInfo.h"

class wxMenu;
class wxRect;
class wxStatusBar;

class TrackList;
class VTrack;
class TrackPanel;
class TrackArtist;

class AudacityTimer:public wxTimer {
 public:
   virtual void Notify();
   TrackPanel *parent;
};

class TrackPanelListener {
 public:
   virtual void TP_DisplayStatusMessage(const char *msg, int fieldNum) = 0;
   virtual int TP_GetCurrentTool() = 0;
   virtual void TP_OnPlayKey() = 0;
   virtual void TP_PushState(wxString desc) = 0;
   virtual void TP_RedrawScrollbars() = 0;
   virtual void TP_ScrollLeft() = 0;
   virtual void TP_ScrollRight() = 0;
   virtual void TP_HasMouse() = 0;
};

class TrackPanel:public wxWindow {
 public:

   TrackPanel(wxWindow * parent,
              wxWindowID id,
              const wxPoint & pos,
              const wxSize & size,
              TrackList * tracks,
              ViewInfo * viewInfo, TrackPanelListener * listener);

   virtual ~ TrackPanel();

   void OnPaint(wxPaintEvent & event);
   void OnMouseEvent(wxMouseEvent & event);
   void OnKeyEvent(wxKeyEvent & event);

   void OnTimer();

   int GetRulerHeight();
   int GetLeftOffset();

   void GetTracksUsableArea(int *width, int *height);

   void SelectNone();

 private:

   void HandleCursor(wxMouseEvent & event);
   void HandleResize(wxMouseEvent & event);
   void HandleSelect(wxMouseEvent & event);
   void HandleEnvelope(wxMouseEvent & event);
   void HandleSlide(wxMouseEvent & event);
   void HandleZoom(wxMouseEvent & event);
   void HandleLabelClick(wxMouseEvent & event);
   void HandleClosing(wxMouseEvent & event);
   void HandleMuting(wxMouseEvent & event);
   void HandleSoloing(wxMouseEvent & event);

   void MakeParentRedrawScrollbars();
   void MakeParentPushState(wxString desc);

   void OnSetName();

   void OnMoveUp();
   void OnMoveDown();

   void OnUpOctave();
   void OnDownOctave();

   void OnChannelLeft();
   void OnChannelRight();
   void OnChannelMono();

   void OnWaveform();
   void OnWaveformDB();
   void OnSpectrum();
   void OnPitch();

   void OnRate8();
   void OnRate11();
   void OnRate16();
   void OnRate22();
   void OnRate44();
   void OnRate48();
   void OnRateOther();

   void OnSplitStereo();
   void OnMergeStereo();

   void RemoveTrack(VTrack * toRemove);

   // Find track info by coordinate
   VTrack *FindTrack(int mouseX, int mouseY, bool label,
                     wxRect * trackRect = NULL, int *trackNum = NULL);

   int GetTitleWidth();
   int GetTitleOffset();
   int GetVRulerWidth();
   int GetVRulerOffset();
   int GetLabelWidth();

   void SetLabelFont(wxDC * dc);

   void DrawRuler(wxDC * dc, bool text = true);
   void DrawTracks(wxDC * dc);

   void GetTrackControlsRect(wxRect & trackRect, wxRect & r);
   void GetCloseBoxRect(wxRect & trackRect, wxRect & r);
   void GetTitleBarRect(wxRect & trackRect, wxRect & r);
   void GetMuteRect(wxRect & trackRect, wxRect & r);
   void GetSoloRect(wxRect & trackRect, wxRect & r);

   void DrawCloseBox(wxDC * dc, wxRect & r, bool down);
   void DrawTitleBar(wxDC * dc, wxRect & r, VTrack * t, bool down);
   void DrawMute(wxDC * dc, wxRect & r, VTrack * t, bool down);
   void DrawSolo(wxDC * dc, wxRect & r, VTrack * t, bool down);

   void DrawVRuler(wxDC * dc, wxRect & r, VTrack * t);

   TrackPanelListener *mListener;

   TrackList *mTracks;
   ViewInfo *mViewInfo;
   wxStatusBar *mStatusBar;

   TrackArtist *mTrackArtist;

   AudacityTimer mTimer;
   int mTimeCount;

   wxBitmap *mBitmap;
   int mPrevWidth;
   int mPrevHeight;

   double mSelStart;

   VTrack *mCapturedTrack;
   wxRect mCapturedRect;
   int mCapturedNum;

   bool mIndicatorShowing;

   int mMouseClickX;
   int mMouseClickY;

   int mMouseMostRecentX;
   int mMouseMostRecentY;

   int mZoomStart;
   int mZoomEnd;

   int mInitialTrackHeight;

   bool mAutoScrolling;

   bool mIsClosing;
   bool mIsSelecting;
   bool mIsResizing;
   bool mIsSliding;
   bool mIsEnveloping;
   bool mIsZooming;
   bool mIsMuting;
   bool mIsSoloing;

   wxCursor *mArrowCursor;
   wxCursor *mSelectCursor;
   wxCursor *mResizeCursor;
   wxCursor *mSlideCursor;
   wxCursor *mZoomInCursor;
   wxCursor *mZoomOutCursor;

   wxMenu *mWaveTrackMenu;
   wxMenu *mNoteTrackMenu;
   wxMenu *mLabelTrackMenu;
   wxMenu *mRateMenu;

   VTrack *mPopupMenuTarget;

 public:

   DECLARE_EVENT_TABLE()
};

#endif
