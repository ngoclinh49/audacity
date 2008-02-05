/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackPanel.cpp

  Dominic Mazzoni
  and lots of other contributors

  Implements TrackPanel and TrackInfo.

********************************************************************//*! 

\todo
  Refactoring of the TrackPanel, possibly as described 
  in \ref TrackPanelRefactor

*//*****************************************************************//*!

\file TrackPanel.cpp
\brief 
  Implements TrackPanel and TrackInfo.

  TrackPanel.cpp is currently some of the worst code in Audacity.  
  It's not really unreadable, there's just way too much stuff in this
  one file.  Rather than apply a quick fix, the long-term plan
  is to create a GUITrack class that knows how to draw itself
  and handle events.  Then this class just helps coordinate
  between tracks.

  Plans under discussion are described in \ref TrackPanelRefactor

*//********************************************************************/

// Documentation: Rather than have a lengthy \todo section, having 
// a \todo a \file and a \page in EXACTLY that order gets Doxygen to 
// put the following lengthy description of refactoring on a new page 
// and link to it from the docs.

/*****************************************************************//**

\class TrackPanel
\brief
  The TrackPanel class coordinates updates and operations on the
  main part of the screen which contains multiple tracks.

  It uses many other classes, but in particular it uses the
  TrackInfo class to draw the label on the left of a track,
  and the TrackArtist class to draw the actual waveforms.

  The TrackPanel manages multiple tracks and their TrackInfos.

  Note that with stereo tracks there will be one TrackInfo
  being used by two wavetracks.

*//*****************************************************************//**

\class TrackInfo
\brief
  The TrackInfo is shown to the side of a track 
  It has the menus, pan and gain controls displayed in it.
 
  TrackPanel and not TrackInfo takes care of the functionality for 
  each of the buttons in that panel.

  In its current implementation TrackInfo is not derived from a
  wxWindow.  Following the original coding style, it has 
  been coded as a 'flyweight' class, which is passed 
  state as needed, except for the array of gains and pans.
 
  If we'd instead coded it as a wxWindow, we would have an instance
  of this class for each instance displayed.

*//**************************************************************//**

\class TrackClip
\brief One clip (i.e short section) of a WaveTrack.

*//**************************************************************//**

\class TrackPanelListener
\brief A now badly named class which is used to give access to a
subset of the TrackPanel functions from all over the place.

*//**************************************************************//**

\class TrackList
\brief A list of TrackListNode items.

*//**************************************************************//**

\class TrackListIterator
\brief An iterator for a TrackList.

*//**************************************************************//**

\class TrackListNode
\brief Used by TrackList, points to a Track.

*//**************************************************************//**

\class TrackPanel::AudacityTimer
\brief Timer class dedicated to infomring the TrackPanel that it 
is time to refresh some aspect of the screen.

*//*****************************************************************//**

\page TrackPanelRefactor Track Panel Refactor
\brief Planned refactoring of TrackPanel.cpp

 - Move menus from current TrackPanel into TrackInfo.
 - Convert TrackInfo from 'flyweight' to heavyweight.
 - Split GuiStereoTrack and GuiWaveTrack out from TrackPanel.

  JKC: Incremental refactoring started April/2003

  Possibly aiming for Gui classes something like this - it's under
  discussion:

<pre>
   +----------------------------------------------------+
   |      AdornedRulerPanel                             |
   +----------------------------------------------------+
   +----------------------------------------------------+
   |+------------+ +-----------------------------------+|
   ||            | | (L)  GuiWaveTrack                 ||
   || TrackInfo | +-----------------------------------+|
   ||            | +-----------------------------------+|
   ||            | | (R)  GuiWaveTrack                 ||
   |+------------+ +-----------------------------------+|
   +-------- GuiStereoTrack ----------------------------+
   +----------------------------------------------------+
   |+------------+ +-----------------------------------+|
   ||            | | (L)  GuiWaveTrack                 ||
   || TrackInfo | +-----------------------------------+|
   ||            | +-----------------------------------+|
   ||            | | (R)  GuiWaveTrack                 ||
   |+------------+ +-----------------------------------+|
   +-------- GuiStereoTrack ----------------------------+
</pre>
    
  With the whole lot sitting in a TrackPanel which forwards 
  events to the sub objects.

  The GuiStereoTrack class will do the special logic for
  Stereo channel grouping.  
  
  The precise names of the classes are subject to revision.
  Have deliberately not created new files for the new classes 
  such as AdornedRulerPanel and TrackInfo - yet.

*//*****************************************************************/


#include "Audacity.h"
#include "TrackPanel.h"

#ifdef __MACOSX__
#include <Carbon/Carbon.h>
#endif

#include <math.h>

#if DEBUG_DRAW_TIMING
#include <sys/time.h>
#endif

#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/font.h>
#include <wx/fontenum.h>
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/numdlg.h>
#include <wx/choicdlg.h>
#include <wx/spinctrl.h>
#include <wx/listbox.h>
#include <wx/textctrl.h>
#include <wx/intl.h>
#include <wx/image.h>
  
#include "AColor.h"
#include "AllThemeResources.h"
#include "AudacityApp.h"
#include "AudioIO.h"
#include "Envelope.h"
#include "Experimental.h"
#include "Internat.h"
#include "LabelTrack.h"
#include "NoteTrack.h"
#include "Prefs.h"
#include "Project.h"
#include "Snap.h"
#include "Theme.h"
#include "TimeTrack.h"
#include "Track.h"
#include "TrackArtist.h"
#include "TrackPanelAx.h"
#include "ViewInfo.h"
#include "WaveTrack.h"

#include "toolbars/ControlToolBar.h"
#include "toolbars/ToolManager.h"
#include "toolbars/ToolsToolBar.h"

#include "widgets/ASlider.h"
#include "widgets/Ruler.h"

#include <wx/arrimpl.cpp>

WX_DEFINE_OBJARRAY(TrackClipArray);

//This loads the appropriate set of cursors, depending on platform.
#include "../images/Cursors.h"
#include <iostream>



//FIX-ME: The WXMAC code below is obsolete
//#if defined(__WXMAC__) && !defined(__UNIX__)
//#include <Menus.h>
//#endif

#ifdef _DEBUG
    #ifdef _MSC_VER
        #undef THIS_FILE
        static char*THIS_FILE= __FILE__;
        #define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
    #endif
#endif

#define kLeftInset 4
#define kTopInset 4

// Is the distance between A and B less than D?
template < class A, class B, class DIST > bool within(A a, B b, DIST d)
{
   return (a > b - d) && (a < b + d);
}

template < class LOW, class MID, class HIGH >
    bool between_inclusive(LOW l, MID m, HIGH h)
{
   return (m >= l && m <= h);
}

template < class LOW, class MID, class HIGH >
    bool between_exclusive(LOW l, MID m, HIGH h)
{
   return (m > l && m < h);
}

template < class CLIPPEE, class CLIPVAL >
    void clip_top(CLIPPEE & clippee, CLIPVAL val)
{
   if (clippee > val)
      clippee = val;
}

template < class CLIPPEE, class CLIPVAL >
    void clip_bottom(CLIPPEE & clippee, CLIPVAL val)
{
   if (clippee < val)
      clippee = val;
}

/// \todo Probably should move to 'Utils.cpp'.
void SetTrackInfoFont(wxDC * dc)
{
   int fontSize = 10;
#if defined __WXMSW__ || __WXGTK__
   fontSize = 8;
#endif

   wxFont labelFont(fontSize, wxSWISS, wxNORMAL, wxNORMAL);
   dc->SetFont(labelFont);
}

enum {
   TrackPanelFirstID = 2000,
   OnSetNameID,
   OnSetFontID,

   OnMoveUpID,
   OnMoveDownID,

   OnUpOctaveID,
   OnDownOctaveID,

   OnChannelLeftID,
   OnChannelRightID,
   OnChannelMonoID,

   OnRate8ID,              // <---
   OnRate11ID,             //    |
   OnRate16ID,             //    |
   OnRate22ID,             //    |
   OnRate44ID,             //    |
   OnRate48ID,             //    | Leave these in order
   OnRate96ID,             //    | see OnTrackMenu()
   OnRateOtherID,          //    |
                           //    |
   On16BitID,              //    |
   On24BitID,              //    |
   OnFloatID,              // <---

   OnWaveformID,
   OnWaveformDBID,
   OnSpectrumID,
#ifdef LOGARITHMIC_SPECTRUM
   OnSpectrumLogID,
#endif
   OnPitchID,

   OnSplitStereoID,
   OnMergeStereoID,

   OnSetTimeTrackRangeID,
   OnCutSelectedTextID,
   OnCopySelectedTextID,
   OnPasteSelectedTextID,
};

BEGIN_EVENT_TABLE(TrackPanel, wxWindow)
    EVT_MOUSE_EVENTS(TrackPanel::OnMouseEvent)
    EVT_COMMAND(wxID_ANY, EVT_CAPTURE_KEY, TrackPanel::OnCaptureKey)
    EVT_KEY_DOWN(TrackPanel::OnKeyDown)
    EVT_CHAR(TrackPanel::OnChar)
    EVT_SIZE(TrackPanel::OnSize)
    EVT_ERASE_BACKGROUND(TrackPanel::OnErase)
    EVT_PAINT(TrackPanel::OnPaint)
    EVT_SET_FOCUS(TrackPanel::OnSetFocus)
    EVT_KILL_FOCUS(TrackPanel::OnKillFocus)
    EVT_CONTEXT_MENU(TrackPanel::OnContextMenu)
    EVT_MENU(OnSetNameID, TrackPanel::OnSetName)
    EVT_MENU(OnSetFontID, TrackPanel::OnSetFont)
    EVT_MENU(OnSetTimeTrackRangeID, TrackPanel::OnSetTimeTrackRange)

    EVT_MENU_RANGE(OnMoveUpID, OnMoveDownID, TrackPanel::OnMoveTrack)
    EVT_MENU_RANGE(OnUpOctaveID, OnDownOctaveID, TrackPanel::OnChangeOctave)
    EVT_MENU_RANGE(OnChannelLeftID, OnChannelMonoID,
               TrackPanel::OnChannelChange)
    EVT_MENU_RANGE(OnWaveformID, OnPitchID, TrackPanel::OnSetDisplay)
    EVT_MENU_RANGE(OnRate8ID, OnRate96ID, TrackPanel::OnRateChange)
    EVT_MENU_RANGE(On16BitID, OnFloatID, TrackPanel::OnFormatChange)
    EVT_MENU(OnRateOtherID, TrackPanel::OnRateOther)
    EVT_MENU(OnSplitStereoID, TrackPanel::OnSplitStereo)
    EVT_MENU(OnMergeStereoID, TrackPanel::OnMergeStereo)

    EVT_MENU(OnCutSelectedTextID, TrackPanel::OnCutSelectedText)
    EVT_MENU(OnCopySelectedTextID, TrackPanel::OnCopySelectedText)
    EVT_MENU(OnPasteSelectedTextID, TrackPanel::OnPasteSelectedText)
END_EVENT_TABLE()


/// Makes a cursor from an XPM, uses CursorId as a fallback.
wxCursor * MakeCursor( int CursorId, const char * pXpm[36],  int HotX, int HotY )
{
   wxCursor * pCursor;

#ifdef CURSORS_SIZE32
   const int HotAdjust =0;
#else
   const int HotAdjust =8;
#endif

   wxImage Image = wxImage(wxBitmap(pXpm).ConvertToImage());   
   Image.SetMaskColour(255,0,0);
   Image.SetMask();// Enable mask.

#ifdef __WXGTK__
   //
   // Kludge: the wxCursor Image constructor is broken in wxGTK.
   // This code, based loosely on the broken code from the wxGTK source,
   // works around the problem by constructing a 1-bit bitmap and
   // calling the other custom cursor constructor.
   //
   // -DMM
   //

   unsigned char *rgbBits = Image.GetData();
   int w = Image.GetWidth() ;
   int h = Image.GetHeight();
   int imagebitcount = (w*h)/8;
   
   unsigned char *bits = new unsigned char [imagebitcount];
   unsigned char *maskBits = new unsigned char [imagebitcount];

   int i, j, i8;
   unsigned char cMask;
   for (i=0; i<imagebitcount; i++) {
      bits[i] = 0;
      i8 = i * 8;
        
      cMask = 1;
      for (j=0; j<8; j++) {
         if (rgbBits[(i8+j)*3+2] < 127)
            bits[i] = bits[i] | cMask;
         cMask = cMask * 2;
      }
   }

   for (i=0; i<imagebitcount; i++) {
      maskBits[i] = 0x0;
      i8 = i * 8;
      
      cMask = 1;
      for (j=0; j<8; j++) {
         if (rgbBits[(i8+j)*3] < 127 || rgbBits[(i8+j)*3+1] > 127)
            maskBits[i] = maskBits[i] | cMask;
         cMask = cMask * 2;
      }
   }

   pCursor = new wxCursor((const char *)bits, w, h,
                          HotX-HotAdjust, HotY-HotAdjust,
                          (const char *)maskBits,
                          new wxColour(0, 0, 0),
                          new wxColour(255, 255, 255));

#else
   Image.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_X, HotX-HotAdjust );
   Image.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_Y, HotY-HotAdjust );
   pCursor = new wxCursor( Image );
#endif

   return pCursor;
}



// Don't warn us about using 'this' in the base member initializer list.
#ifndef __WXGTK__ //Get rid if this pragma for gtk
#pragma warning( disable: 4355 )
#endif
TrackPanel::TrackPanel(wxWindow * parent, wxWindowID id,
                       const wxPoint & pos,
                       const wxSize & size,
                       TrackList * tracks,
                       ViewInfo * viewInfo,
                       TrackPanelListener * listener,
                       AdornedRulerPanel * ruler)
   : wxPanel(parent, id, pos, size, wxWANTS_CHARS | wxNO_BORDER),
     mTrackInfo(this),
     mListener(listener),
     mTracks(tracks),
     mViewInfo(viewInfo),
     mRuler(ruler),
     mTrackArtist(NULL),
     mBacking(NULL),
     mRefreshBacking(false),
     mAutoScrolling(false)
#ifdef EXPERIMENTAL_RULER_AUTOSIZE
     ,
     vrulerSize(36,0)
#endif //EXPERIMENTAL_RULER_AUTOSIZE
#ifndef __WXGTK__   //Get rid if this pragma for gtk
#pragma warning( default: 4355 )
#endif
{
   SetLabel(_("Track Panel"));
   SetName(_("Track Panel"));

   mAx = new TrackPanelAx( this );
#if wxUSE_ACCESSIBILITY
   SetAccessible( mAx );
#endif
   mMouseCapture = IsUncaptured;
   mSlideUpDownOnly = false;
   mLabelTrackStartXPos=-1;
   mCircularTrackNavigation = false;

   UpdatePrefs();

   mRedrawAfterStop = false;
   mIndicatorShowing = false;

   mPencilCursor  = MakeCursor( wxCURSOR_PENCIL,    DrawCursorXpm,    12, 22);
   mSelectCursor  = MakeCursor( wxCURSOR_IBEAM,     IBeamCursorXpm,   17, 16);
   mEnvelopeCursor= MakeCursor( wxCURSOR_ARROW,     EnvCursorXpm,     16, 16);
   mDisabledCursor= MakeCursor( wxCURSOR_NO_ENTRY,  DisabledCursorXpm,16, 16);
   mSlideCursor   = MakeCursor( wxCURSOR_SIZEWE,    TimeCursorXpm,    16, 16);
   mZoomInCursor  = MakeCursor( wxCURSOR_MAGNIFIER, ZoomInCursorXpm,  19, 15);
   mZoomOutCursor = MakeCursor( wxCURSOR_MAGNIFIER, ZoomOutCursorXpm, 19, 15);
   mLabelCursorLeft  = MakeCursor( wxCURSOR_ARROW,  LabelCursorLeftXpm, 19, 15);
   mLabelCursorRight = MakeCursor( wxCURSOR_ARROW,  LabelCursorRightXpm, 16, 16);

   mArrowCursor = new wxCursor(wxCURSOR_ARROW);
   mSmoothCursor = new wxCursor(wxCURSOR_SPRAYCAN);
   mResizeCursor = new wxCursor(wxCURSOR_SIZENS);
   mRearrangeCursor = new wxCursor(wxCURSOR_HAND);
   mAdjustLeftSelectionCursor = new wxCursor(wxCURSOR_POINT_LEFT);
   mAdjustRightSelectionCursor = new wxCursor(wxCURSOR_POINT_RIGHT);

   // Use AppendCheckItem so we can have ticks beside the items.
   // We would use AppendRadioItem but it only currently works on windows and GTK.
   mRateMenu = new wxMenu();
   mRateMenu->AppendCheckItem(OnRate8ID, wxT("8000 Hz"));
   mRateMenu->AppendCheckItem(OnRate11ID, wxT("11025 Hz"));
   mRateMenu->AppendCheckItem(OnRate16ID, wxT("16000 Hz"));
   mRateMenu->AppendCheckItem(OnRate22ID, wxT("22050 Hz"));
   mRateMenu->AppendCheckItem(OnRate44ID, wxT("44100 Hz"));
   mRateMenu->AppendCheckItem(OnRate48ID, wxT("48000 Hz"));
   mRateMenu->AppendCheckItem(OnRate96ID, wxT("96000 Hz"));
   mRateMenu->AppendCheckItem(OnRateOtherID, _("Other..."));

   mFormatMenu = new wxMenu();
   mFormatMenu->AppendCheckItem(On16BitID, GetSampleFormatStr(int16Sample));
   mFormatMenu->AppendCheckItem(On24BitID, GetSampleFormatStr(int24Sample));
   mFormatMenu->AppendCheckItem(OnFloatID, GetSampleFormatStr(floatSample));

   mWaveTrackMenu = new wxMenu();
   mWaveTrackMenu->Append(OnSetNameID, _("Name..."));
   mWaveTrackMenu->AppendSeparator();
   mWaveTrackMenu->Append(OnMoveUpID, _("Move Track Up"));
   mWaveTrackMenu->Append(OnMoveDownID, _("Move Track Down"));
   mWaveTrackMenu->AppendSeparator();
   mWaveTrackMenu->Append(OnWaveformID, _("Waveform"));
   mWaveTrackMenu->Append(OnWaveformDBID, _("Waveform (dB)"));
   mWaveTrackMenu->Append(OnSpectrumID, _("Spectrum"));
#ifdef LOGARITHMIC_SPECTRUM
   mWaveTrackMenu->Append(OnSpectrumLogID, _("Spectrum log(f)"));
#endif
   mWaveTrackMenu->Append(OnPitchID, _("Pitch (EAC)"));
   mWaveTrackMenu->AppendSeparator();
   mWaveTrackMenu->AppendCheckItem(OnChannelMonoID, _("Mono"));
   mWaveTrackMenu->AppendCheckItem(OnChannelLeftID, _("Left Channel"));
   mWaveTrackMenu->AppendCheckItem(OnChannelRightID, _("Right Channel"));
   mWaveTrackMenu->Append(OnMergeStereoID, _("Make Stereo Track"));
   mWaveTrackMenu->Append(OnSplitStereoID, _("Split Stereo Track"));
   mWaveTrackMenu->AppendSeparator();
   mWaveTrackMenu->Append(0, _("Set Sample Format"), mFormatMenu);
   mWaveTrackMenu->AppendSeparator();
   mWaveTrackMenu->Append(0, _("Set Rate"), mRateMenu);

   mNoteTrackMenu = new wxMenu();
   mNoteTrackMenu->Append(OnSetNameID, _("Name..."));
   mNoteTrackMenu->AppendSeparator();
   mNoteTrackMenu->Append(OnMoveUpID, _("Move Track Up"));
   mNoteTrackMenu->Append(OnMoveDownID, _("Move Track Down"));
   mNoteTrackMenu->AppendSeparator();
   mNoteTrackMenu->Append(OnUpOctaveID, _("Up Octave"));
   mNoteTrackMenu->Append(OnDownOctaveID, _("Down Octave"));

   mLabelTrackMenu = new wxMenu();
   mLabelTrackMenu->Append(OnSetNameID, _("Name..."));
   mLabelTrackMenu->AppendSeparator();
   mLabelTrackMenu->Append(OnSetFontID, _("Font..."));
   mLabelTrackMenu->AppendSeparator();
   mLabelTrackMenu->Append(OnMoveUpID, _("Move Track Up"));
   mLabelTrackMenu->Append(OnMoveDownID, _("Move Track Down"));

   mTimeTrackMenu = new wxMenu();
   mTimeTrackMenu->Append(OnSetNameID, _("Name..."));
   mTimeTrackMenu->AppendSeparator();
   mTimeTrackMenu->Append(OnMoveUpID, _("Move Track Up"));
   mTimeTrackMenu->Append(OnMoveDownID, _("Move Track Down"));
   mTimeTrackMenu->AppendSeparator();
   mTimeTrackMenu->Append(OnSetTimeTrackRangeID, _("Set Range..."));

   mLabelTrackInfoMenu = new wxMenu();
   mLabelTrackInfoMenu->Append(OnCutSelectedTextID, _("Cut"));
   mLabelTrackInfoMenu->Append(OnCopySelectedTextID, _("Copy"));
   mLabelTrackInfoMenu->Append(OnPasteSelectedTextID, _("Paste"));

   mTrackArtist = new TrackArtist();
   mTrackArtist->SetInset(1, kTopInset + 1, kLeftInset + 2, 2);

   mCapturedTrack = NULL;
   mPopupMenuTarget = NULL;

//   SetBackgroundColour( wxColour( 255,255,255 ));

   mTimeCount = 0;
   mTimer.parent = this;
   mTimer.Start(50, FALSE);

   //Initialize a member variable pointing to the current
   //drawing track.
   mDrawingTrack =NULL;

   //More initializations, some of these for no other reason than
   //to prevent runtime memory check warnings
   mZoomStart = -1;
   mZoomEnd = -1;
   mPrevWidth = -1;
   mPrevHeight = -1;

   //Initialize the last selection adjustment time.
   mLastSelectionAdjustment = ::wxGetLocalTimeMillis();

   // This is used to snap the cursor to the nearest track that
   // lines up with it.
   mSnapManager = NULL;
   mSnapLeft = -1;
   mSnapRight = -1;

   mLastCursor = -1;
   mLastIndicator = -1;
}

TrackPanel::~TrackPanel()
{
   mTimer.Stop();

   // This can happen if a label is being edited and the user presses
   // ALT+F4 or Command+Q
   if (HasCapture())
      ReleaseMouse();

   if (mBacking)
   {
      mBackingDC.SelectObject( wxNullBitmap );
      delete mBacking;
   }
//   delete mAx;
   delete mTrackArtist;

   delete mArrowCursor;
   delete mPencilCursor;
   delete mSelectCursor;
   delete mEnvelopeCursor;
   delete mDisabledCursor;
   delete mSlideCursor;
   delete mResizeCursor;
   delete mSmoothCursor;
   delete mZoomInCursor;
   delete mZoomOutCursor;
   delete mLabelCursorLeft;
   delete mLabelCursorRight;
   delete mRearrangeCursor;
   delete mAdjustLeftSelectionCursor;
   delete mAdjustRightSelectionCursor;

   delete mSnapManager;

   // Note that the submenus (mRateMenu, ...)
   // are deleted by their parent
   delete mWaveTrackMenu;
   delete mNoteTrackMenu;
   delete mLabelTrackMenu;
   delete mLabelTrackInfoMenu;
   delete mTimeTrackMenu;
}

void TrackPanel::UpdatePrefs()
{
   mdBr = gPrefs->Read(wxT("/GUI/EnvdBRange"), ENV_DB_RANGE);
   gPrefs->Read(wxT("/GUI/AutoScroll"), &mViewInfo->bUpdateTrackIndicator,
               true);
   gPrefs->Read(wxT("/GUI/AdjustSelectionEdges"), &mAdjustSelectionEdges,
               true);
   gPrefs->Read(wxT("/GUI/CircularTrackNavigation"), &mCircularTrackNavigation,
               false);
   gPrefs->Read(wxT("/GUI/Solo"), &mSoloPref, wxT("Standard") );
   gPrefs->Read(wxT("/AudioIO/SeekShortPeriod"), &mSeekShort,
               1.0);
   gPrefs->Read(wxT("/AudioIO/SeekLongPeriod"), &mSeekLong,
               15.0);

   if (mTrackArtist) {
      mTrackArtist->UpdatePrefs();
   }
}

void TrackPanel::SetStop(bool bStopped)
{
   mViewInfo->bIsPlaying = !bStopped;
   Refresh(false);
}

/// Remembers the track we clicked on and why we captured it.
/// We also use this function to clear the record 
/// of the captured track, passing NULL as the track.
void TrackPanel::SetCapturedTrack( Track * t, enum MouseCaptureEnum MouseCapture )
{
   if( t== NULL ) 
   {
      wxASSERT( MouseCapture == IsUncaptured );
   }
   else
   {
      wxASSERT( MouseCapture != IsUncaptured );
   }
   mCapturedTrack = t;
   mMouseCapture = MouseCapture;
}

void TrackPanel::SelectNone()
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   while (t) {
      t->SetSelected(false);
      if (t->GetKind() == Track::Label)
         ((LabelTrack *) t)->Unselect();
      t = iter.Next();
   }
}

/// Select all tracks marked by the label track lt
void TrackPanel::SelectTracksByLabel( LabelTrack *lt )
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();

   //do nothing if at least one other track is selected
   while (t) {
      if( t->GetSelected() && t != lt )
         return;
      t = iter.Next();
   }

   //otherwise, select all tracks
   t = iter.First();
   while( t )
   {
      t->SetSelected( true );
      t = iter.Next();
   }
}

void TrackPanel::GetTracksUsableArea(int *width, int *height) const
{
   GetSize(width, height);
   *width -= GetLabelWidth();
   // AS: MAGIC NUMBER: What does 2 represent?
   *width -= 2 + kLeftInset;
}

/// Gets the pointer to the AudacityProject that
/// goes with this track panel.
AudacityProject * TrackPanel::GetProject() const
{
   //JKC casting away constness here.
   //Do it in two stages in case 'this' is not a wxWindow.
   //when the compiler will flag the error.
   wxWindow const * const pConstWind = this;
   wxWindow * pWind=(wxWindow*)pConstWind;
#ifdef EXPERIMENTAL_NOTEBOOK
   pWind = pWind->GetParent(); //Page
   wxASSERT( pWind );
   pWind = pWind->GetParent(); //Notebook
   wxASSERT( pWind );
#endif
   pWind = pWind->GetParent(); //MainPanel
   wxASSERT( pWind );
   pWind = pWind->GetParent(); //Project
   wxASSERT( pWind );
   return (AudacityProject*)pWind;
}

/// AS: This gets called on our wx timer events.
void TrackPanel::OnTimer()
{
   mTimeCount++;
   // AS: If the user is dragging the mouse and there is a track that
   //  has captured the mouse, then scroll the screen, as necessary.
   if ((mMouseCapture==IsSelecting) && mCapturedTrack) {
      ScrollDuringDrag();
   }

   wxCommandEvent dummyEvent;
   AudacityProject *p = GetProject();

   // Each time the loop, check to see if we were playing or
   // recording audio, but the stream has stopped.
   if (p->GetAudioIOToken()>0 &&
       !gAudioIO->IsStreamActive(p->GetAudioIOToken())) {
      p->GetControlToolBar()->OnStop(dummyEvent);      
   }

   // Next, check to see if we were playing or recording
   // audio, but now Audio I/O is completely finished.
   // The main point of this is to properly push the state
   // and flush the tracks once we've completely finished
   // recording new state.
   if (p->GetAudioIOToken()>0 &&
       !gAudioIO->IsAudioTokenActive(p->GetAudioIOToken())) {

      if (gAudioIO->GetNumCaptureChannels() > 0) {
         // Tracks are buffered during recording.  This flushes
         // them so that there's nothing left in the append
         // buffers.
         TrackListIterator iter(mTracks);
         for (Track * t = iter.First(); t; t = iter.Next()) {
            if (t->GetKind() == Track::Wave) {
               ((WaveTrack *)t)->Flush();
            }
         }
         MakeParentPushState(_("Recorded Audio"), _("Record"));
      }
      mRedrawAfterStop = false;

      MakeParentRedrawScrollbars();
      p->SetAudioIOToken(0);
      p->RedrawProject();
		//ANSWER-ME: Was DisplaySelection added to solve a repaint problem?        
      DisplaySelection();
   }

   // AS: The "indicator" is the little graphical mark shown in the ruler
   //  that indicates where the current play/record position is.
   if (!gAudioIO->IsPaused() &&
       (mIndicatorShowing || gAudioIO->IsStreamActive(p->GetAudioIOToken())))
   {
      DrawIndicator();
   }

   if(gAudioIO->IsStreamActive(p->GetAudioIOToken()) &&
      gAudioIO->GetNumCaptureChannels()) {

      // Periodically update the display while recording
      
      if (!mRedrawAfterStop) {
         mRedrawAfterStop = true;
         MakeParentRedrawScrollbars();
         mListener->TP_ScrollUpDown( 99999999 );
         Refresh( false );
      }
      else {
         if ((mTimeCount % 5) == 0) {
            // Must tell OnPaint() to recreate the backing bitmap
            // since we've not done a full refresh.
            mRefreshBacking = true;

            // Refresh only the waveform area, not the labels
            // (This actually speeds up redrawing!)
            wxRect trackRect;
            GetSize(&trackRect.width, &trackRect.height);
            trackRect.x = GetLeftOffset(); 
            trackRect.y = 0;
            trackRect.width -= GetLeftOffset();
            Refresh(false, &trackRect);
         }
      }
   }
   if(mTimeCount > 1000)
      mTimeCount = 0;
}

///  We check on each timer tick to see if we need to scroll.
///  Scrolling is handled by mListener, which is an interface
///  to the window TrackPanel is embedded in.
void TrackPanel::ScrollDuringDrag()
{
   // DM: If we're "autoscrolling" (which means that we're scrolling
   //  because the user dragged from inside to outside the window,
   //  not because the user clicked in the scroll bar), then
   //  the selection code needs to be handled slightly differently.
   //  We set this flag ("mAutoScrolling") to tell the selecting
   //  code that we didn't get here as a result of a mouse event,
   //  and therefore it should ignore the mouseEvent parameter,
   //  and instead use the last known mouse position.  Setting
   //  this flag also causes the Mac to redraw immediately rather
   //  than waiting for the next update event; this makes scrolling
   //  smoother on MacOS 9.

   if (mMouseMostRecentX >= mCapturedRect.x + mCapturedRect.width) {
      mAutoScrolling = true;
      mListener->TP_ScrollRight();
   }
   else if (mMouseMostRecentX < mCapturedRect.x) {
      mAutoScrolling = true;
      mListener->TP_ScrollLeft();
   }

   if (mAutoScrolling) {
      // AS: To keep the selection working properly as we scroll,
      //  we fake a mouse event (remember, this function is called
      //  from a timer tick).

      // AS: For some reason, GCC won't let us pass this directly.
      wxMouseEvent e(wxEVT_MOTION);
      HandleSelect(e);
      mAutoScrolling = false;
   }
}

///  This updates the indicator (on a timer tick) that shows
///  where the current play or record position is.  To do this,
///  we cheat a little.  The indicator is drawn during the ruler
///  drawing process (that should probably change, but...), so
///  we create a memory DC and tell the ruler to draw itself there,
///  and then just blit that to the screen.
///  The indicator is a small triangle, red for record, green for play.
void TrackPanel::DrawIndicator()
{
   wxClientDC dc( this );
   DoDrawIndicator( dc );
}

/// Second level DrawIndicator()
void TrackPanel::DoDrawIndicator(wxDC & dc)
{
   bool onScreen;
   int x;

   if( mLastIndicator != -1 )
   {
      onScreen = between_inclusive( mViewInfo->h,
                                    mLastIndicator,
                                    mViewInfo->h + mViewInfo->screen );
      if( onScreen )
      {
         x = GetLeftOffset() + int ( ( mLastIndicator - mViewInfo->h) * mViewInfo->zoom );

         dc.Blit( x, 0, 1, mBacking->GetHeight(), &mBackingDC, x, 0 );
      }

      // Nothing's every perfect...
      //
      // Redraw the cursor since we may have just wiped it out
      if( mLastCursor == mLastIndicator )
      {
         DoDrawCursor( dc );
      }

      mLastIndicator = -1;
   }

   // The stream time can be < 0 if the audio is currently stopped
   double pos = gAudioIO->GetStreamTime();

   AudacityProject *p = GetProject();
   bool audioActive = ( gAudioIO->IsStreamActive( p->GetAudioIOToken() ) != 0 );
   onScreen = between_inclusive( mViewInfo->h,
                                 pos,
                                 mViewInfo->h + mViewInfo->screen );

   // This displays the audio time, too...
   DisplaySelection();

   // BG: Scroll screen if option is set
   // msmeyer: But only if not playing looped or in one-second mode
   if( mViewInfo->bUpdateTrackIndicator &&
       p->mLastPlayMode != loopedPlay &&
       p->mLastPlayMode != oneSecondPlay && 
       audioActive &&
       pos >= 0 &&
       !onScreen &&
       !gAudioIO->IsPaused() )
   {
      mListener->TP_ScrollWindow( pos );
   }

   // Always update scrollbars even if not scrolling the window. This is
   // important when new audio is recorded, because this can change the
   // length of the project and therefore the appearance of the scrollbar.
   MakeParentRedrawScrollbars();

   mIndicatorShowing = ( onScreen && audioActive );

   // Remember it
   mLastIndicator = pos;

   // Get the size of the trackpanel region, so we know where to redraw
   wxRect panelarea = GetRect();
   panelarea.x = GetLeftOffset();

   // Set play/record color
   bool rec = gAudioIO->GetNumCaptureChannels() ? false : true;
   AColor::IndicatorColor( &dc, rec );
      
   // Draw cursor in all visible tracks
   TrackListIterator iter( mTracks );

   // Iterate through each track
   x = GetLeftOffset() + int ( ( pos - mViewInfo->h ) * mViewInfo->zoom );
   int y = -mViewInfo->vpos;
   Track *t;
   for( t = iter.First(); t; t = iter.Next() )
   {
      int height = t->GetHeight();
      wxCoord top = y + kTopInset + 1;
      wxCoord bottom = y + height - 2;
      wxRect trackarea( x, top, 1, bottom - top );

      // Only want non-Label tracks that are visible 
      if( ( panelarea.Intersects( trackarea ) ) &&
          ( t->GetKind() != Track::Label ) )
      {
         // Draw the new indicator in its new location
         dc.DrawLine( x, top, x, bottom );
      }

      // Increment y so you draw on the proper track
      y += height;
   }

   mRuler->DrawIndicator( pos, rec );
}

/// This function draws the cursor things, both in the
/// ruler as seen at the top of the screen, but also in each of the
/// selected tracks.
/// These are the 'vertical lines' through waves and ruler.
void TrackPanel::DrawCursor()
{
   wxClientDC dc( this );
   DoDrawCursor( dc );
}

/// Second level DrawCursor()
void TrackPanel::DoDrawCursor(wxDC & dc)
{
   bool onScreen;
   int x;

   if( mLastCursor != -1 )
   {
      onScreen = between_inclusive( mViewInfo->h,
                                    mLastCursor,
                                    mViewInfo->h + mViewInfo->screen );
      if( onScreen )
      {
         x = GetLeftOffset() + int ( ( mLastCursor - mViewInfo->h) * mViewInfo->zoom );

         dc.Blit( x, 0, 1, mBacking->GetHeight(), &mBackingDC, x, 0 );
      }

      mLastCursor = -1;
   }

   mLastCursor = mViewInfo->sel0;

   onScreen = between_inclusive( mViewInfo->h,
                                 mLastCursor,
                                 mViewInfo->h + mViewInfo->screen );

   if( !onScreen )
   {
      return;
   }

   AColor::CursorColor( &dc );

   x = GetLeftOffset() +
       int ( ( mLastCursor - mViewInfo->h ) * mViewInfo->zoom );
   int y = -mViewInfo->vpos;

   // Draw cursor in all selected tracks
   TrackListIterator iter(mTracks);
   Track *t;
   for( t = iter.First(); t; t = iter.Next() )
   {
      int height = t->GetHeight();
      wxCoord top = y + kTopInset + 1;
      wxCoord bottom = y + height - 2;

      if( t->GetSelected() && t->GetKind() == Track::Time )
      {
         TimeTrack *tt = (TimeTrack *) t;
         double t0 = tt->warp( mLastCursor - mViewInfo->h );
         int warpedX = GetLeftOffset() + int ( t0 * mViewInfo->zoom );
         dc.DrawLine( warpedX, top, warpedX, bottom );
      }
      else if( t->GetSelected() || mAx->IsFocused( t ) )
      {
         dc.DrawLine( x, top, x, bottom ); // <-- The whole point of this routine.
      }

      y += height;
   }

   // AS: Ah, no, this is where we draw the blinky thing in the ruler.
   mRuler->DrawCursor( mLastCursor );

   DisplaySelection();
}

/// OnSize() is called when the panel is resized
void TrackPanel::OnSize(wxSizeEvent & /* event */)
{
   int width, height;
   GetSize( &width, &height );

   // wxMac doesn't like zero dimensions, so protect against it
   width = width == 0 ? 1 : width;
   height = height == 0 ? 1 : height;

   // (Re)allocate the backing bitmap
   if( mBacking )
   {
      mBackingDC.SelectObject( wxNullBitmap );
      delete mBacking;
   }

   mBacking = new wxBitmap( width, height );
   mBackingDC.SelectObject( *mBacking );

   // Refresh the entire area.  Really only need to refresh when
   // expanding...is it worth the trouble?
   Refresh( false );
}

/// OnErase( ) is called during the normal course of 
/// completing an erase operation.
void TrackPanel::OnErase(wxEraseEvent & /* event */)
{
   // Ignore it for now.  This reduces flashing when dragging windows
   // over track area while playing or recording.
   //
   // However, if artifacts or the like are discovered later, then
   // we could blit the backing bitmap here.
}

/// AS: OnPaint( ) is called during the normal course of 
///  completing a repaint operation.
void TrackPanel::OnPaint(wxPaintEvent & /* event */)
{
#if DEBUG_DRAW_TIMING
   wxStopWatch sw;
#endif

   // Construct the paint DC on the heap so that it may be deleted
   // early
   wxDC *dc = new wxPaintDC( this );

   // Supposed to be good-to-do under Windows
   dc->BeginDrawing();

   // Retrieve the damage rectangle
   wxRect box = GetUpdateRegion().GetBox();

   // Recreate the backing bitmap if we have a full refresh
   // (See TrackPanel::Refresh())
   if( mRefreshBacking || ( box == GetRect() ) )
   {
      // Reset (should a mutex be used???)
      mRefreshBacking = false;

      // Redraw the backing bitmap
      DrawTracks( &mBackingDC );

      // Copy it to the display
      dc->Blit( 0, 0, mBacking->GetWidth(), mBacking->GetHeight(), &mBackingDC, 0, 0 );
   }
   else
   {
      // Copy full, possibly clipped, damage rectange
      dc->Blit( box.x, box.y, box.width, box.height, &mBackingDC, box.x, box.y );
   } 

   // Supposed to be good-to-do under Windows
   dc->EndDrawing();

   // Done with the clipped DC
   delete dc;

   // Drawing now goes directly to the client area
   wxClientDC cdc( this );

   // Update the indicator in case it was damaged.
   DoDrawIndicator( cdc );

   // Draw the cursor
   if( mViewInfo->sel0 == mViewInfo->sel1)
      DoDrawCursor( cdc );

#if DEBUG_DRAW_TIMING
   sw.Pause();
   wxLogDebug(wxT("Total: %d milliseconds"), sw.Time() );
#endif
}

/// Makes our Parent (well, whoever is listening to us) push their state.
/// this causes application state to be preserved on a stack for undo ops.
void TrackPanel::MakeParentPushState(wxString desc, wxString shortDesc,
                                     bool consolidate)
{
   mListener->TP_PushState(desc, shortDesc, consolidate);
}

void TrackPanel::MakeParentModifyState()
{
   mListener->TP_ModifyState();
}

void TrackPanel::MakeParentRedrawScrollbars()
{
   mListener->TP_RedrawScrollbars();
}

void TrackPanel::MakeParentResize()
{
   mListener->TP_HandleResize();
}

void TrackPanel::HandleShiftKey(bool down)
{
   mLastMouseEvent.m_shiftDown = down;
   HandleCursorForLastMouseEvent();
}

void TrackPanel::HandleControlKey(bool down)
{
   mLastMouseEvent.m_controlDown = down;
   HandleCursorForLastMouseEvent();
}

void TrackPanel::HandleCursorForLastMouseEvent()
{
   HandleCursor(mLastMouseEvent);
}

/// Used to determine whether it is safe or not to perform certain
/// edits at the moment.
/// @return true if audio is being recorded or is playing.
bool TrackPanel::IsUnsafe()
{
   AudacityProject *p = GetProject();
   bool bUnsafe = (p->GetAudioIOToken()>0 &&
                  gAudioIO->IsStreamActive(p->GetAudioIOToken()));
   return bUnsafe;
}


/// Uses a previously noted 'activity' to determine what 
/// cursor to use.
/// @var mMouseCapture holds the current activity.
bool TrackPanel::SetCursorByActivity( )
{
   bool unsafe = IsUnsafe();

   switch( mMouseCapture )
   {
   case IsSelecting:
      SetCursor(*mSelectCursor);
      return true;
   case IsSliding:
      SetCursor( unsafe ? *mDisabledCursor : *mSlideCursor);
      return true;
   case IsEnveloping:
      SetCursor( unsafe ? *mDisabledCursor : *mEnvelopeCursor);
      return true;
   case IsRearranging:
      SetCursor( unsafe ? *mDisabledCursor : *mRearrangeCursor);
      return true;
   case IsAdjustingLabel:
      return true;
   case IsOverCutLine:
      SetCursor( unsafe ? *mDisabledCursor : *mArrowCursor);
   default:
      break;
   }
   return false;
}

/// When in the label, we can either vertical zoom or re-order tracks.
/// Dont't change cursor/tip to zoom if display is not waveform (either linear of dB) or Spectrum
void TrackPanel::SetCursorAndTipWhenInLabel( Track * t,
         wxMouseEvent &event, const wxChar ** ppTip )
{
#ifdef LOGARITHMIC_SPECTRUM
   if (event.m_x >= GetVRulerOffset() &&
      t->GetKind() == Track::Wave &&
      (((WaveTrack *) t)->GetDisplay() <= WaveTrack::SpectrumDisplay
		||((WaveTrack *) t)->GetDisplay() <= WaveTrack::SpectrumLogDisplay)) {
#else
   if (event.m_x >= GetVRulerOffset() &&
      t->GetKind() == Track::Wave &&
      ((WaveTrack *) t)->GetDisplay() <= WaveTrack::SpectrumDisplay) {
#endif
      *ppTip = _("Click to vertically zoom in, Shift-click to zoom out, Drag to create a particular zoom region.");
      SetCursor(event.ShiftDown()? *mZoomOutCursor : *mZoomInCursor);
   }
   else {
      // Set a status message if over a label
      *ppTip = _("Drag the label vertically to change the order of the tracks.");
      SetCursor(*mArrowCursor);
   }
}

/// When in the resize area we can adjust size or relative size.
void TrackPanel::SetCursorAndTipWhenInVResizeArea( Track * label, 
         bool bLinked, const wxChar ** ppTip )
{
   // Check to see whether it is the first channel of a stereo track
   if (bLinked) {
      // If we are in the label we got here 'by mistake' and we're 
      // not actually in the resize area at all.  (The resize area 
      // is shorter when it is between stereo tracks).

      // IF we are in the label THEN return.
      // Subsequently called functions can detect that a tip and 
      // cursor are still needed.
      if (label) 
         return;
      *ppTip = _("Click and drag to adjust relative size of stereo tracks.");
      SetCursor(*mResizeCursor);
   } else {
      *ppTip = _("Click and drag to resize the track.");
      SetCursor(*mResizeCursor);
   }
}

/// When in a label track, find out if we've hit anything that 
/// would cause a cursor change.
void TrackPanel::SetCursorAndTipWhenInLabelTrack( LabelTrack * pLT, 
       wxMouseEvent & event, const wxChar ** ppTip )
{
   int edge=pLT->OverGlyph(event.m_x, event.m_y);
   if(edge !=0)
   {
      SetCursor(*mArrowCursor);
   }

   //KLUDGE: We refresh the whole Label track when the icon hovered over
   //changes colouration.  As well as being inefficient we are also 
   //doing stuff that should be delegated to the label track itself.
   edge += pLT->mbHitCenter ? 4:0; 
   if( edge != pLT->mOldEdge )
   {
      pLT->mOldEdge = edge;
      RefreshTrack( pLT, true );
   }
   // IF edge!=0 THEN we've set the cursor and we're done.
   // signal this by setting the tip.
   if( edge != 0 )
   {
      *ppTip=
         (pLT->mbHitCenter ) ?
         _("Drag one or more label boundaries") :
         _("Drag label boundary");
   }
}

// The select tool can have different cursors and prompts depending on what
// we hover over, most notably when hovering over the selction boundaries.
// Determine and set the cursor and tip accordingly.
void TrackPanel::SetCursorAndTipWhenSelectTool( Track * t, 
        wxMouseEvent & event, wxRect &r, bool bMultiToolMode, const wxChar ** ppTip )
{
   SetCursor(*mSelectCursor);

   //In Multi-tool mode, give multitool prompt if no-special-hit.
   if( bMultiToolMode ) {
      #ifdef __WXMAC__
      /* i18n-hint: This string is for the Mac OS, which uses Command-, as the shortcut for Preferences */
      *ppTip = _("Multi-Tool Mode: Cmd-, for Mouse and Keyboard Preferences");
      #else
      /* i18n-hint: This string is for Windows and Linux, which uses Control-P as the shortcut for Preferences */
      *ppTip = _("Multi-Tool Mode: Ctrl-P for Mouse and Keyboard Preferences");
      #endif
   }
   
   //Make sure we are within the selected track
   if (!t || !t->GetSelected()) 
      return;

   wxInt64 leftSel = TimeToPosition(mViewInfo->sel0, r.x);
   wxInt64 rightSel = TimeToPosition(mViewInfo->sel1, r.x);

   // Something is wrong if right edge comes before left edge
   wxASSERT(!(rightSel < leftSel));

   // Adjusting the selection edges can be turned off in
   // the preferences...
   if (!mAdjustSelectionEdges) {
   }
   // Is the cursor over the left selection boundary?
   else if (within(event.m_x, leftSel, SELECTION_RESIZE_REGION)) {
      *ppTip = _("Click and drag to move left selection boundary.");
      SetCursor(*mAdjustLeftSelectionCursor);
   }
   // Is the cursor over the right selection boundary?
   else if (within(event.m_x, rightSel, SELECTION_RESIZE_REGION)) {
      *ppTip = _("Click and drag to move right selection boundary.");
      SetCursor(*mAdjustRightSelectionCursor);
   }
   // It's possible we didn't set the tip and cursor.
   // Subsequently called functions can detect this.
}

/// In this function we know what tool we are using,
/// so set the cursor accordingly.
void TrackPanel::SetCursorAndTipByTool( int tool, 
         wxMouseEvent & event, const wxChar ** /*ppTip*/ )
{
   bool unsafe = IsUnsafe();

   // Change the cursor based on the active tool.
   switch (tool) {
   case selectTool:
      wxFAIL;// should have already been handled 
      break;
   case envelopeTool:
      SetCursor(unsafe ? *mDisabledCursor : *mEnvelopeCursor);
      break;
   case slideTool:
      SetCursor(unsafe ? *mDisabledCursor : *mSlideCursor);
      break;
   case zoomTool:
      SetCursor(event.ShiftDown()? *mZoomOutCursor : *mZoomInCursor);
      break;
   case drawTool:
      if (unsafe)
         SetCursor(*mDisabledCursor);
      else
         SetCursor(event.AltDown()? *mSmoothCursor : *mPencilCursor);
      break;
   }
   // doesn't actually change the tip itself, but it could (should?) do at some
   // future date.
}

///  TrackPanel::HandleCursor( ) sets the cursor drawn at the mouse location.
///  As this procedure checks which region the mouse is over, it is
///  appropriate to establish the message in the status bar.
void TrackPanel::HandleCursor(wxMouseEvent & event)
{
   mLastMouseEvent = event;

   // (1), If possible, set the cursor based on the current activity
   //      ( leave the StatusBar alone ).
   if( SetCursorByActivity() )
      return;

   // (2) If we are not over a track at all, set the cursor to Arrow and 
   //     clear the StatusBar, 
   wxRect r;
   int num;
   Track *label = FindTrack(event.m_x, event.m_y, true, true, &r, &num);
   Track *nonlabel = FindTrack(event.m_x, event.m_y, false, false, &r, &num);
   Track *t = label ? label : nonlabel;

   if (!t) {
      SetCursor(*mArrowCursor);
      mListener->TP_DisplayStatusMessage(wxT(""));
      return;
   }

   // (3) The easy cases are done.
   // Now we've got to hit-test against a number of different possibilities.
   // We could be over the label or a vertical ruler etc...

   // Strategy here is to set the tip when we have determined and
   // set the correct cursor.  We stop doing tests for what we've
   // hit once the tip is not NULL.
   const wxChar *tip = NULL;

   if (label) {
      SetCursorAndTipWhenInLabel( label, event, &tip );
   }

   // Are we within the vertical resize area?
   if ((tip == NULL ) && within(event.m_y, r.y + r.height, TRACK_RESIZE_REGION)) 
   {
      SetCursorAndTipWhenInVResizeArea( label, t->GetLinked(), &tip );
      // tip may still be NULL at this point, in which case we go on looking.
   }

   // Otherwise, we must be over a track of some kind 
   // Is it a label track?
   if ((tip==NULL) && (t->GetKind() == Track::Label))
   {
      // We are over a label track
      SetCursorAndTipWhenInLabelTrack( (LabelTrack*)t, event, &tip );
      // ..and if we haven't yet determined the cursor, 
      // we go on to do all the standard track hit tests.
   }  

   if( tip==NULL )
   {
      ToolsToolBar * ttb = mListener->TP_GetToolsToolBar();
      if( ttb == NULL )
         return;
      // JKC: DetermineToolToUse is called whenever the mouse 
      // moves.  I had some worries about calling it when in 
      // multimode as it then has to hit-test all 'objects' in
      // the track panel, but performance seems fine in 
      // practice (on a P500).
      int tool = DetermineToolToUse( ttb, event );

      tip = ttb->GetMessageForTool( tool );

      // We don't include the select tool in 
      // SetCursorAndTipByTool() because it's more complex than
      // the other tool cases.
      if( tool != selectTool )
      {
         SetCursorAndTipByTool( tool, event, &tip);
      }
      else
      {
         bool bMultiToolMode = ttb->IsDown(multiTool);
         SetCursorAndTipWhenSelectTool( t, event, r, bMultiToolMode, &tip );
      }
   }

   if (tip)
      mListener->TP_DisplayStatusMessage(tip);
}


/// This function handles various ways of starting and extending
/// selections.  These are the selections you make by clicking and
/// dragging over a waveform.
void TrackPanel::HandleSelect(wxMouseEvent & event)
{
   wxRect r;
   int num;
   Track *t = FindTrack(event.m_x, event.m_y, false, false, &r, &num);

   // AS: Ok, did the user just click the mouse, release the mouse,
   //  or drag?
   if (event.LeftDown()) {
      // AS: Now, did they click in a track somewhere?  If so, we want
      //  to extend the current selection or start a new selection, 
      //  depending on the shift key.  If not, cancel all selections.
      if (t)
         SelectionHandleClick(event, t, r, num);
      else {
         SelectNone();
         Refresh(false);
      }
      
   } else if (event.LeftUp() || event.RightUp()) {
      if (mSnapManager) {
         delete mSnapManager;
         mSnapManager = NULL;
      }
      mSnapLeft = -1;
      mSnapRight = -1;

      SetCapturedTrack( NULL );
      //Send the new selection state to the undo/redo stack:
      MakeParentModifyState();
      
   } else if (event.LeftDClick() && !event.ShiftDown()) {
      if (!mCapturedTrack) {
         wxRect r;
         int num;
         mCapturedTrack =
            FindTrack(event.m_x, event.m_y, false, false, &r, &num);
         if (!mCapturedTrack)
            return;
      }

      // Deselect all other tracks and select this one.
      SelectNone();
      
      mTracks->Select(mCapturedTrack);

      // Default behavior: select whole track
      mViewInfo->sel0 = mCapturedTrack->GetOffset();
      mViewInfo->sel1 = mCapturedTrack->GetEndTime();

      // Special case: if we're over a clip in a WaveTrack,
      // select just that clip
      if (mCapturedTrack->GetKind() == Track::Wave) {
         WaveTrack *w = (WaveTrack *)mCapturedTrack;
         WaveClip *selectedClip = w->GetClipAtX(event.m_x);
         if (selectedClip) {
            mViewInfo->sel0 = selectedClip->GetOffset();
            mViewInfo->sel1 = selectedClip->GetEndTime();      
         }
         //Also, capture this track for dragging until we up-click.
         mCapturedClipArray.Add(TrackClip(w, selectedClip));
                     
         mMouseCapture = IsSliding;

         Refresh(false);
         StartSlide(event);
         goto done;
      }

      Refresh(false);
      SetCapturedTrack( NULL );
      MakeParentModifyState();
   } 
 done:
   SelectionHandleDrag(event, t);
}

/// This function gets called when we're handling selection
/// and the mouse was just clicked.
void TrackPanel::SelectionHandleClick(wxMouseEvent & event,
                                      Track * pTrack, wxRect r, int num)
{
   mCapturedTrack = pTrack;
   mCapturedRect = r;
   mCapturedNum = num;

   mMouseClickX = event.m_x;
   mMouseClickY = event.m_y;
   bool startNewSelection = true;
   mMouseCapture=IsSelecting;

   if (mSnapManager)
      delete mSnapManager;
   mSnapManager = new SnapManager(mTracks, NULL,
                                  mViewInfo->zoom,
                                  4); // pixel tolerance
   mSnapLeft = -1;
   mSnapRight = -1;

   if (event.ShiftDown()) {
      // If the shift button is down and no track is selected yet,
      // at least select the track we clicked into.
      bool isAtLeastOneTrackSelected = false;
      double selend = PositionToTime(event.m_x, r.x);
     
      TrackListIterator iter(mTracks);
      for (Track *t = iter.First(); t; t = iter.Next())
         if (t->GetSelected()) {
            isAtLeastOneTrackSelected = true;
            break;
         }
      
      if (!isAtLeastOneTrackSelected)
         pTrack->SetSelected(true);

      // Edit the selection boundary nearest the mouse click.
      if (fabs(selend - mViewInfo->sel0) < fabs(selend - mViewInfo->sel1))
         mSelStart = mViewInfo->sel1;
      else
         mSelStart = mViewInfo->sel0;

      // If the shift button is down, extend the current selection.
      ExtendSelection(event.m_x, r.x, pTrack);
      return;
   }


   // A control-click will set just the indicator to the clicked spot,
   // and turn playback on.  
   else if(event.CmdDown())
      {
         AudacityProject *p = GetActiveProject();
         if (p) {
            
            double clicktime = PositionToTime(event.m_x, GetLeftOffset());
            double endtime = clicktime < mViewInfo->sel1? mViewInfo->sel1: mViewInfo->total ;

         //Behavior should differ depending upon whether we are
         //currently in playback mode or not.

         bool busy = gAudioIO->IsBusy();
         if(!busy)
            {
               //If we aren't currently playing back, start playing back at 
               //the clicked point
               ControlToolBar * ctb = p->GetControlToolBar();
               ctb->SetPlay(true);
               ctb->PlayPlayRegion(clicktime, endtime,false) ;
                  
            }
            else
            {
               //If we are playing back, stop and move playback
               //to the clicked point.
               //This unpauses paused audio as well.  The right thing to do might be to
               //leave it paused but move the point.  This would probably
               //require a new method in ControlToolBar: SetPause();
               ControlToolBar * ctb = p->GetControlToolBar();
               ctb->StopPlaying();
               ctb->PlayPlayRegion(clicktime,endtime,false) ;
            }
       
         }

         return;        
      }  
   //Make sure you are within the selected track
   if (pTrack && pTrack->GetSelected()) {
      wxInt64 leftSel = TimeToPosition(mViewInfo->sel0, r.x);
      wxInt64 rightSel = TimeToPosition(mViewInfo->sel1, r.x);
      wxASSERT(leftSel <= rightSel);
      // Adjusting selection edges can be turned off in the
      // preferences now
      if (!mAdjustSelectionEdges) {
      }
      // Is the cursor over the left selection boundary?
      else if (within(event.m_x, leftSel, SELECTION_RESIZE_REGION)) {
         // Pin the right selection boundary
         mSelStart = mViewInfo->sel1;
         startNewSelection = false;
      }
      // Is the cursor over the right selection boundary?
      else if (within(event.m_x, rightSel, SELECTION_RESIZE_REGION)) {
         // Pin the left selection boundary
         mSelStart = mViewInfo->sel0;
         startNewSelection = false;
      }
   }
 
   //Determine if user clicked on a label track. 
   if (pTrack->GetKind() == Track::Label) {
      LabelTrack *lt = (LabelTrack *) pTrack;
      if (lt->HandleMouse(event, r,//mCapturedRect,
                          mViewInfo->h, mViewInfo->zoom,
                          &mViewInfo->sel0, &mViewInfo->sel1)) {
         MakeParentPushState(_("Modified Label"),
                             _("Label Edit"),
                             true /* consolidate */);
      }

      // IF the user clicked a label, THEN select all other tracks by Label
      if (lt->IsSelected()) {
         mTracks->Select( lt );
         SelectTracksByLabel( lt );
         DisplaySelection();
         return;
      }
   }
  
   if (startNewSelection) {
      // If we didn't move a selection boundary, start a new selection
      SelectNone();
      StartSelection(event.m_x, r.x);
      mTracks->Select(pTrack);
      SetFocusedTrack(pTrack);
      DisplaySelection();
   }
}


/// Reset our selection markers.
void TrackPanel::StartSelection(int mouseXCoordinate, int trackLeftEdge)
{
   mSelStart = mViewInfo->h + ((mouseXCoordinate - trackLeftEdge)
                               / mViewInfo->zoom);

   double s = mSelStart;

   if (mSnapManager) {
      mSnapLeft = -1;
      mSnapRight = -1;
      if (mSnapManager->Snap(mCapturedTrack, mSelStart, false, &s)) {
         mSnapLeft = TimeToPosition(s, trackLeftEdge);
      }
   }

   mViewInfo->sel0 = s;
   mViewInfo->sel1 = s;

   MakeParentModifyState();
}

/// Extend the existing selection
void TrackPanel::ExtendSelection(int mouseXCoordinate, int trackLeftEdge,
                                 Track *pTrack)
{
   double selend = PositionToTime(mouseXCoordinate, trackLeftEdge);
   clip_bottom(selend, 0.0);

   double origSel0, origSel1;
   double sel0, sel1;
   Track *track0, *track1;

   if (pTrack == NULL && mCapturedTrack != NULL)
      pTrack = mCapturedTrack;

   if (mSelStart < selend) {
      sel0 = mSelStart;
      track0 = mCapturedTrack;
      sel1 = selend;
      track1 = pTrack;
   }
   else {
      sel1 = mSelStart;
      track1 = mCapturedTrack;
      sel0 = selend;
      track0 = pTrack;
   }

   origSel0 = sel0;
   origSel1 = sel1;

   if (mSnapManager) {
      mSnapLeft = -1;
      mSnapRight = -1;
      if (mSnapManager->Snap(mCapturedTrack, sel0, false, &sel0)) {
         mSnapLeft = TimeToPosition(sel0, trackLeftEdge);
      }
      if (mSnapManager->Snap(mCapturedTrack, sel1, true, &sel1)) {
         mSnapRight = TimeToPosition(sel1, trackLeftEdge);
      }

      if (mSnapLeft >= 0 && mSnapRight >= 0 && mSnapRight - mSnapLeft < 3) {
         // Too close together.  Better not to snap at all.
         sel0 = origSel0;
         sel1 = origSel1;
         mSnapLeft = -1;
         mSnapRight = -1;
      }
   }

   mViewInfo->sel0 = sel0;
   mViewInfo->sel1 = sel1;

   MakeParentModifyState();

   // Full refresh since the label area may need to indicate
   // newly selected tracks.
   Refresh(false);

   // Make sure the ruler follows suit.
   mRuler->DrawSelection();

   // As well as the SelectionBar.
   DisplaySelection();
}

/// AS: If we're dragging to extend a selection (or actually,
///  if the screen is scrolling while you're selecting), we
///  handle it here.
void TrackPanel::SelectionHandleDrag(wxMouseEvent & event, Track *clickedTrack)
{
   // AS: If we're not in the process of selecting (set in
   //  the SelectionHandleClick above), fuhggeddaboudit.
   if (mMouseCapture!=IsSelecting)
      return;

   // Also fuhggeddaboudit if we're not dragging and not autoscrolling.
   if ((!event.Dragging() && !mAutoScrolling) || event.CmdDown())
      return;

   wxRect r      = mCapturedRect;
   int num       = mCapturedNum;
   Track *pTrack = mCapturedTrack;

   // AS: Note that FindTrack will replace r and num's values.
   if (!pTrack)
      pTrack = FindTrack(event.m_x, event.m_y, false, false, &r, &num);

   // Also fuhggeddaboudit if not in a track.
   if (!pTrack) 
      return;

   int x = mAutoScrolling ? mMouseMostRecentX : event.m_x;
   int y = mAutoScrolling ? mMouseMostRecentY : event.m_y;

   // JKC: Logic to prevent a selection smaller than 5 pixels to
   // prevent accidental dragging when selecting.
   // (if user really wants a tiny selection, they should zoom in).
   // Can someone make this value of '5' configurable in
   // preferences?
   const int minimumSizedSelection = 5; //measured in pixels
   wxInt64 SelStart=TimeToPosition( mSelStart, r.x); //cvt time to pixels.
   // Abandon this drag if selecting < 5 pixels.
   if(wxLongLong(SelStart-x).Abs() < minimumSizedSelection)
       return;

   // Handle which tracks are selected
   int num2;
   if (0 != FindTrack(x, y, false, false, NULL, &num2)) {
      // The tracks numbered num...num2 should be selected

      TrackListIterator iter(mTracks);
      Track *t = iter.First();
      for (int i = 1; t; i++, t = iter.Next()) {
         if ((i >= num && i <= num2) || (i >= num2 && i <= num))
            mTracks->Select(t);
      }
   }

   ExtendSelection(x, r.x, clickedTrack);

#ifdef __WXMAC__
#if ((wxMAJOR_VERSION == 2) && (wxMINOR_VERSION <= 4))
   if (mAutoScrolling)
      MacUpdateImmediately();
#endif
#endif
}

/// Converts a position (mouse X coordinate) to 
/// project time, in seconds.  Needs the left edge of
/// the track as an additional parameter.
double TrackPanel::PositionToTime(wxInt64 mouseXCoordinate,
                                  wxInt64 trackLeftEdge) const
{
   return mViewInfo->h + ((mouseXCoordinate - trackLeftEdge)
                          / mViewInfo->zoom);
}


/// STM: Converts a project time to screen x position.
wxInt64 TrackPanel::TimeToPosition(double projectTime,
                                   wxInt64 trackLeftEdge) const
{
   return static_cast <
       wxInt64 >(mViewInfo->zoom * (projectTime - mViewInfo->h) +
                 trackLeftEdge);
}

/// HandleEnvelope gets called when the user is changing the
/// amplitude envelope on a track.
void TrackPanel::HandleEnvelope(wxMouseEvent & event)
{
   if (event.LeftDown()) {
      wxRect r;
      int num;
      mCapturedTrack = FindTrack(event.m_x, event.m_y, false, false, &r, &num);

      if (!mCapturedTrack)
         return;

      if (mCapturedTrack->GetKind() == Track::Wave)
      {
         mCapturedEnvelope =
            ((WaveTrack*)mCapturedTrack)->GetEnvelopeAtX(event.GetX());
      } else {
         mCapturedEnvelope = NULL;
      }
      
      mCapturedRect = r;
      mCapturedRect.y += kTopInset;
      mCapturedRect.height -= kTopInset;
      mCapturedNum = num;
   }
   // AS: if there's actually a selected track, then forward all of the
   //  mouse events to its envelope.
   if (mCapturedTrack)
      ForwardEventToEnvelope(event);

   if (event.LeftUp()) {
      mCapturedTrack = NULL;
      MakeParentPushState(
         _("Adjusted envelope."),
         _("Envelope"),
         false /* do not consolidate these actions */
         );
   }
}

/// We've established we're a time track.  
/// send events for its envelope.
void TrackPanel::ForwardEventToTimeTrackEnvelope(wxMouseEvent & event)
{
   TimeTrack *ptimetrack = (TimeTrack *) mCapturedTrack;
   Envelope *pspeedenvelope = ptimetrack->GetEnvelope();
   
   wxRect envRect = mCapturedRect;
   envRect.y++;
   envRect.height -= 2;
   bool needUpdate =
      pspeedenvelope->MouseEvent(
         event, envRect,
         mViewInfo->h, mViewInfo->zoom,
         false,0.,1.);
   if( needUpdate )
   {
      //ptimetrack->Draw();
      Refresh(false);
   }
}

/// We've established we're a wave track.  
/// send events for its envelope.
void TrackPanel::ForwardEventToWaveTrackEnvelope(wxMouseEvent & event)
{
   WaveTrack *pwavetrack = (WaveTrack *) mCapturedTrack;
   Envelope *penvelope = mCapturedEnvelope;

   // Possibly no-envelope, for example when in spectrum view mode.
   // if so, then bail out.
   if (!penvelope)
      return;
   
   // AS: WaveTracks can be displayed in several different formats.
   //  This asks which one is in use. (ie, Wave, Spectrum, etc)
   int display = pwavetrack->GetDisplay();
   bool needUpdate = false;
   
   // AS: If we're using the right type of display for envelope operations
   //  ie one of the Wave displays
   if (display <= 1) {
      bool dB = (display == 1);
      
      // AS: Then forward our mouse event to the envelope.
      // It'll recalculate and then tell us whether or not to redraw.
      wxRect envRect = mCapturedRect;
      envRect.y++;
      envRect.height -= 2;
      float zoomMin, zoomMax;
      pwavetrack->GetDisplayBounds(&zoomMin, &zoomMax);
      needUpdate = penvelope->MouseEvent(
         event, envRect,
         mViewInfo->h, mViewInfo->zoom,
         dB, zoomMin, zoomMax);
      
      // If this track is linked to another track, make the identical
      // change to the linked envelope:
      WaveTrack *link = (WaveTrack *) mTracks->GetLink(mCapturedTrack);
      if (link) {
         Envelope *e2 = link->GetEnvelopeAtX(event.GetX());
         // There isn't necessarily an envelope there; no guarantee a
         // linked track has the same WaveClip structure...
         if (e2) {
            wxRect envRect = mCapturedRect;
            envRect.y++;
            envRect.height -= 2;
            float zoomMin, zoomMax;
            pwavetrack->GetDisplayBounds(&zoomMin, &zoomMax);
            needUpdate |= e2->MouseEvent(event, envRect,
                                         mViewInfo->h, mViewInfo->zoom, dB,
                                         zoomMin, zoomMax);
         }
      }
   }
   if (needUpdate) {
      Refresh(false);
   }
}


/// The Envelope class actually handles things at the mouse
/// event level, so we have to forward the events over.  Envelope
/// will then tell us whether or not we need to redraw.

// AS: I'm not sure why we can't let the Envelope take care of
//  redrawing itself.  ?

void TrackPanel::ForwardEventToEnvelope(wxMouseEvent & event)
{
   if (mCapturedTrack && mCapturedTrack->GetKind() == Track::Time)
   {
      ForwardEventToTimeTrackEnvelope( event );
   }
   else if (mCapturedTrack && mCapturedTrack->GetKind() == Track::Wave)
   {
      ForwardEventToWaveTrackEnvelope( event );
   }
}

void TrackPanel::HandleSlide(wxMouseEvent & event)
{
   if (event.LeftDown())
      StartSlide(event);

   if (mMouseCapture != IsSliding)
      return;

   if (event.Dragging() && mCapturedTrack)
      DoSlide(event);

   if (event.LeftUp()) {
      SetCapturedTrack( NULL );

      if (mSnapManager) {
         delete mSnapManager;
         mSnapManager = NULL;
      }
      mSnapLeft = -1;
      mSnapRight = -1;

      if (!mDidSlideVertically && mHSlideAmount==0)
         return;

      MakeParentRedrawScrollbars();

      wxString msg;
      bool consolidate;
      if (mDidSlideVertically) {
         msg.Printf(_("Moved clip to another track"));
         consolidate = false;
      }
      else {
         wxString direction = mHSlideAmount>0 ? _("right") : _("left");
         /* i18n-hint: %s is a direction like left or right */
         msg.Printf(_("Time shifted tracks/clips %s %.02f seconds"),
                    direction.c_str(), fabs(mHSlideAmount));
         consolidate = true;
      }
      MakeParentPushState(msg, _("Time-Shift"), consolidate);
   }
}

/// Prepare for sliding.
void TrackPanel::StartSlide(wxMouseEvent & event)
{
   wxRect r;
   int num;

   mHSlideAmount = 0.0;
   mDidSlideVertically = false;
   
   Track *vt = FindTrack(event.m_x, event.m_y, false, false, &r, &num);
   if (!vt) 
      return;

   ToolsToolBar * ttb = mListener->TP_GetToolsToolBar();
   bool multiToolModeActive = (ttb && ttb->IsDown(multiTool));

   if (vt->GetKind() == Track::Wave && !event.ShiftDown())
   {
      WaveTrack* wt = (WaveTrack*)vt;
      mCapturedClip = wt->GetClipAtX(event.m_x);
      if (mCapturedClip == NULL)
         return;

      // The captured clip is the focus, but we need to create a list
      // of all clips that have to move, also...
      
      mCapturedClipArray.Clear();

      double clickTime = 
         PositionToTime(event.m_x, GetLeftOffset());
      bool clickedInSelection =
         (wt->GetSelected() &&
          clickTime >= mViewInfo->sel0 &&
          clickTime < mViewInfo->sel1);

      if (clickedInSelection) {
         // Loop through every clip and include it if it overlaps the
         // selection.  This will get mCapturedClip (and its stereo pair)
         // automatically

         mCapturedClipIsSelection = true;

         TrackListIterator iter(mTracks);
         Track *t = iter.First();
         while (t) {
            if (t->GetSelected()) {
               if (t->GetKind() == Track::Wave) {
                  WaveTrack *wt = (WaveTrack *)t;
                  WaveClipList::Node* it;
                  for(it=wt->GetClipIterator(); it; it=it->GetNext()) {
                     WaveClip *clip = it->GetData();
                     double clip0 = clip->GetStartTime();
                     double clip1 = clip->GetEndTime();
                     
                     if (clip0 <= mViewInfo->sel1 &&
                         clip1 >= mViewInfo->sel0) {
                        mCapturedClipArray.Add(TrackClip(wt, clip));
                     }
                  }      
               }
               else {
                  mCapturedClipArray.Add(TrackClip(t, NULL));
               }
            }
            t = iter.Next();
         }
      }
      else {
         // Only add mCapturedClip, and possibly its stereo partner,
         // to the list of clips to move.

         mCapturedClipIsSelection = false;

         mCapturedClipArray.Add(TrackClip(wt, mCapturedClip));

         Track *partner = mTracks->GetLink(wt);
         if (partner && partner->GetKind()==Track::Wave) {
            WaveClip *clip = ((WaveTrack *)partner)->GetClipAtX(event.m_x);
            if (clip) {
               mCapturedClipArray.Add(TrackClip(partner, clip));
            }
         }
      }
   } else {
      mCapturedClip = NULL;
      mCapturedClipArray.Clear();
   }
   
   mSlideUpDownOnly = event.CmdDown() && !multiToolModeActive;

   mCapturedTrack = vt;
   mCapturedRect = r;
   mCapturedNum = num;

   mMouseClickX = event.m_x;
   mMouseClickY = event.m_y;

   mSelStart = mViewInfo->h + ((event.m_x - r.x) / mViewInfo->zoom);

   if (mSnapManager)
      delete mSnapManager;
   mSnapManager = new SnapManager(mTracks,
                                  &mCapturedClipArray,
                                  mViewInfo->zoom,
                                  4); // pixel tolerance
   mSnapLeft = -1;
   mSnapRight = -1;
   mSnapPreferRightEdge = false;
   if (mCapturedClip) {
      if (fabs(mSelStart - mCapturedClip->GetEndTime()) <
          fabs(mSelStart - mCapturedClip->GetStartTime()))
         mSnapPreferRightEdge = true;
   }

   mMouseCapture = IsSliding;
}

/// Slide tracks horizontally, or slide clips horizontally or vertically
/// (e.g. moving clips between tracks).

// GM: DoSlide now implementing snap-to
// samples functionality based on sample rate. 
void TrackPanel::DoSlide(wxMouseEvent & event)
{
   unsigned int i;

   // find which track the mouse is currently in (mouseTrack) -
   // this may not be the same as the one we started in...
   WaveTrackArray tracks = mTracks->GetWaveTrackArray(false);
   WaveTrack* mouseTrack = NULL;
   for (i=0; i<tracks.GetCount(); i++)
   {
      WaveTrack* track = tracks.Item(i);
      wxRect r;
      track->GetDisplayRect(&r);
      if (event.m_y >= r.y && event.m_y < r.y+r.height)
      {
         mouseTrack = track;
         break;
      }
   }
   if(i>=tracks.GetCount())
      return;  // not in a track at all - probably between/above/below them

   // Start by undoing the current slide amount; everything
   // happens relative to the original horizontal position of
   // each clip...
   if (mCapturedClip) {
      for(i=0; i<mCapturedClipArray.GetCount(); i++) {
         if (mCapturedClipArray[i].clip)
            mCapturedClipArray[i].clip->Offset(-mHSlideAmount);
         else
            mCapturedClipArray[i].track->Offset(-mHSlideAmount);
      }
   }
   else {
      mCapturedTrack->Offset(-mHSlideAmount);
      Track* link = mTracks->GetLink(mCapturedTrack);
      if (link)
         link->Offset(-mHSlideAmount);
   }
   
   if (mCapturedClipIsSelection) {
      // Slide the selection, too
      mViewInfo->sel0 -= mHSlideAmount;
      mViewInfo->sel1 -= mHSlideAmount;
   }
   mHSlideAmount = 0.0;

   double desiredSlideAmount = (event.m_x - mMouseClickX) / mViewInfo->zoom;
   desiredSlideAmount = rint(mouseTrack->GetRate() * desiredSlideAmount) / mouseTrack->GetRate();  // set it to a sample point

   // Adjust desiredSlideAmount using SnapManager
   if (mSnapManager && mCapturedClip) {
      double clipLeft = mCapturedClip->GetStartTime() + desiredSlideAmount;
      double clipRight = mCapturedClip->GetEndTime() + desiredSlideAmount;

      double newClipLeft = clipLeft;
      double newClipRight = clipRight;

      mSnapManager->Snap(mCapturedTrack, clipLeft, false, &newClipLeft);
      mSnapManager->Snap(mCapturedTrack, clipRight, false, &newClipRight);

      // Only one of them is allowed to snap
      if (newClipLeft != clipLeft && newClipRight != clipRight) {
         if (mSnapPreferRightEdge)
            newClipLeft = clipLeft;
         else
            newClipRight = clipRight;
      }

      // Take whichever one snapped (if any) and compute the new desiredSlideAmount
      mSnapLeft = -1;
      mSnapRight = -1;
      if (newClipLeft != clipLeft) {
         double difference = (newClipLeft - clipLeft);
         desiredSlideAmount += difference;
         mSnapLeft = TimeToPosition(newClipLeft, GetLeftOffset());
      }
      else if (newClipRight != clipRight) {
         double difference = (newClipRight - clipRight);
         desiredSlideAmount += difference;
         mSnapRight = TimeToPosition(newClipRight, GetLeftOffset());
      }
   }

   // Implement sliding within the track(s)
   if (mSlideUpDownOnly) {
      desiredSlideAmount = 0.0;
   }

   //If the mouse is over a track that isn't the captured track,
   //drag the clip to the mousetrack
   if (mCapturedClip && mouseTrack != mCapturedTrack &&
       !mCapturedClipIsSelection)
   {
      // Make sure we always have the first linked track of a stereo track
      if (!mouseTrack->GetLinked() && mTracks->GetLink(mouseTrack))
         mouseTrack = (WaveTrack*)mTracks->GetLink(mouseTrack);

      // Temporary apply the offset because we want to see if the
      // track fits with the desired offset
      for(i=0; i<mCapturedClipArray.GetCount(); i++)
         if (mCapturedClipArray[i].clip)
            mCapturedClipArray[i].clip->Offset(desiredSlideAmount);
      // See if it can be moved
      if (MoveClipToTrack(mCapturedClip,
                          (WaveTrack*)mCapturedTrack, mouseTrack)) {
         mCapturedTrack = mouseTrack;
         mDidSlideVertically = true;

         // Make the offset permanent; start from a "clean slate"
         mHSlideAmount = 0.0;
         desiredSlideAmount = 0.0;
         mMouseClickX = event.m_x;
      }
      else {
         // Undo the offset
         for(i=0; i<mCapturedClipArray.GetCount(); i++)
            if (mCapturedClipArray[i].clip)
               mCapturedClipArray[i].clip->Offset(-desiredSlideAmount);
      }

      Refresh(false);
   }

   // Implement sliding within the track(s)
   if (mSlideUpDownOnly)
      return;
      
   // Determine desired amount to slide
   mHSlideAmount = desiredSlideAmount;

   if (mHSlideAmount == 0.0) {
      Refresh(false);
      return;
   }

   if (mCapturedClip) {
      double allowed;
      double initialAllowed;
      double safeBigDistance = 1000 + 2.0 * (mTracks->GetEndTime() -
                                             mTracks->GetStartTime());

      do {
         initialAllowed = mHSlideAmount;

         unsigned int i, j;
         for(i=0; i<mCapturedClipArray.GetCount(); i++) {
            WaveTrack *track = (WaveTrack *)mCapturedClipArray[i].track;
            WaveClip *clip = mCapturedClipArray[i].clip;
            
            if (clip) {
               // Move all other selected clips totally out of the way
               // temporarily because they're all moving together and
               // we want to find out if OTHER clips are in the way,
               // not one of the moving ones
               for(j=0; j<mCapturedClipArray.GetCount(); j++) {
                  WaveClip *clip2 = mCapturedClipArray[j].clip;
                  if (clip2 && clip2 != clip)
                     clip2->Offset(-safeBigDistance);
               }
               
               if (track->CanOffsetClip(clip, mHSlideAmount, &allowed)) {
                  mHSlideAmount = allowed;
               }
               else
                  mHSlideAmount = 0.0;
               
               for(j=0; j<mCapturedClipArray.GetCount(); j++) {
                  WaveClip *clip2 = mCapturedClipArray[j].clip;
                  if (clip2 && clip2 != clip)
                     clip2->Offset(safeBigDistance);
               }
            }               
         }
      } while (mHSlideAmount != initialAllowed);

      if (mHSlideAmount != 0.0) {
         unsigned int i;
         for(i=0; i<mCapturedClipArray.GetCount(); i++) {
            Track *track = mCapturedClipArray[i].track;
            WaveClip *clip = mCapturedClipArray[i].clip;
            if (clip)
               clip->Offset(mHSlideAmount);
            else
               track->Offset(mHSlideAmount);
         }
      }
   }
   else {
      // For non wavetracks...
      mCapturedTrack->Offset(mHSlideAmount);
      Track* link = mTracks->GetLink(mCapturedTrack);
      if (link)
         link->Offset(mHSlideAmount);
   }
   if (mCapturedClipIsSelection) {
      // Slide the selection, too
      mViewInfo->sel0 += mHSlideAmount;
      mViewInfo->sel1 += mHSlideAmount;
   }

   Refresh(false);
}


///  This function takes care of our different zoom 
///  possibilities.  It is possible for a user to just
///  "zoom in" or "zoom out," but it is also possible 
///  for a user to drag and select an area that he
///  or she wants to be zoomed in on.  We use mZoomStart
///  and mZoomEnd to track the beggining and end of such
///  a zoom area.  Note that the ViewInfo member
///  mViewInfo actually keeps track of our zoom constant,
///  so we achieve zooming by altering the zoom constant
///  and forcing a refresh.
void TrackPanel::HandleZoom(wxMouseEvent & event)
{
   if (event.ButtonDown() || event.LeftDClick()) {
      HandleZoomClick(event);
   }
   else if (mMouseCapture == IsZooming) {
      if (event.Dragging()) {
         HandleZoomDrag(event);
      }
      else if (event.ButtonUp()) {
         HandleZoomButtonUp(event);
      }
   }
}

/// Zoom button down, record the position.
void TrackPanel::HandleZoomClick(wxMouseEvent & event)
{
   if (mCapturedTrack)
      return;

   mCapturedTrack = FindTrack(event.m_x, event.m_y, false, false,
                              &mCapturedRect, &mCapturedNum);
   if (!mCapturedTrack)
      return;

   SetCapturedTrack(mCapturedTrack, IsZooming);

   mZoomStart = event.m_x;
   mZoomEnd = event.m_x;
}

/// Zoom drag
void TrackPanel::HandleZoomDrag(wxMouseEvent & event)
{
   int left, width, height;

   left = GetLeftOffset();
   GetTracksUsableArea(&width, &height);

   mZoomEnd = event.m_x;

   if (event.m_x < left) {
      mZoomEnd = left;
   }
   else if (event.m_x >= left + width - 1) {
      mZoomEnd = left + width - 1;
   }

   if (IsDragZooming()) {
      Refresh(false);
   }
}

/// Zoom button up
void TrackPanel::HandleZoomButtonUp(wxMouseEvent & event)
{
   if (mZoomEnd < mZoomStart) {
      int temp = mZoomEnd;
      mZoomEnd = mZoomStart;
      mZoomStart = temp;
   }

   if (IsDragZooming())
      DragZoom(event, GetLeftOffset());
   else
      DoZoomInOut(event, GetLeftOffset());

   mZoomEnd = mZoomStart = 0;

   SetCapturedTrack(NULL);

   MakeParentRedrawScrollbars();
   Refresh(false);
}

/// Determines if drag zooming is active
bool TrackPanel::IsDragZooming()
{
   return (abs(mZoomEnd - mZoomStart) > DragThreshold);
}

///  This actually sets the Zoom value when you're done doing
///  a drag zoom.
void TrackPanel::DragZoom(wxMouseEvent & event, int trackLeftEdge)
{
   double left = PositionToTime(mZoomStart, trackLeftEdge);
   double right = PositionToTime(mZoomEnd, trackLeftEdge);

   if (event.ShiftDown())
      mViewInfo->zoom /= mViewInfo->screen / (right - left);
   else
      mViewInfo->zoom *= mViewInfo->screen / (right - left);

   if (mViewInfo->zoom > gMaxZoom)
      mViewInfo->zoom = gMaxZoom;
   if (mViewInfo->zoom <= gMinZoom)
      mViewInfo->zoom = gMinZoom;

   mViewInfo->h = left;
}

/// This handles normal Zoom In/Out, if you just clicked;
/// IOW, if you were NOT dragging to zoom an area.
/// \todo MAGIC NUMBER: We've got several in this function.
void TrackPanel::DoZoomInOut(wxMouseEvent & event, int trackLeftEdge)
{
   double center_h = PositionToTime(event.m_x, trackLeftEdge);

   if (event.RightUp() || event.RightDClick() || event.ShiftDown())
      mViewInfo->zoom = wxMax(mViewInfo->zoom / 2.0, gMinZoom);
   else
      mViewInfo->zoom = wxMin(mViewInfo->zoom * 2.0, gMaxZoom);

   if (event.MiddleUp() || event.MiddleDClick())
      mViewInfo->zoom = 44100.0 / 512.0;        // AS: Reset zoom.

   double new_center_h = PositionToTime(event.m_x, trackLeftEdge);

   mViewInfo->h += (center_h - new_center_h);
}

/// Vertical zooming (triggered by clicking in the
/// vertical ruler)
void TrackPanel::HandleVZoom(wxMouseEvent & event)
{
   if (event.ButtonDown() || event.ButtonDClick()) {
      HandleVZoomClick( event );
   }
   else if (event.Dragging()) {
      HandleVZoomDrag( event );
   }
   else if (event.ButtonUp()) {
      HandleVZoomButtonUp( event );
   }
}

/// VZoom click
void TrackPanel::HandleVZoomClick( wxMouseEvent & event )
{
   if (mCapturedTrack)
      return;
   mCapturedTrack = FindTrack(event.m_x, event.m_y, true, false,
                              &mCapturedRect, &mCapturedNum);
   if (!mCapturedTrack)
      return;

#ifdef LOGARITHMIC_SPECTRUM
   // don't do anything if track is not wave or Spectrum/log Spectrum
   if (mCapturedTrack->GetKind() != Track::Wave
      || ((WaveTrack *) mCapturedTrack)->GetDisplay() > WaveTrack::SpectrumLogDisplay)
      return;
#else
   // don't do anything if track is not wave or Spectrum
   if (mCapturedTrack->GetKind() != Track::Wave
      || ((WaveTrack *) mCapturedTrack)->GetDisplay() > WaveTrack::SpectrumDisplay)
      return;
#endif
   mMouseCapture = IsVZooming;
   mZoomStart = event.m_y;
   mZoomEnd = event.m_y;
}

/// VZoom drag
void TrackPanel::HandleVZoomDrag( wxMouseEvent & event )
{
   mZoomEnd = event.m_y;
   if (IsDragZooming()){
      Refresh(false);
   }
}

/// VZoom Button up.
/// There are three cases:
///   - Drag-zooming; we already have a min and max
///   - Zoom out; ensure we don't go too small.
///   - Zoom in; ensure we don't go too large.
void TrackPanel::HandleVZoomButtonUp( wxMouseEvent & event )
{
   int minBins;
   if (!mCapturedTrack)
      return;

   mMouseCapture = IsUncaptured;

   // don't do anything if track is not wave
   if (mCapturedTrack->GetKind() != Track::Wave)
      return;

   WaveTrack *track = (WaveTrack *)mCapturedTrack;
   WaveTrack *partner = (WaveTrack *)mTracks->GetLink(track);
   int height = track->GetHeight();
   int ypos = mCapturedRect.y;

   // Ensure start and end are in order (swap if not).
   if (mZoomEnd < mZoomStart) {
      int temp = mZoomEnd;
      mZoomEnd = mZoomStart;
      mZoomStart = temp;
   }

   float min, max, c, l, binSize = 0.0;
#ifdef LOGARITHMIC_SPECTRUM
   bool spectrum, spectrumLog;
#else
   bool spectrum;
#endif
   int windowSize; 
#ifdef EXPERIMENTAL_FFT_SKIP_POINTS
   int fftSkipPoints=0; 
#endif //EXPERIMENTAL_FFT_SKIP_POINTS
   double rate = ((WaveTrack *)track)->GetRate();
   ((WaveTrack *) track)->GetDisplay() == WaveTrack::SpectrumDisplay ? spectrum = true : spectrum = false;
#ifdef LOGARITHMIC_SPECTRUM
   spectrumLog=(((WaveTrack *) track)->GetDisplay() == WaveTrack::SpectrumLogDisplay);
#endif
   if(spectrum) {
      min = gPrefs->Read(wxT("/Spectrum/MinFreq"), 0L);
      if(min < 0)
         min = 0;
      max = gPrefs->Read(wxT("/Spectrum/MaxFreq"), 8000L);
      if(max > rate/2.)
         max = rate/2.;
      windowSize = gPrefs->Read(wxT("/Spectrum/FFTSize"), 256);
#ifdef EXPERIMENTAL_FFT_SKIP_POINTS
      fftSkipPoints = gPrefs->Read(wxT("/Spectrum/FFTSkipPoints"), 0L);
#endif //EXPERIMENTAL_FFT_SKIP_POINTS
      binSize = rate / windowSize;
      minBins = wxMin(10, windowSize/2); //minimum 10 freq bins, unless there are less
   }
   else
#ifdef LOGARITHMIC_SPECTRUM
      if(spectrumLog) {
         min = gPrefs->Read(wxT("/SpectrumLog/MinFreq"), rate/2000.0);
         if(min < 1)
            min = 1;
         max = gPrefs->Read(wxT("/SpectrumLog/MaxFreq"), rate/2.);
         if(max > rate/2.)
            max = rate/2.;
         windowSize = gPrefs->Read(wxT("/Spectrum/FFTSize"), 256);
#ifdef EXPERIMENTAL_FFT_SKIP_POINTS
         fftSkipPoints = gPrefs->Read(wxT("/Spectrum/FFTSkipPoints"), 0L);
#endif //EXPERIMENTAL_FFT_SKIP_POINTS
         binSize = rate / windowSize;
         minBins = wxMin(10, windowSize/2); //minimum 10 freq bins, unless there are less
      }
      else
         track->GetDisplayBounds(&min, &max);
#else
      track->GetDisplayBounds(&min, &max);
#endif
   if (IsDragZooming()) {
      // Drag Zoom
      float p1, p2, tmin, tmax;
      tmin=min;
      tmax=max;

#ifdef LOGARITHMIC_SPECTRUM
      if(spectrumLog) {
         double xmin = 1-(mZoomEnd - ypos) / (float)height;
         double xmax = 1-(mZoomStart - ypos) / (float)height;
         double lmin=log10(tmin), lmax=log10(tmax);
         double d=lmax-lmin;
         min=wxMax(1.0, pow(10, xmin*d+lmin));
         max=wxMin(rate/2.0, pow(10, xmax*d+lmin));
         // Enforce vertical zoom limits
         // done in the linear freq domain for now, but not too far out
         if(max < min + minBins * binSize)
            max = min + minBins * binSize;
         if(max > rate/2.) {
            max = rate/2.;
            min = max - minBins * binSize;
         }
      }
      else {
         p1 = (mZoomStart - ypos) / (float)height;
         p2 = (mZoomEnd - ypos) / (float)height;
         max = (tmax * (1.0-p1) + tmin * p1);
         min = (tmax * (1.0-p2) + tmin * p2);

         // Enforce vertical zoom limits
         if(spectrum) {
            if(min < 0.)
               min = 0.;
            if(max < min + minBins * binSize)
               max = min + minBins * binSize;
            if(max > rate/2.) {
               max = rate/2.;
               min = max - minBins * binSize;
            }
         }
         else {
            if (max - min < 0.2) {
               c = (min+max)/2;
               min = c-0.1;
               max = c+0.1;
            }
         }
      }
#else
      p1 = (mZoomStart - ypos) / (float)height;
      p2 = (mZoomEnd - ypos) / (float)height;
      max = (tmax * (1.0-p1) + tmin * p1);
      min = (tmax * (1.0-p2) + tmin * p2);

      // Enforce vertical zoom limits
      if(spectrum) {
         if(min < 0.)
            min = 0.;
         if(max < min + minBins * binSize)
            max = min + minBins * binSize;
         if(max > rate/2.) {
            max = rate/2.;
            min = max - minBins * binSize;
         }
      }
      else {
         if (max - min < 0.2) {
            c = (min+max)/2;
            min = c-0.1;
            max = c+0.1;
         }
      }
#endif
   }
   else if (event.ShiftDown() || event.RightUp()) {
      // Zoom OUT
      // Zoom out to -1.0...1.0 first, then, and only
      // then, if they click again, allow one more
      // zoom out.
      if(spectrum) {
         c = 0.5*(min+max);
         l = (c - min);
         if(c - 2*l <= 0) {
            min = 0.0;
            max = wxMin( rate/2., 2. * max);
         }
         else {
            min = wxMax( 0.0, c - 2*l);
            max = wxMin( rate/2., c + 2*l);
         }
      }
#ifdef LOGARITHMIC_SPECTRUM
      else {
         if(spectrumLog)
		   {
            float p1;
            p1 = (mZoomStart - ypos) / (float)height;
            c = 1.0-p1;
            double xmin = c - 1.;
            double xmax = c + 1.;
            double lmin = log10(min), lmax = log10(max);
            double d = lmax-lmin;
            min = wxMax(1,pow(10, xmin*d+lmin));
            max = wxMin(rate/2., pow(10, xmax*d+lmin));
         }
         else {
            if (min <= -1.0 && max >= 1.0) {
               min = -2.0;
               max = 2.0;
            }
            else {
               c = 0.5*(min+max);
               l = (c - min);
               min = wxMax( -1.0, c - 2*l);
               max = wxMin(  1.0, c + 2*l);
            }
         }
      }
#else
      else {
         if (min <= -1.0 && max >= 1.0) {
            min = -2.0;
            max = 2.0;
         }
         else {
            c = 0.5*(min+max);
            l = (c - min);
            min = wxMax( -1.0, c - 2*l);
            max = wxMin(  1.0, c + 2*l);
         }
      }
#endif
   }
   else {
      // Zoom IN
      float p1;
      if(spectrum) {
         c = 0.5*(min+max);
         // Enforce maximum vertical zoom
         l = wxMax( minBins * binSize, (c - min));

         p1 = (mZoomStart - ypos) / (float)height;
         c = (max * (1.0-p1) + min * p1);
         min = wxMax( 0.0, c - 0.5*l);
         max = wxMin( rate/2., min + l);
      }
      else {
#ifdef LOGARITHMIC_SPECTRUM
         if(spectrumLog) {
            p1 = (mZoomStart - ypos) / (float)height;
            c = 1.0-p1;
            double xmin = c - 0.25;
            double xmax = c + 0.25;
            double lmin = log10(min), lmax = log10(max);
            double d = lmax-lmin;
            min = wxMax(1,pow(10, xmin*d+lmin));
            max = wxMin(rate/2., pow(10, xmax*d+lmin));
            // Enforce vertical zoom limits
            // done in the linear freq domain for now, but not too far out
            if(max < min + minBins * binSize)
               max = min + minBins * binSize;
            if(max > rate/2.) {
               max = rate/2.;
               min = max - minBins * binSize;
            }
         }
         else {
            // Zoom in centered on cursor
            if (min < -1.0 || max > 1.0) {
               min = -1.0;
               max = 1.0;
            }
            else {
               c = 0.5*(min+max);
               // Enforce maximum vertical zoom
               l = wxMax( 0.1, (c - min));

               p1 = (mZoomStart - ypos) / (float)height;
               c = (max * (1.0-p1) + min * p1);
               min = c - 0.5*l;
               max = c + 0.5*l;
            }
         }
#else
         // Zoom in centered on cursor
         if (min < -1.0 || max > 1.0) {
            min = -1.0;
            max = 1.0;
         }
         else {
            c = 0.5*(min+max);
            // Enforce maximum vertical zoom
            l = wxMax( 0.1, (c - min));

            p1 = (mZoomStart - ypos) / (float)height;
            c = (max * (1.0-p1) + min * p1);
            min = c - 0.5*l;
            max = c + 0.5*l;
         }
#endif
      }
   }

   if(spectrum) {
      gPrefs->Write(wxT("/Spectrum/MaxFreq"), (long)max);
      gPrefs->Write(wxT("/Spectrum/MinFreq"), (long)min);
      TrackListIterator iter(mTracks);
      Track *t = iter.First();
      while (t) {
         if (t->GetKind() == Track::Wave) {
            WaveTrack *wt = (WaveTrack *)t;
            WaveClipList::Node* it;
            for(it=wt->GetClipIterator(); it; it=it->GetNext()) {
               WaveClip *clip = it->GetData();
               clip->mSpecPxCache->valid = false;
            }
         }
         t = iter.Next();
      }
   }
#ifdef LOGARITHMIC_SPECTRUM
   else
      if(spectrumLog) {
         gPrefs->Write(wxT("/SpectrumLog/MaxFreq"), (long)max);
         gPrefs->Write(wxT("/SpectrumLog/MinFreq"), (long)min);
         TrackListIterator iter(mTracks);
         Track *t = iter.First();
         while (t) {
            if (t->GetKind() == Track::Wave) {
               WaveTrack *wt = (WaveTrack *)t;
               WaveClipList::Node* it;
               for(it=wt->GetClipIterator(); it; it=it->GetNext()) {
                  WaveClip *clip = it->GetData();
                  clip->mSpecPxCache->valid = false;
               }
            }
            t = iter.Next();
         }
      }
#endif
   else {
      track->SetDisplayBounds(min, max);
      if (partner)
         partner->SetDisplayBounds(min, max);
   }

   mZoomEnd = mZoomStart = 0;
   Refresh(false);
   mCapturedTrack = NULL;
   MakeParentModifyState();
}

/// Determines if we can edit samples in a wave track.
/// Also pops up warning messages in certain cases where we can't.
///  @return true if we can edit the samples, false otherwise.
bool TrackPanel::IsSampleEditingPossible( wxMouseEvent & event, Track * t )
{
   //Exit if we don't have a track
   if(!t)
      return false;
   
   //Exit if it's not a WaveTrack
   if(t->GetKind() != Track::Wave)
      return false;
   
   //Get out of here if we shouldn't be drawing right now:
   //If we aren't displaying the waveform, Display a message dialog
   if(((WaveTrack *)t)->GetDisplay() != WaveTrack::WaveformDisplay)
   {
      wxMessageBox(_("Draw currently only works with linear-view waveforms."), wxT("Notice"));
      return false;
   }
   
   //Get rate in order to calculate the critical zoom threshold
   //Find out the zoom level
   double rate = ((WaveTrack *)t)->GetRate();
   bool showPoints = (mViewInfo->zoom / rate > 3.0);

   //If we aren't zoomed in far enough, show a message dialog.
   if(!showPoints)
   {
      // Release capture so user will be able to click OK on Linux
      bool hadCapture = HasCapture();
      if (hadCapture)
         ReleaseMouse();
         
      wxMessageBox(_("You are not zoomed in enough. Zoom in until you can see the individual samples."), wxT("Notice"));
      
      // Re-aquire capture
      if (hadCapture)
         CaptureMouse();
      return false;
   }
   return true;
}

/// We're in a track view and zoomed enough to see the samples.
/// Someone has just clicked the mouse.  What do we do?
void TrackPanel::HandleSampleEditingClick( wxMouseEvent & event )
{
   //declare a rectangle to determine clicking position
   wxRect r;
   int dummy;
   Track *t;

   //Get the track the mouse is over, and save it away for future events
   mDrawingTrack = NULL;
   t = FindTrack(event.m_x, event.m_y, false, false, &r, &dummy);
   
   if( !IsSampleEditingPossible( event, t ) )
   {
      if( HasCapture() )
         ReleaseMouse();
      return;
   }

   /// \todo Should mCapturedTrack take the place of mDrawingTrack??
   mDrawingTrack = t;
   mDrawingTrackTop=r.y;

   //If we are still around, we are drawing in earnest.  Set some member data structures up:
   //First, calculate the starting sample.  To get this, we need the time
   double t0 = PositionToTime(event.m_x, GetLeftOffset());
   double rate = ((WaveTrack *)mDrawingTrack)->GetRate();
   float newLevel;   //Declare this for use later

   //convert t0 to samples
   mDrawingStartSample = (sampleCount) (double)(t0 * rate + 0.5 );
   
   //Now, figure out what the value of that sample is.
   //First, get the sequence of samples so you can mess with it
   //Sequence *seq = ((WaveTrack *)mDrawingTrack)->GetSequence();


   //Determine how drawing should occur.  If alt is down, 
   //do a smoothing, instead of redrawing.
   if( event.m_altDown ) 
   {
      //*************************************************
      //***  ALT-DOWN-CLICK (SAMPLE SMOOTHING)        ***
      //*************************************************
      //
      //  Smoothing works like this:  There is a smoothing kernel radius constant that
      //  determines how wide the averaging window is.  Plus, there is a smoothing brush radius, 
      //  which determines how many pixels wide around the selected pixel this smoothing is applied.
      //
      //  Samples will be replaced by a mixture of the original points and the smoothed points, 
      //  with a triangular mixing probability whose value at the center point is 
      //  SMOOTHING_PROPORTION_MAX and at the far bounds is SMOOTHING_PROPORTION_MIN

      //Get the region of samples around the selected point
      int sampleRegionSize = 1 + 2 * (SMOOTHING_KERNEL_RADIUS + SMOOTHING_BRUSH_RADIUS);
      float *sampleRegion = new float[sampleRegionSize];
      float * newSampleRegion = new float[1 + 2 * SMOOTHING_BRUSH_RADIUS];
      
      //Get a sample  from the track to do some tricks on.
      ((WaveTrack*)mDrawingTrack)->Get((samplePtr)sampleRegion, floatSample, 
                                       (int)mDrawingStartSample - SMOOTHING_KERNEL_RADIUS - SMOOTHING_BRUSH_RADIUS,
                                       sampleRegionSize);
      int i, j;

      //Go through each point of the smoothing brush and apply a smoothing operation.
      for(j = -SMOOTHING_BRUSH_RADIUS; j <= SMOOTHING_BRUSH_RADIUS; j++){
         float sumOfSamples = 0;
         for (i= -SMOOTHING_KERNEL_RADIUS; i <= SMOOTHING_KERNEL_RADIUS; i++){
            //Go through each point of the smoothing kernel and find the average
            
            //The average is a weighted average, scaled by a weighting kernel that is simply triangular
            // A triangular kernel across N items, with a radius of R ( 2 R + 1 points), if the farthest:
            // points have a probability of a, the entire triangle has total probability of (R + 1)^2.
            //      For sample number i and middle brush sample M,  (R + 1 - abs(M-i))/ ((R+1)^2) gives a 
            //   legal distribution whose total probability is 1.
            //
            //
            //                weighting factor                       value  
            sumOfSamples += (SMOOTHING_KERNEL_RADIUS + 1 - abs(i)) * sampleRegion[i + j + SMOOTHING_KERNEL_RADIUS + SMOOTHING_BRUSH_RADIUS];

         }
         newSampleRegion[j + SMOOTHING_BRUSH_RADIUS] = sumOfSamples/((SMOOTHING_KERNEL_RADIUS + 1) *(SMOOTHING_KERNEL_RADIUS + 1) );
      }
      

      // Now that the new sample levels are determined, go through each and mix it appropriately
      // with the original point, according to a 2-part linear function whose center has probability
      // SMOOTHING_PROPORTION_MAX and extends out SMOOTHING_BRUSH_RADIUS, at which the probability is
      // SMOOTHING_PROPORTION_MIN.  _MIN and _MAX specify how much of the smoothed curve make it through.
      
      float prob;
      
      for(j=-SMOOTHING_BRUSH_RADIUS; j <= SMOOTHING_BRUSH_RADIUS; j++){

         prob = SMOOTHING_PROPORTION_MAX - (float)abs(j)/SMOOTHING_BRUSH_RADIUS * (SMOOTHING_PROPORTION_MAX - SMOOTHING_PROPORTION_MIN);

         newSampleRegion[j+SMOOTHING_BRUSH_RADIUS] =
            newSampleRegion[j + SMOOTHING_BRUSH_RADIUS] * prob + 
            sampleRegion[SMOOTHING_BRUSH_RADIUS + SMOOTHING_KERNEL_RADIUS + j] * (1 - prob);
      }
      //Set the sample to the point of the mouse event
      ((WaveTrack*)mDrawingTrack)->Set((samplePtr)newSampleRegion, floatSample, mDrawingStartSample - SMOOTHING_BRUSH_RADIUS, 1 + 2 * SMOOTHING_BRUSH_RADIUS);

      //Clean this up right away to avoid a memory leak
      delete[] sampleRegion;
      delete[] newSampleRegion;
   } 
   else 
   {
      //*************************************************
      //***   PLAIN DOWN-CLICK (NORMAL DRAWING)       ***
      //*************************************************

      //Otherwise (e.g., the alt button is not down) do normal redrawing, based on the mouse position.
      // Calculate where the mouse is located vertically (between +/- 1)
      ((WaveTrack*)mDrawingTrack)->Get((samplePtr)&mDrawingStartSampleValue, floatSample,(int) mDrawingStartSample, 1);
      float zoomMin, zoomMax;
      ((WaveTrack *)mDrawingTrack)->GetDisplayBounds(&zoomMin, &zoomMax);
      newLevel = zoomMax -
         ((event.m_y - mDrawingTrackTop)/(float)mDrawingTrack->GetHeight()) *
         (zoomMax - zoomMin);

      //Take the envelope into account
      Envelope *env = ((WaveTrack *)mDrawingTrack)->GetEnvelopeAtX(event.GetX());

      if (env)
      {
         double envValue = env->GetValue(t0);
         if (envValue > 0)
            newLevel /= envValue;
         else
            newLevel = 0;
   
         //Make sure the new level is between +/-1
         newLevel = newLevel >  1.0 ?  1.0: newLevel;
         newLevel = newLevel < -1.0 ? -1.0: newLevel;
      }

      //Set the sample to the point of the mouse event
      ((WaveTrack*)mDrawingTrack)->Set((samplePtr)&newLevel, floatSample, mDrawingStartSample, 1);
   }

   //Set the member data structures for drawing
   mDrawingLastDragSample=mDrawingStartSample;
   mDrawingLastDragSampleValue = newLevel;

   //Redraw the region of the selected track
   Refresh(false);
}

void TrackPanel::HandleSampleEditingDrag( wxMouseEvent & event )
{
   //*************************************************
   //***    DRAG-DRAWING                           ***
   //*************************************************

   //The following will happen on a drag or a down-click.
   // The point should get re-drawn at the location of the mouse.
   //Exit if the mDrawingTrack is null.
   if( mDrawingTrack == NULL)
      return;
   
   //Exit dragging if the alt key is down--Don't allow left-right dragging for smoothing operation
   if (event.m_altDown)
      return;

   //Get the rate of the sequence, for use later
   float rate = ((WaveTrack *)mDrawingTrack)->GetRate();
   sampleCount s0;     //declare this for use below.  It designates the sample number which to draw.

   // Figure out what time the click was at
   double t0 = PositionToTime(event.m_x, GetLeftOffset());
   float newLevel;
   //Find the point that we want to redraw at. If the control button is down, 
   //adjust only the originally clicked-on sample 

   //*************************************************
   //***   CTRL-DOWN (Hold Initial Sample Constant ***
   //*************************************************

   if( event.m_controlDown) {
      s0 = mDrawingStartSample;
   } 
   else 
   {
      //*************************************************
      //***    Normal CLICK-drag  (Normal drawing)    ***
      //*************************************************

      //Otherwise, adjust the sample you are dragging over right now.
      //convert this to samples
      s0 = (sampleCount) (double)(t0 * rate + 0.5);
   }

   //Sequence *seq = ((WaveTrack *)mDrawingTrack)->GetSequence();
   ((WaveTrack*)mDrawingTrack)->Get((samplePtr)&mDrawingStartSampleValue, floatSample, (int)mDrawingStartSample, 1);
   
   //Otherwise, do normal redrawing, based on the mouse position.
   // Calculate where the mouse is located vertically (between +/- 1)


   float zoomMin, zoomMax;
   ((WaveTrack *)mDrawingTrack)->GetDisplayBounds(&zoomMin, &zoomMax);
   newLevel = zoomMax -
      ((event.m_y - mDrawingTrackTop)/(float)mDrawingTrack->GetHeight()) *
      (zoomMax - zoomMin);

   //Take the envelope into account
   Envelope *env = ((WaveTrack *)mDrawingTrack)->GetEnvelopeAtX(event.GetX());
   if (env)
   {
      double envValue = env->GetValue(t0);
      if (envValue > 0)
         newLevel /= envValue;
      else
         newLevel = 0;

      //Make sure the new level is between +/-1
      newLevel = newLevel >  1.0 ?  1.0: newLevel;
      newLevel = newLevel < -1.0 ? -1.0: newLevel;
   }

   //Now, redraw all samples between current and last redrawn sample
   
   float tmpvalue;
   //Handle cases of 0 or 1 special, to improve speed
   //JKC I don't think this makes any noticeable difference to speed
   // whatsoever!  The real reason for the special case is probably to 
   // avoid division by zero....
   if(abs(s0 - mDrawingLastDragSample) <= 1){
      ((WaveTrack*)mDrawingTrack)->Set((samplePtr)&newLevel,  floatSample, s0, 1);         
   }
   else 
   {
      //Go from the smaller to larger sample. 
      int start = wxMin( s0, mDrawingLastDragSample) +1;
      int end   = wxMax( s0, mDrawingLastDragSample);
      for(sampleCount i= start; i<= end; i++) {
         //This interpolates each sample linearly:
         tmpvalue=mDrawingLastDragSampleValue + (newLevel - mDrawingLastDragSampleValue)  * 
            (float)(i-mDrawingLastDragSample)/(s0-mDrawingLastDragSample );
         ((WaveTrack*)mDrawingTrack)->Set((samplePtr)&tmpvalue, floatSample, i, 1);
      }
   }
   //Update the member data structures.
   mDrawingLastDragSample=s0;
   mDrawingLastDragSampleValue = newLevel;

   //Redraw the region of the selected track
   Refresh(false);
}

void TrackPanel::HandleSampleEditingButtonUp( wxMouseEvent & event )
{
   //*************************************************
   //***    UP-CLICK  (Finish drawing)             ***
   //*************************************************
   
   //On up-click, send the state to the undo stack
   mDrawingTrack=NULL;       //Set this to NULL so it will catch improper drag events.
   MakeParentPushState(_("Moved Sample"),
                       _("Sample Edit"),
                       true /* consolidate */);
}


/// This handles adjusting individual samples by hand using the draw tool(s)
///
/// There are several member data structure for handling drawing:
///  - mDrawingTrack:               keeps track of which track you clicked down on, so drawing doesn't 
///                                 jump to a new track
///  - mDrawingTrackTop:            The top position of the drawing track--makes drawing easier.
///  - mDrawingStartSample:         The sample you clicked down on, so that you can hold it steady
///  - mDrawingStartSampleValue:    The original value of the initial sample
///  - mDrawingLastDragSample:      When drag-drawing, this keeps track of the last sample you dragged over,
///                                 so it can smoothly redraw samples that got skipped over
///  - mDrawingLastDragSampleValue: The value of the last 
void TrackPanel::HandleSampleEditing(wxMouseEvent & event)
{
   if (event.LeftDown() ) {
      HandleSampleEditingClick( event);
   } else if (mDrawingTrack && event.Dragging()) {
      HandleSampleEditingDrag( event );
   }  else if(mDrawingTrack && event.ButtonUp()) {
      HandleSampleEditingButtonUp( event );
   }
}


// This is for when a given track gets the x.
void TrackPanel::HandleClosing(wxMouseEvent & event)
{
   AudacityProject *p = GetProject(); //lda

   Track *t = mCapturedTrack;
   wxRect r = mCapturedRect;

   wxRect closeRect;
   mTrackInfo.GetCloseBoxRect(r, closeRect);

   wxClientDC dc(this);

   if (event.Dragging())
      mTrackInfo.DrawCloseBox(&dc, r, closeRect.Inside(event.m_x, event.m_y));
   else if (event.LeftUp()) {
      mTrackInfo.DrawCloseBox(&dc, r, false);
      if (closeRect.Inside(event.m_x, event.m_y)) {
         if (!gAudioIO->IsStreamActive(p->GetAudioIOToken()))
            RemoveTrack(t);
      }
      SetCapturedTrack( NULL );
   }
   // BG: There are no more tracks on screen
   if (mTracks->IsEmpty()) {
      //BG: Set zoom to normal
      mViewInfo->zoom = 44100.0 / 512.0;

      //STM: Set selection to 0,0
      mViewInfo->sel0 = 0.0;
      mViewInfo->sel1 = 0.0;

      mListener->TP_RedrawScrollbars();
      mListener->TP_DisplayStatusMessage(wxT("")); //STM: Clear message if all tracks are removed
      
      Refresh(false);
   }
}

/// Removes the specified track.  Called from HandleClosing.
void TrackPanel::RemoveTrack(Track * toRemove)
{
   TrackListIterator iter(mTracks);
   Track *t;

   // If it was focused, reassign focus to the next or, if
   // unavailable, the previous track.
   if (GetFocusedTrack() == toRemove) {
      t = mTracks->GetNext(toRemove, true);
      if (t == NULL)
         t = mTracks->GetPrev( toRemove, true );
      SetFocusedTrack(t);  // It's okay if this is NULL
   }

   Track *partner = mTracks->GetLink(toRemove);
   wxString name;

   t = iter.First();
   while (t) {
      if (t == toRemove || t == partner) {
         name = t->GetName();
         delete t;
         t = iter.RemoveCurrent();
      } else
         t = iter.Next();
   }

   if( iter.First() == NULL )
      SetFocusedTrack( NULL );

   MakeParentPushState(
      wxString::Format(_("Removed track '%s.'"),
      name.c_str()),
      _("Track Remove"));
   MakeParentRedrawScrollbars();
   MakeParentResize();
   Refresh(false);
}

void TrackPanel::HandlePopping(wxMouseEvent & event)
{
   Track *t = mCapturedTrack;
   wxRect r = mCapturedRect;

   if( t==NULL ){
      SetCapturedTrack( NULL );
      return;
   }

   wxRect titleRect;
   mTrackInfo.GetTitleBarRect(r, titleRect);

   wxClientDC dc(this);

   if (event.Dragging()) {
      mTrackInfo.DrawTitleBar(&dc, r, t, titleRect.Inside(event.m_x, event.m_y));
   }
   else if (event.LeftUp()) {
      if (titleRect.Inside(event.m_x, event.m_y))
      {
         OnTrackMenu(t);
      }

      SetCapturedTrack( NULL );

      mTrackInfo.DrawTitleBar(&dc, r, t, false);
   }
}

/// Handle when the mute or solo button is pressed for some track.
void TrackPanel::HandleMutingSoloing(wxMouseEvent & event, bool solo)
{
   Track *t = mCapturedTrack;
   wxRect r = mCapturedRect;

   if( t==NULL ){
      wxASSERT(false);// Soloing or muting but no captured track!
      SetCapturedTrack( NULL );
      return;
   }

   wxRect buttonRect;
   mTrackInfo.GetMuteSoloRect(r, buttonRect, solo, HasSoloButton());

   wxClientDC dc(this);

   if (event.Dragging()){
         mTrackInfo.DrawMuteSolo(&dc, r, t, buttonRect.Inside(event.m_x, event.m_y),
                   solo, HasSoloButton());
   }
   else if (event.LeftUp() )
      {
         
         if (buttonRect.Inside(event.m_x, event.m_y)) 
            {
               if(solo)
                  
                  OnTrackSolo(event.ShiftDown(),t);
               else
                  OnTrackMute(event.ShiftDown(),t);
            }  
         SetCapturedTrack( NULL );
         // mTrackInfo.DrawMuteSolo(&dc, r, t, false, solo);
         Refresh(false);
      }
}

void TrackPanel::HandleMinimizing(wxMouseEvent & event)
{
   Track *t = mCapturedTrack;
   wxRect r = mCapturedRect;

   if( t==NULL ){
      SetCapturedTrack( NULL );
      return;
   }

   wxRect buttonRect;
   mTrackInfo.GetMinimizeRect(r, buttonRect, t->GetMinimized());

   wxClientDC dc(this);

   if (event.Dragging()) {
      mTrackInfo.DrawMinimize(&dc, r, t, buttonRect.Inside(event.m_x, event.m_y), t->GetMinimized());
   }
   else if (event.LeftUp()) {
      if (buttonRect.Inside(event.m_x, event.m_y))
      {
         t->SetMinimized(!t->GetMinimized());
         if (mTracks->GetLink(t))
            mTracks->GetLink(t)->SetMinimized(t->GetMinimized());
         MakeParentRedrawScrollbars();
         MakeParentModifyState();
      }

      SetCapturedTrack( NULL );

      mTrackInfo.DrawMinimize(&dc, r, t, false, t->GetMinimized());
      Refresh(false);
   }
}

void TrackPanel::HandleSliders(wxMouseEvent &event, bool pan)
{
   LWSlider *slider;

   if (pan)
      slider = mTrackInfo.mPans[mCapturedNum];
   else
      slider = mTrackInfo.mGains[mCapturedNum];

   slider->OnMouseEvent(event);

   //If we have a double-click, do this...
   if(event.LeftDClick())
      {
         mMouseCapture = IsUncaptured;
      }

   float newValue = slider->Get();
   WaveTrack *link = (WaveTrack *)mTracks->GetLink(mCapturedTrack);
   
   if (pan) {
      ((WaveTrack *)mCapturedTrack)->SetPan(newValue);
      if (link)
         link->SetPan(newValue);
   }
   else {
      ((WaveTrack *)mCapturedTrack)->SetGain(newValue);
      if (link)
         link->SetGain(newValue);
   }
   
   if (event.ButtonUp()) {
      MakeParentPushState(pan ? _("Moved pan slider") : _("Moved gain slider"),
                          pan ? _("Pan") : _("Gain"),
                          true /* consolidate */);
      SetCapturedTrack( NULL );
   }

   this->Refresh(false);
}

void TrackPanel::OnContextMenu(wxContextMenuEvent & event)
{
   OnTrackMenu();
}

/// This handles when the user clicks on the "Label" area
/// of a track, ie the part with all the buttons and the drop
/// down menu, etc.
void TrackPanel::HandleLabelClick(wxMouseEvent & event)
{
   // AS: If not a click, ignore the mouse event.
   if (!(event.LeftDown() || event.LeftDClick()))
      return;

   AudacityProject *p = GetProject();
   bool unsafe = (p->GetAudioIOToken()>0 &&
                  gAudioIO->IsStreamActive(p->GetAudioIOToken()));

   wxRect r;
   int num;

   Track *t = FindTrack(event.m_x, event.m_y, true, true, &r, &num);

   // AS: If the user clicked outside all tracks, make nothing
   //  selected.
   if (!t) {
      SelectNone();
      Refresh(false);
      return;
   }

   // LL: Check close box
   if (CloseFunc(t, r, event.m_x, event.m_y))
      return;

   // LL: Check title bar for popup
   if (PopupFunc(t, r, event.m_x, event.m_y))
      return;

   // MM: Check minimize buttons on WaveTracks. Must be before
   //     solo/mute buttons, sliders etc.
   if (MinimizeFunc(t, r, event.m_x, event.m_y))
      return;

   if (t->GetKind() == Track::Wave)
   {
      // DM: Check Mute and Solo buttons on WaveTracks:
      if (MuteSoloFunc(t, r, event.m_x, event.m_y, false) ||
         MuteSoloFunc(t, r, event.m_x, event.m_y, true))
         return;

      if (GainFunc(t, r, event,
                  num-1, event.m_x, event.m_y))
         return;

      if (PanFunc(t, r, event,
                  num-1, event.m_x, event.m_y))
         return;
   }
   // DM: If it's a NoteTrack, it has special controls
   else if (t->GetKind() == Track::Note)
   {
      wxRect midiRect;
      mTrackInfo.GetTrackControlsRect(r, midiRect);
      if (midiRect.Inside(event.m_x, event.m_y)) {
         ((NoteTrack *) t)->LabelClick(midiRect, event.m_x, event.m_y,
                                       event.RightDown()
                                       || event.RightDClick());
         Refresh(false);
         return;
      }
   }

   // DM: If they weren't clicking on a particular part of a track label,
   //  deselect other tracks and select this one.

   // JH: also, capture the current track for rearranging, so the user
   //  can drag the track up or down to swap it with others
   if (!unsafe) {
      SetCapturedTrack( t, IsRearranging );
      TrackPanel::CalculateRearrangingThresholds(event);
   }

   // AS: If the shift botton is being held down, then just invert 
   //  the selection on this track.
   if (event.ShiftDown()) {
      mTracks->Select(t, !t->GetSelected());
      Refresh(false);
      return;
   }

   SelectNone();
   mTracks->Select(t);
   SetFocusedTrack(t);
   mViewInfo->sel0 = t->GetOffset();
   mViewInfo->sel1 = t->GetEndTime();

   Refresh(false);  
   if (!unsafe)
      MakeParentModifyState();
}

/// The user is dragging one of the tracks: change the track order
/// accordingly
void TrackPanel::HandleRearrange(wxMouseEvent & event)
{
   // are we finishing the drag?
   if (event.LeftUp()) {
      SetCapturedTrack( NULL );
      SetCursor(*mArrowCursor);
      return;
   }

   wxString dir;

   if (event.m_y < mMoveUpThreshold) {
      mTracks->MoveUp(mCapturedTrack);
      dir = _("up");
   }
   else if (event.m_y > mMoveDownThreshold) {
      mTracks->MoveDown(mCapturedTrack);
      dir = _("down");
   }
   else
   {
      return;
   }

   MakeParentPushState(wxString::Format(_("Moved '%s' %s"),
                                        mCapturedTrack->GetName().c_str(),
                                        dir.c_str()),
                       _("Move Track"));

   // JH: if we moved up or down, recalculate the thresholds
   TrackPanel::CalculateRearrangingThresholds(event);
   Refresh(false);
}

/// Figure out how far the user must drag the mouse up or down
/// before the track will swap with the one above or below
void TrackPanel::CalculateRearrangingThresholds(wxMouseEvent & event)
{
   wxASSERT(mCapturedTrack);

   // JH: this will probably need to be tweaked a bit, I'm just
   //   not sure what formula will have the best feel for the
   //   user.
   if (mTracks->CanMoveUp(mCapturedTrack))
      mMoveUpThreshold =
          event.m_y - mTracks->GetGroupHeight( mTracks->GetPrev(mCapturedTrack,true) );
   else
      mMoveUpThreshold = INT_MIN;

   if (mTracks->CanMoveDown(mCapturedTrack))
      mMoveDownThreshold =
          event.m_y + mTracks->GetGroupHeight( mTracks->GetNext(mCapturedTrack,true) );
   else
      mMoveDownThreshold = INT_MAX;
//   wxLogDebug(wxT("Y:%i, Up:%i, Down:%i"),
//      event.m_y, mMoveUpThreshold-event.m_y, mMoveDownThreshold-event.m_y );
}

bool TrackPanel::GainFunc(Track * t, wxRect r, wxMouseEvent &event,
                          int index, int x, int y)
{
   wxRect sliderRect;
   mTrackInfo.GetGainRect(r, sliderRect);
   if (!sliderRect.Inside(x, y)) 
      return false;

   SetCapturedTrack( t, IsGainSliding);
   mCapturedRect = r;
   mCapturedNum = index;
   HandleSliders(event, false);

   return true;
}

bool TrackPanel::PanFunc(Track * t, wxRect r, wxMouseEvent &event,
                         int index, int x, int y)
{
   wxRect sliderRect;
   mTrackInfo.GetPanRect(r, sliderRect);
   if (!sliderRect.Inside(x, y))
      return false;

   SetCapturedTrack( t, IsPanSliding);
   mCapturedRect = r;
   mCapturedNum = index;
   HandleSliders(event, true);

   return true;
}

/// Mute or solo the given track (t).  If solo is true, we're 
/// soloing, otherwise we're muting.  Basically, check and see 
/// whether x and y fall within the  area of the appropriate button.
bool TrackPanel::MuteSoloFunc(Track * t, wxRect r, int x, int y,
                              bool solo)
{
   wxRect buttonRect;
   mTrackInfo.GetMuteSoloRect(r, buttonRect, solo, HasSoloButton());
   if (!buttonRect.Inside(x, y)) 
      return false;

   wxClientDC dc(this);
   SetCapturedTrack( t, solo ? IsSoloing : IsMuting);
   mCapturedRect = r;

   mTrackInfo.DrawMuteSolo(&dc, r, t, true, solo, HasSoloButton());
   return true;
}

bool TrackPanel::MinimizeFunc(Track * t, wxRect r, int x, int y)
{
   wxRect buttonRect;
   mTrackInfo.GetMinimizeRect(r, buttonRect, t->GetMinimized());
   if (!buttonRect.Inside(x, y)) 
      return false;

   wxClientDC dc(this);
   SetCapturedTrack( t, IsMinimizing );
   mCapturedRect = r;

   mTrackInfo.DrawMinimize(&dc, r, t, true, t->GetMinimized());
   return true;
}

bool TrackPanel::CloseFunc(Track * t, wxRect r, int x, int y)
{
   wxRect closeRect;
   mTrackInfo.GetCloseBoxRect(r, closeRect);

   if (!closeRect.Inside(x, y))
      return false;

   wxClientDC dc(this);
   SetCapturedTrack( t, IsClosing );
   mCapturedRect = r;

   mTrackInfo.DrawCloseBox(&dc, r, true);
   return true;
}

bool TrackPanel::PopupFunc(Track * t, wxRect r, int x, int y)
{
   wxRect titleRect;
   mTrackInfo.GetTitleBarRect(r, titleRect);
   if (!titleRect.Inside(x, y)) 
      return false;

   wxClientDC dc(this);
   SetCapturedTrack( t, IsPopping );
   mCapturedRect = r;

   mTrackInfo.DrawTitleBar(&dc, r, t, true);
   return true;
}

///  ButtonDown means they just clicked and haven't released yet.
///  We use this opportunity to save which track they clicked on,
///  and the initial height of the track, so as they drag we can
///  update the track size.
void TrackPanel::HandleResizeClick( wxMouseEvent & event )
{
   wxRect r;
   wxRect rLabel;
   int num;

   // DM: Figure out what track is about to be resized
   Track *t = FindTrack(event.m_x, event.m_y, false, false, &r, &num);
   Track *label = FindTrack(event.m_x, event.m_y, true, true, &rLabel, &num);

   if (t && t->GetMinimized())
   {
      // Jump out of minimized mode when resizing.
      // Initial height will be height as a minimized track.
      t->SetHeight( t->GetHeight());
      t->SetMinimized(false);
   }

   // If the click is at the bottom of a non-linked track label, we
   // treat it as a normal resize.  If the label is of a linked track,
   // we ignore the click.

   if (label && !label->GetLinked())
   {
      t = label;
   }

   if (!t) 
      return;

   Track *prev = mTracks->GetPrev(t);
   Track *next = mTracks->GetNext(t);

   // DM: Capture the track so that we continue to resize
   //  THIS track, no matter where the user moves the mouse
   SetCapturedTrack( t, IsResizing );
   //mCapturedRect = r;
   //mCapturedNum = num;

   mMouseClickX = event.m_x;
   mMouseClickY = event.m_y;

   //STM:  Determine whether we should rescale one or two tracks

   if (mTracks->GetLink(prev) == t) {
      if(prev->GetMinimized())
      {
         prev->SetHeight( prev->GetHeight());
         prev->SetMinimized(false);
      }
      // mCapturedTrack is the lower track
      mInitialTrackHeight = t->GetHeight();
      mInitialUpperTrackHeight = prev->GetHeight();
      SetCapturedTrack( t, IsResizingBelowLinkedTracks );
   } else if (next && mTracks->GetLink(t) == next) {
      // mCapturedTrack is the upper track
      mInitialTrackHeight = next->GetHeight();
      mInitialUpperTrackHeight = t->GetHeight();
      SetCapturedTrack( t, IsResizingBetweenLinkedTracks );
   } else {
      // DM: Save the initial mouse location and the initial height
      mInitialTrackHeight = t->GetHeight();
   }
}

///  This happens when the button is released from a drag.
///  Since we actually took care of resizing the track when
///  we got drag events, all we have to do here is clean up.
///  We also modify the undo state (the action doesn't become
///  undo-able, but it gets merged with the previous undo-able
///  event).
void TrackPanel::HandleResizeButtonUp(wxMouseEvent & event)
{
   SetCapturedTrack( NULL );
   MakeParentRedrawScrollbars();
   MakeParentModifyState();
}

///  Resize dragging means that the mouse button IS down and has moved
///  from its initial location.  By the time we get here, we
///  have already received a ButtonDown() event and saved the
///  track being resized in mCapturedTrack.
void TrackPanel::HandleResizeDrag(wxMouseEvent & event)
{
   int delta = (event.m_y - mMouseClickY);

   //STM: We may be dragging one or two (stereo) tracks.  
   // If two, resize proportionally if we are dragging the lower track, and
   // adjust compensatively if we are dragging the upper track.
   switch( mMouseCapture )
   {
   case IsResizingBelowLinkedTracks:
      {
         Track *prev = mTracks->GetPrev(mCapturedTrack);

         double proportion = static_cast < double >(mInitialTrackHeight)
             / (mInitialTrackHeight + mInitialUpperTrackHeight);

         int newTrackHeight = static_cast < int >
             (mInitialTrackHeight + delta * proportion);

         int newUpperTrackHeight = static_cast < int >
             (mInitialUpperTrackHeight + delta * (1.0 - proportion));

         //make sure neither track is smaller than its minimum height
         if (newTrackHeight < mCapturedTrack->GetMinimizedHeight())
            newTrackHeight = mCapturedTrack->GetMinimizedHeight();
         if (newUpperTrackHeight < prev->GetMinimizedHeight())
            newUpperTrackHeight = prev->GetMinimizedHeight();

         mCapturedTrack->SetHeight(newTrackHeight);
         prev->SetHeight(newUpperTrackHeight);
         break;
      }
   case IsResizingBetweenLinkedTracks:
      {
         Track *next = mTracks->GetNext(mCapturedTrack);
         int newUpperTrackHeight = mInitialUpperTrackHeight + delta;
         int newTrackHeight = mInitialTrackHeight - delta;

         // make sure neither track is smaller than its minimum height
         if (newTrackHeight < next->GetMinimizedHeight()) {
            newTrackHeight = next->GetMinimizedHeight();
            newUpperTrackHeight =
                mInitialUpperTrackHeight + mInitialTrackHeight - next->GetMinimizedHeight();
         }
         if (newUpperTrackHeight < mCapturedTrack->GetMinimizedHeight()) {
            newUpperTrackHeight = mCapturedTrack->GetMinimizedHeight();
            newTrackHeight =
                mInitialUpperTrackHeight + mInitialTrackHeight - mCapturedTrack->GetMinimizedHeight();
         }

         mCapturedTrack->SetHeight(newUpperTrackHeight);
         next->SetHeight(newTrackHeight);
         break;
      }
   case IsResizing:
      {
         int newTrackHeight = mInitialTrackHeight + delta;
         if (newTrackHeight < mCapturedTrack->GetMinimizedHeight())
            newTrackHeight = mCapturedTrack->GetMinimizedHeight();
         mCapturedTrack->SetHeight(newTrackHeight);
         break;
      }
   default:
      // don't refresh in this case.
      return;
   }
   
   Refresh(false);
}

/// HandleResize gets called when:
///  - A mouse-down event occurs in the "resize region" of a track,
///    i.e. to change its vertical height.
///  - A mouse event occurs and mIsResizing==true (i.e. while
///    the resize is going on)
void TrackPanel::HandleResize(wxMouseEvent & event)
{
   if (event.LeftDown()) {
      HandleResizeClick( event );
   } 
   else if (event.LeftUp()) 
   {
      HandleResizeButtonUp( event );
   }
   else if (event.Dragging()) {
      HandleResizeDrag( event );
   }
}

/// Handle mouse wheel rotation (for zoom in/out and vertical scrolling)
void TrackPanel::HandleWheelRotation(wxMouseEvent & event)
{
   int steps = event.m_wheelRotation /
      (event.m_wheelDelta > 0 ? event.m_wheelDelta : 120);

   if (event.ShiftDown())
   {
      // MM: Scroll left/right when used with Shift key down
      mListener->TP_ScrollWindow(
         mViewInfo->h +
         50.0 * -steps / mViewInfo->zoom);
   } else if (event.CmdDown())
   {
      // MM: Zoom in/out when used with Control key down
      // MM: I don't understand what trackLeftEdge does
      int trackLeftEdge = GetLeftOffset();
      
      double center_h = PositionToTime(event.m_x, trackLeftEdge);
      if (steps < 0)
         mViewInfo->zoom = wxMax(mViewInfo->zoom / (2.0 * -steps), gMinZoom);
      else
         mViewInfo->zoom = wxMin(mViewInfo->zoom * (2.0 * steps), gMaxZoom);

      double new_center_h = PositionToTime(event.m_x, trackLeftEdge);
      mViewInfo->h += (center_h - new_center_h);
      
      MakeParentRedrawScrollbars();
      Refresh(false);
   } else
   {
      // MM: Zoom up/down when used without modifier keys
      mListener->TP_ScrollUpDown(-steps * 4);
   }
}

/// Filter captured keys typed into LabelTracks.
void TrackPanel::OnCaptureKey(wxCommandEvent & event)
{
   // Only deal with LabelTracks
   Track *t = GetFocusedTrack();
   if (!t || t->GetKind() != Track::Label) {
      event.Skip();
      return;
   }
   wxKeyEvent *kevent = (wxKeyEvent *)event.GetEventObject();

   event.Skip(!((LabelTrack *)t)->CaptureKey(*kevent));
}

/// Allow typing into LabelTracks.
void TrackPanel::OnKeyDown(wxKeyEvent & event)
{
   // Only deal with LabelTracks
   Track *t = GetFocusedTrack();
   if (!t || t->GetKind() != Track::Label) {
      event.Skip();
      return;
   }

   LabelTrack *lt = (LabelTrack *)t;
   double bkpSel0 = mViewInfo->sel0, bkpSel1 = mViewInfo->sel1;

   // Pass keystroke to labeltrack's handler and add to history if any
   // updates were done
   if (lt->OnKeyDown(mViewInfo->sel0, mViewInfo->sel1, event))
      MakeParentPushState(_("Modified Label"),
                          _("Label Edit"),
                          true /* consolidate */);

   // Make sure caret is in view
   int x;
   if (lt->CalcCursorX(this, &x)) {
      ScrollIntoView(x);
   }

   // If selection modified, refresh
   // Otherwise, refresh track display if the keystroke was handled
   if( bkpSel0 != mViewInfo->sel0 || bkpSel1 != mViewInfo->sel1 )
      Refresh( false );
   else if (!event.GetSkipped()) 
      RefreshTrack(t, true);
}

/// Allow typing into LabelTracks.
void TrackPanel::OnChar(wxKeyEvent & event)
{
   // Only deal with LabelTracks
   Track *t = GetFocusedTrack();
   if (!t || t->GetKind() != Track::Label) {
      event.Skip();
      return;
   }

   double bkpSel0 = mViewInfo->sel0, bkpSel1 = mViewInfo->sel1;
   // Pass keystroke to labeltrack's handler and add to history if any
   // updates were done
   if (((LabelTrack *)t)->OnChar(mViewInfo->sel0, mViewInfo->sel1, event))
      MakeParentPushState(_("Modified Label"),
                          _("Label Edit"),
                          true /* consolidate */);

   // If selection modified, refresh
   // Otherwise, refresh track display if the keystroke was handled
   if( bkpSel0 != mViewInfo->sel0 || bkpSel1 != mViewInfo->sel1 )
      Refresh( false );
   else if (!event.GetSkipped()) 
      RefreshTrack(t, true);
}

/// This handles just generic mouse events.  Then, based
/// on our current state, we forward the mouse events to
/// various interested parties.
void TrackPanel::OnMouseEvent(wxMouseEvent & event)
{
   if (event.m_wheelRotation != 0)
      HandleWheelRotation(event);
   
   if (!mAutoScrolling) {
      mMouseMostRecentX = event.m_x;
      mMouseMostRecentY = event.m_y;
   }

   if (event.LeftDown()) {
      mCapturedTrack = NULL;
      
      // The activate event is used to make the 
      // parent window 'come alive' if it didn't have focus.
      wxActivateEvent e;
      GetParent()->ProcessEvent(e);

      // wxTimers seem to be a little unreliable, so this
      // "primes" it to make sure it keeps going for a while...

      // When this timer fires, we call TrackPanel::OnTimer and
      // possibly update the screen for offscreen scrolling.
      mTimer.Stop();
      mTimer.Start(50, FALSE);
   }

   if (event.ButtonDown()) {
      SetFocus();
      CaptureMouse();
   }
   else if (event.ButtonUp()) {
      if (HasCapture())
         ReleaseMouse();
   }

   switch( mMouseCapture )
   {
   case IsVZooming: 
      HandleVZoom(event);
      break;
   case IsClosing:
      HandleClosing(event);
      break;
   case IsPopping:
      HandlePopping(event);
      break;
   case IsMuting:
      HandleMutingSoloing(event, false);
      break;
   case IsSoloing:
      HandleMutingSoloing(event, true);
      break;
   case IsResizing: 
   case IsResizingBetweenLinkedTracks:
   case IsResizingBelowLinkedTracks:
      HandleResize(event);
      HandleCursor(event);
      break;
   case IsRearranging:
      HandleRearrange(event);
      break;
   case IsGainSliding:
      HandleSliders(event, false);
      break;
   case IsPanSliding:
      HandleSliders(event, true);
      break;
   case IsMinimizing:
      HandleMinimizing(event);
      break;
   case IsZooming:
      HandleZoom(event);
      break;
   case IsAdjustingLabel:
      HandleLabelTrackMouseEvent((LabelTrack *)mCapturedTrack, mCapturedRect, event);
      break;
   default: //includes case of IsUncaptured
      HandleTrackSpecificMouseEvent(event);
      break;
   }

   //EnsureVisible should be called after the up-click.
   if (event.ButtonUp()) {
      wxRect r;
      int num;

      Track *t = FindTrack(event.m_x, event.m_y, false, false, &r, &num);
      if (t)
         EnsureVisible(t);
   }


}

bool TrackPanel::HandleTrackLocationMouseEvent(WaveTrack * track, wxRect &r, wxMouseEvent &event)
{
   // FIX-ME: Disable this and return true when CutLines aren't showing?
   // (Don't use gPerfs-> for the fix as registry access is slow).

   if (mMouseCapture == WasOverCutLine)
   {
      // Needed to avoid select events right after IsOverCutLine event on button up
      if (event.ButtonUp()) {
         mMouseCapture = IsUncaptured;
         return false;
      }
      return true;
   }
   
   if (mMouseCapture == IsOverCutLine)
   {
      if (!mCapturedTrackLocationRect.Inside(event.m_x, event.m_y))
      {
         SetCapturedTrack( NULL );
         SetCursorByActivity();
         return false;
      }
      
      bool handled = false;

      if (event.LeftDown())
      {
         if (mCapturedTrackLocation.typ == WaveTrack::locationCutLine)
         {
            // When user presses left button on cut line, expand the line again
            double cutlineStart = 0, cutlineEnd = 0;
      
            // Release capture so user will be able to click OK on Linux
            if (HasCapture())
               ReleaseMouse();

            if (track->ExpandCutLine(mCapturedTrackLocation.pos, &cutlineStart, &cutlineEnd))
            {
               WaveTrack* linked = (WaveTrack*)mTracks->GetLink(track);
               if (linked)
                  linked->ExpandCutLine(mCapturedTrackLocation.pos);
               mViewInfo->sel0 = cutlineStart;
               mViewInfo->sel1 = cutlineEnd;
               DisplaySelection();
               MakeParentPushState(_("Expanded Cut Line"), _("Expand"), false );
               handled = true;
            }
         } else if (mCapturedTrackLocation.typ == WaveTrack::locationMergePoint)
         {
            track->MergeClips(mCapturedTrackLocation.clipidx1, mCapturedTrackLocation.clipidx2);
            WaveTrack* linked = (WaveTrack*)mTracks->GetLink(track);
            if (linked)
               linked->MergeClips(mCapturedTrackLocation.clipidx1, mCapturedTrackLocation.clipidx2);
            MakeParentPushState(_("Merged Clips"),_("Merge"), true );
            handled = true;
         }
         Refresh(false);
      }

      if (!handled && event.RightDown())
      {
         track->RemoveCutLine(mCapturedTrackLocation.pos);
         WaveTrack* linked = (WaveTrack*)mTracks->GetLink(track);
         if (linked)
            linked->RemoveCutLine(mCapturedTrackLocation.pos);
         MakeParentPushState(_("Removed Cut Line"), _NoAcc("&Remove"), false );
         handled = true;
      }
      
      if (handled)
      {
         SetCapturedTrack( NULL );
         mMouseCapture = WasOverCutLine; 
         Refresh(false);
         return true;
      }

      return true;
   }

   for (int i=0; i<track->GetNumCachedLocations(); i++)
   {
      WaveTrack::Location loc = track->GetCachedLocation(i);
      
      double x = (loc.pos - mViewInfo->h) * mViewInfo->zoom;
      if (x >= 0 && x < r.width)
      {
         wxRect locRect;
         locRect.x = int( r.x + x ) - 5;
         locRect.width = 11;
         locRect.y = r.y;
         locRect.height = r.height;
         if (locRect.Inside(event.m_x, event.m_y))
         {
            SetCapturedTrack(track, IsOverCutLine);
            mCapturedTrackLocation = loc;
            mCapturedTrackLocationRect = locRect;
            SetCursorByActivity();
            return true;
         }
      }
   }

   return false;
}


/// Event has happened on a track and it has been determined to be a label track.
bool TrackPanel::HandleLabelTrackMouseEvent(LabelTrack * lTrack, wxRect &r, wxMouseEvent & event)
{
   /// \todo This function is one of a large number of functions in 
   /// TrackPanel which suitably modified belong in other classes.
   if(event.LeftDown())      
   {
      TrackListIterator iter(mTracks);
      Track *n = iter.First();
 
      while (n) {
         if (n->GetKind() == Track::Label && lTrack != n) {
            ((LabelTrack *)n)->ResetFlags();
            ((LabelTrack *)n)->Unselect();
         }
         n = iter.Next();
      }

      //If the button was pressed, check to see if we are over
      //a glyph (this is the second of three calls to the function).
      //std::cout << ((LabelTrack*)pTrack)->OverGlyph(event.m_x, event.m_y) << std::endl;
      if(lTrack->OverGlyph(event.m_x, event.m_y))
      {
         SetCapturedTrack(lTrack, IsAdjustingLabel);
         mCapturedRect = r; 
         mCapturedRect.x += kLeftInset;
         mCapturedRect.width -= kLeftInset;
      }
   } else if (event.Dragging()) {
      ;
   } else if( event.LeftUp()) {
      SetCapturedTrack( NULL );
   }
   
   if (lTrack->HandleMouse(event, mCapturedRect,
      mViewInfo->h, mViewInfo->zoom, &mViewInfo->sel0, &mViewInfo->sel1)) {

      MakeParentPushState(_("Modified Label"),
                          _("Label Edit"),
                          true /* consolidate */);
   }

   
   if (event.RightDown()) {
      // popup menu for editing
      Refresh(false);
     
      if ((lTrack->getSelectedIndex() != -1) && lTrack->OverTextBox(lTrack->GetLabel(lTrack->getSelectedIndex()), event.m_x, event.m_y)) {
         mPopupMenuTarget = lTrack;
         mLabelTrackInfoMenu->Enable(OnCutSelectedTextID, lTrack->IsTextSelected());
         mLabelTrackInfoMenu->Enable(OnCopySelectedTextID, lTrack->IsTextSelected());
         mLabelTrackInfoMenu->Enable(OnPasteSelectedTextID, lTrack->IsTextClipSupported());
         PopupMenu(mLabelTrackInfoMenu, event.m_x + 1, event.m_y + 1);
         // it's an invalid dragging event
         lTrack->SetWrongDragging(true);
      }
      return true;
   }
   
   //If we are adjusting a label on a labeltrack, do not do anything 
   //that follows. Instead, redraw the track.
   if(mMouseCapture == IsAdjustingLabel)
   {
      Refresh(false);
      return true;
   }

   // handle dragging
   if(event.Dragging()) {
      // locate the initial mouse position
      if (event.LeftIsDown()) {
         if (mLabelTrackStartXPos == -1) {
            mLabelTrackStartXPos = event.m_x;
            mLabelTrackStartYPos = event.m_y;

            if ((lTrack->getSelectedIndex() != -1) && 
               lTrack->OverTextBox(
                  lTrack->GetLabel(lTrack->getSelectedIndex()), 
                  mLabelTrackStartXPos,
                  mLabelTrackStartYPos)) 
            {
               mLabelTrackStartYPos = -1;
            }
         }
         // if initial mouse position in the text box
         // then only drag text
         if (mLabelTrackStartYPos == -1) {
            Refresh(false);
            return true;
         }
      }
   }
  
   // handle mouse left button up
   if (event.LeftUp()) {
      mLabelTrackStartXPos = -1;
   }

   // handle shift+ctrl down
   /*if (event.ShiftDown()) { // && event.ControlDown()) {
      lTrack->SetHighlightedByKey(true);
      Refresh(false);
      return;
   }*/

   // handle shift+mouse left button
   if (event.ShiftDown() && event.ButtonDown() && (lTrack->getSelectedIndex() != -1)) {
      // if the mouse is clicked in text box, set flags
      if (lTrack->OverTextBox(lTrack->GetLabel(lTrack->getSelectedIndex()), event.m_x, event.m_y)) {
         lTrack->SetInBox(true);
         lTrack->SetDragXPos(event.m_x);
         lTrack->SetResetCursorPos(true);
         Refresh(false);
         return true;
      }
   }
   // return false, there is more to do...
   return false;
}

// AS: I don't really understand why this code is sectioned off
//  from the other OnMouseEvent code.
void TrackPanel::HandleTrackSpecificMouseEvent(wxMouseEvent & event)
{
   Track * pTrack;
   wxRect r;
   wxRect rLabel;
   int num;

   AudacityProject *p = GetProject();
   bool unsafe = (p->GetAudioIOToken()>0 &&
                  gAudioIO->IsStreamActive(p->GetAudioIOToken()));

   FindTrack(event.m_x, event.m_y, true, true, &rLabel, &num);
   pTrack = FindTrack(event.m_x, event.m_y, false, false, &r, &num);

   //call HandleResize if I'm over the border area 
   if (event.LeftDown() &&
       (within(event.m_y, r.y + r.height, TRACK_RESIZE_REGION)
        || within(event.m_y, rLabel.y + rLabel.height,
                  TRACK_RESIZE_REGION))) {
      HandleResize(event);
      HandleCursor(event);
      return;
   }

   //Determine if user clicked on the track's left-hand label
   if (!mCapturedTrack && event.m_x < GetLeftOffset()) {
      if (event.m_x >= GetVRulerOffset()) {
         if( !event.Dragging() ) // JKC: Only want the mouse down event.
            HandleVZoom(event);
         HandleCursor(event);
      }
      else {
         HandleLabelClick(event);
         HandleCursor(event);
      }
      return;
   }

   //Determine if user clicked on a label track.
   //If so, use MouseDown handler for the label track.
   if (pTrack && (pTrack->GetKind() == Track::Label)) 
   {
      if(HandleLabelTrackMouseEvent( (LabelTrack *) pTrack, r, event ))
         return;
   }

   if (pTrack && (pTrack->GetKind() == Track::Wave) && 
       (mMouseCapture == IsUncaptured || mMouseCapture == IsOverCutLine ||
        mMouseCapture == WasOverCutLine))
   {
      if (HandleTrackLocationMouseEvent( (WaveTrack *) pTrack, r, event ))
         return;
   }

   ToolsToolBar * pTtb = mListener->TP_GetToolsToolBar();
   if( pTtb == NULL )
      return;

   int toolToUse = DetermineToolToUse(pTtb, event);

   switch (toolToUse) {
   case selectTool:
      HandleSelect(event);
      break;
   case envelopeTool:
      if (!unsafe)
         HandleEnvelope(event);
      break;
   case slideTool:
      if (!unsafe)
         HandleSlide(event);
      break;
   case zoomTool:
      HandleZoom(event);
      break;
   case drawTool:
      if (!unsafe)
         HandleSampleEditing(event);
      break;
   }

   if ((event.Moving() || event.LeftUp())  &&
       (mMouseCapture == IsUncaptured ))
//       (mMouseCapture != IsSelecting ) && 
//       (mMouseCapture != IsEnveloping) &&
//       (mMouseCapture != IsSliding) )
   {
      HandleCursor(event);
   }
   if (event.LeftUp()) {
      mCapturedTrack = NULL;
   }
}

/// If we are in multimode, looks at the type of track and where we are on it to 
/// determine what object we are hovering over and hence what tool to use.
/// @param pTtb - A pointer to the tools tool bar
/// @param event - Mouse event, with info about position and what mouse buttons are down.
int TrackPanel::DetermineToolToUse( ToolsToolBar * pTtb, wxMouseEvent & event)
{
   int currentTool = pTtb->GetCurrentTool();

   // Unless in Multimode keep using the current tool.
   if( !pTtb->IsDown(multiTool) )
      return currentTool;

   // We NEVER change tools whilst we are dragging.
   if( event.Dragging() || event.LeftUp() )
      return currentTool;

   // Just like dragging.
   // But, this event might be the final button up
   // so keep the same tool.
//   if( mIsSliding || mIsSelecting || mIsEnveloping )
   if( mMouseCapture != IsUncaptured )
      return currentTool;

   // So now we have to find out what we are near to..
   wxRect r;
   int num;

   Track *pTrack = FindTrack(event.m_x, event.m_y, false, false, &r, &num);
   if( !pTrack )
      return currentTool;

   int trackKind = pTrack->GetKind();
   currentTool = selectTool; // the default.

   if( event.ButtonIsDown(3) || event.RightUp()){
      currentTool = zoomTool;
   } else if( trackKind == Track::Time ){
      currentTool = envelopeTool;
   } else if( trackKind == Track::Label ){
      currentTool = selectTool;
   } else if( trackKind != Track::Wave) {
      currentTool = selectTool;
   // So we are in a wave track.
   // From here on the order in which we hit test determines 
   // which tool takes priority in the rare cases where it
   // could be more than one.
   } else if (event.CmdDown()){
      // msmeyer: If control is down, slide single clip
      // msmeyer: If control and shift are down, slide all clips
      currentTool = slideTool;
   } else if( HitTestEnvelope( pTrack, r, event ) ){
      currentTool = envelopeTool;
   } else if( HitTestSlide( pTrack, r, event )){
      currentTool = slideTool;
   } else if( HitTestSamples( pTrack, r, event )){
      currentTool = drawTool;
   }

   //Use the false argument since in multimode we don't 
   //want the button indicating which tool is in use to be updated.
   pTtb->SetCurrentTool( currentTool, false );
   return currentTool;
}

/// Function that tells us if the mouse event landed on a 
/// envelope boundary.
bool TrackPanel::HitTestEnvelope(Track *track, wxRect &r, wxMouseEvent & event)
{
   WaveTrack *wavetrack = (WaveTrack *)track;
   Envelope *envelope = wavetrack->GetEnvelopeAtX(event.GetX());

   if (!envelope)
      return false;   

   int displayType = wavetrack->GetDisplay();
   // Not an envelope hit, unless we're using a type of wavetrack display 
   // suitable for envelopes operations, ie one of the Wave displays.
   if ( displayType > 1) 
      return false;  // No envelope, not a hit, so return.

   // Get envelope point, range 0.0 to 1.0
   bool dB = (displayType == 1);
   double envValue = envelope->GetValueAtX( 
      event.m_x, r, mViewInfo->h, mViewInfo->zoom );

   float zoomMin, zoomMax;
   wavetrack->GetDisplayBounds(&zoomMin, &zoomMax);

   // Get y position of envelope point.
   int yValue = GetWaveYPosNew( envValue,
      zoomMin, zoomMax,      
      r.height, dB, true, mdBr, false ) + r.y;

   // Get y position of center line
   int ctr = GetWaveYPosNew( 0.0,
      zoomMin, zoomMax,      
      r.height, dB, true, mdBr, false ) + r.y;
  
   // Get y distance of mouse from center line (in pixels).
   int yMouse = abs(ctr - event.m_y);
   // Get y distance of envelope from center line (in pixels)
   yValue = abs(ctr-yValue);
   
   // JKC: It happens that the envelope is actually drawn offset from its 
   // 'true' position (it is 3 pixels wide).  yMisalign is really a fudge
   // factor to allow us to hit it exactly, but I wouldn't dream of 
   // calling it yFudgeFactor :)
   const int yMisalign = 2; 
   // Perhaps yTolerance should be put into preferences?
   const int yTolerance = 5; // how far from envelope we may be and count as a hit.
   int distance;

   // For amplification using the envelope we introduced the idea of contours.
   // The contours have the same shape as the envelope, which may be partially off-screen.
   // The contours are closer in to the center line.
   int ContourSpacing = (int) (r.height / (2* (zoomMax-zoomMin)));
   const int MaxContours = 2;

   // Adding ContourSpacing/2 selects a region either side of the contour.
   int yDisplace = yValue - yMisalign - yMouse  + ContourSpacing/2;
   if (yDisplace > (MaxContours * ContourSpacing))
      return false;
   // Subtracting the ContourSpacing/2 we added earlier ensures distance is centred on the contour.
   distance = abs( ( yDisplace % ContourSpacing ) - ContourSpacing/2);
   return( distance < yTolerance );
}

/// Function that tells us if the mouse event landed on an 
/// editable sample
bool TrackPanel::HitTestSamples(Track *track, wxRect &r, wxMouseEvent & event)
{
   WaveTrack *wavetrack = (WaveTrack *)track;
   //Get rate in order to calculate the critical zoom threshold
   double rate = wavetrack->GetRate();

   //Find out the zoom level
   bool showPoints = (mViewInfo->zoom / rate > 3.0);
   if( !showPoints )
      return false;

   int displayType = wavetrack->GetDisplay();
   if ( displayType > 1) 
      return false;  // Not a wave, so return.

   float oneSample;
   double pps = mViewInfo->zoom;
   double tt = (event.m_x - r.x) / pps + mViewInfo->h;
   int    s0 = (int)(tt * rate + 0.5);

   // Just get one sample.
   wavetrack->Get((samplePtr)&oneSample, floatSample, s0, 1);
   
   // Get y distance of envelope point from center line (in pixels).
   bool dB = (displayType == 1);
   float zoomMin, zoomMax;

   wavetrack->GetDisplayBounds(&zoomMin, &zoomMax);
   
   double envValue = 1.0;
   Envelope* env = wavetrack->GetEnvelopeAtX(event.GetX());
   if (env)
      envValue = env->GetValue(tt);

   int yValue = GetWaveYPosNew( oneSample * envValue, 
      zoomMin, zoomMax,      
      r.height, dB, true, mdBr, false) + r.y;   

   // Get y position of mouse (in pixels)
   int yMouse = event.m_y;

   // Perhaps yTolerance should be put into preferences?
   const int yTolerance = 10; // More tolerance on samples than on envelope.
   return( abs( yValue -  yMouse ) < yTolerance );   
}

/// Function that tells us if the mouse event landed on a 
/// time-slider that allows us to time shift the sequence.
bool TrackPanel::HitTestSlide(Track *track, wxRect &r, wxMouseEvent & event)
{
   // Perhaps we should delegate this to TrackArtist as only TrackArtist
   // knows what the real sizes are??

   // The drag Handle width includes border, width and a little extra margin.
   const int adjustedDragHandleWidth = 14;
   // The hotspot for the cursor isn't at its centre.  Adjust for this. 
   const int hotspotOffset = 5;

   // We are doing an approximate test here - is the mouse in the right or left border?
   if( event.m_x + hotspotOffset < r.x + adjustedDragHandleWidth)
      return true;

   if( event.m_x + hotspotOffset > r.x + r.width - adjustedDragHandleWidth)
      return true;

   return false;
}

double TrackPanel::GetMostRecentXPos()
{
   return mViewInfo->h +
      (mMouseMostRecentX - GetLabelWidth()) / mViewInfo->zoom;
}

void TrackPanel::RefreshTrack(Track *trk, bool refreshbacking)
{
   int y = -mViewInfo->vpos;
   TrackListIterator iter(mTracks);
   Track *t;
   for( t = iter.First(); t; t = iter.Next() )
   {
      int h = t->GetHeight();

      if( t == trk )
      {
         int x = GetLeftOffset();
         int w = GetRect().GetWidth() - x;
         wxRect trackarea( x, y + kTopInset + 1, w, h - 2 );

         if( refreshbacking )
         {
            mRefreshBacking = true;
         }
         Refresh( false, &trackarea );
         break;
      }

      y += h;
   }
}


/// This function overrides Refresh() of wxWindow so that the 
/// boolean play indicator can be set to false, so that an old play indicator that is
/// no longer there won't get  XORed (to erase it), thus redrawing it on the 
/// TrackPanel
void TrackPanel::Refresh(bool eraseBackground /* = TRUE */,
                         const wxRect *rect /* = NULL */)
{
   // Tell OnPaint() to refresh the backing bitmap.
   //
   // Originally I had the check within the OnPaint() routine and it
   // was working fine.  That was until I found that, even though a full
   // refresh was requested, Windows only set the onscreen portion of a
   // window as damaged.
   //
   // So, if any part of the trackpanel was off the screen, full refreshes
   // didn't work and the display got corrupted.
   if( !rect || ( *rect == GetRect() ) )
   {
      mRefreshBacking = true;
   }
   wxWindow::Refresh(eraseBackground, rect);
   DisplaySelection();
}

/// Draw the actual track areas.  We only draw the borders
/// and the little buttons and menues and whatnot here, the
/// actual contents of each track are drawn by the TrackArtist.
void TrackPanel::DrawTracks(wxDC * dc)
{
   wxRect clip;
   GetSize(&clip.width, &clip.height);

   wxRect panelRect = clip;
   panelRect.y = -mViewInfo->vpos;

   wxRect tracksRect = panelRect;
   tracksRect.x += GetLabelWidth();
   tracksRect.width -= GetLabelWidth();

   ToolsToolBar *pTtb = mListener->TP_GetToolsToolBar();
   bool bMultiToolDown = pTtb->IsDown(multiTool);
   bool envelopeFlag   = pTtb->IsDown(envelopeTool) || bMultiToolDown;
   bool samplesFlag    = pTtb->IsDown(drawTool) || bMultiToolDown;
   bool sliderFlag     = bMultiToolDown;

   // The track artist actually draws the stuff inside each track
   mTrackArtist->DrawTracks(mTracks, *dc, GetUpdateRegion(),
                            tracksRect, clip, mViewInfo, 
                            envelopeFlag, samplesFlag, sliderFlag);

   DrawEverythingElse(dc, panelRect, clip);
}


/// Draws 'Everything else'.  In particular it draws:
///  - Drop shadow for tracks and vertical rulers.
///  - Zooming Indicators.
///  - Fills in space below the tracks. 
void TrackPanel::DrawEverythingElse(wxDC * dc, const wxRect panelRect,
                                    const wxRect clip)
{
   // We draw everything else
   TrackListIterator iter(mTracks);

   wxRect trackRect = panelRect;
   wxRect focusRect(-1, -1, 0, 0);
   wxRect r;

   int i = 0;
   for (Track * t = iter.First(); t; t = iter.Next()) {
      DrawEverythingElse(t, dc, r, trackRect, i);
      if( mAx->IsFocused( t ) ) {
         focusRect = mLastDrawnTrackRect;
      }
      i++;
   }

   if ((mMouseCapture == IsZooming || mMouseCapture == IsVZooming) &&
       IsDragZooming()) {
      DrawZooming(dc, clip);
   }

   // Paint over the part below the tracks
   GetSize(&trackRect.width, &trackRect.height);
   AColor::TrackPanelBackground(dc, false);
   dc->DrawRectangle(trackRect);

   if (GetFocusedTrack() != NULL && wxWindow::FindFocus() == this) {
      HighlightFocusedTrack(dc, focusRect);
   }

   // Draw snap guidelines if we have any
   if (mSnapManager && (mSnapLeft >= 0 || mSnapRight >= 0)) {
      AColor::SnapGuidePen(dc);
      if (mSnapLeft >= 0)
         dc->DrawLine((int)mSnapLeft, 0, mSnapLeft, 30000);
      if (mSnapRight >= 0)
         dc->DrawLine((int)mSnapRight, 0, mSnapRight, 30000);
   }
}

/// Draws 'Everything else' for one particular track.  Basically
/// everything except the actual waveform.
///
/// Note that this is being called in a loop and that the parameter values
/// are expected to be maintained each time through.
void TrackPanel::DrawEverythingElse(Track * t, wxDC * dc, wxRect & r,
                                    wxRect & trackRect, int index)
{
   trackRect.height = t->GetHeight();

   // Draw label area
   SetTrackInfoFont(dc);
   dc->SetTextForeground(theTheme.Colour( clrTrackPanelText ));

   int labelw = GetLabelWidth();
   int vrul = GetVRulerOffset();

   // If this track is linked to the next one, display a common
   // border for both, otherwise draw a normal border
   r = trackRect;

   bool skipBorder = false;
   if (t->GetLinked())
      r.height += mTracks->GetLink(t)->GetHeight();
   else if (mTracks->GetLink(t))
      skipBorder = true;

   if (!skipBorder)
      DrawOutside(t, dc, r, labelw, vrul, trackRect, index);

   // Believe it of not, we can speed up redrawing if we don't
   // redraw the vertical ruler when only the waveform data has
   // changed.

   int width, height;
   GetSize(&width, &height);
   wxRegion region = GetUpdateRegion();

#if DEBUG_DRAW_TIMING
   wxRect rbox = region.GetBox();
   wxPrintf(wxT("Update Region: %d %d %d %d\n"),
          rbox.x, rbox.y, rbox.width, rbox.height);
#endif

   wxRegionContain contain = region.Contains(0, 0, GetLeftOffset(), height);
   if (contain == wxPartRegion || contain == wxInRegion) {
      r = trackRect;
      r.x += GetVRulerOffset();
      r.y += kTopInset;
      r.width = GetVRulerWidth();
      r.height -= (kTopInset + 2);
      mTrackArtist->DrawVRuler(t, dc, r);
#ifdef EXPERIMENTAL_RULER_AUTOSIZE
      UpdateVRulerRect();
#endif //EXPERIMENTAL_RULER_AUTOSIZE
   }

   trackRect.y += t->GetHeight();
}

/// Draw a three-level highlight gradient around the focused track.
void TrackPanel::HighlightFocusedTrack(wxDC * dc, const wxRect r) {
   dc->SetBrush(*wxTRANSPARENT_BRUSH);
   AColor::TrackFocusPen(dc, 0);
   dc->DrawRectangle(r.x-1, r.y-1, r.width+2, r.height+2);
   AColor::TrackFocusPen(dc, 1);
   dc->DrawRectangle(r.x-2, r.y-2, r.width+4, r.height+4);
   AColor::TrackFocusPen(dc, 2);
   dc->DrawRectangle(r.x-3, r.y-3, r.width+6, r.height+6);
}

/// Draw zooming indicator that shows the region that will
/// be zoomed into when the user clicks and drags with a
/// zoom cursor.  Handles both vertical and horizontal
/// zooming.
void TrackPanel::DrawZooming(wxDC * dc, const wxRect clip)
{
   wxRect r;

   dc->SetBrush(*wxTRANSPARENT_BRUSH);
   dc->SetPen(*wxBLACK_DASHED_PEN);

   if (mMouseCapture==IsVZooming) {
      int width, height;
      GetTracksUsableArea(&width, &height);

      r.y = mZoomStart;
      r.x = GetVRulerOffset();
      r.width = width + GetVRulerWidth() + 1; //+1 extends into border rect
      r.height = mZoomEnd - mZoomStart;
   }
   else {
      r.x = mZoomStart;
      r.y = -1;
      r.width = mZoomEnd - mZoomStart;
      r.height = clip.height + 2;
   }
      
   dc->DrawRectangle(r);
}


void TrackPanel::DrawOutside(Track * t, wxDC * dc, const wxRect rec,
                             const int labelw, const int vrul,
                             const wxRect trackRect, int index)
{
   wxRect r = rec;

   DrawOutsideOfTrack(t, dc, r);
   r.x += kLeftInset;
   r.y += kTopInset;
   r.width -= kLeftInset * 2;
   r.height -= kTopInset;

   mLastDrawnTrackRect = r;

   dc->SetTextForeground(theTheme.Colour( clrTrackPanelText ));
   bool bIsWave = (t->GetKind() == Track::Wave);

#ifdef EXPERIMENTAL_RULER_AUTOSIZE
   mTrackInfo.DrawBackground(dc, r, t->GetSelected(), bIsWave, labelw, vrul);
#else //!EXPERIMENTAL_RULER_AUTOSIZE
   mTrackInfo.DrawBackground(dc, r, t->GetSelected(), bIsWave, labelw );
#endif //EXPERIMENTAL_RULER_AUTOSIZE

   DrawBordersAroundTrack(t, dc, r, labelw, vrul);
   DrawShadow(t, dc, r);

   r.width = mTrackInfo.GetTitleWidth();
   mTrackInfo.DrawCloseBox(dc, r, (mMouseCapture==IsClosing));
   mTrackInfo.DrawTitleBar(dc, r, t, (mMouseCapture==IsPopping));

   mTrackInfo.DrawMinimize(dc, r, t, (mMouseCapture==IsMinimizing), t->GetMinimized());
   mTrackInfo.DrawBordersWithin( dc, r, bIsWave );

   if (t->GetKind() == Track::Wave) {
      mTrackInfo.DrawMuteSolo(dc, r, t, (mMouseCapture == IsMuting), false, HasSoloButton());
      mTrackInfo.DrawMuteSolo(dc, r, t, (mMouseCapture == IsSoloing), true, HasSoloButton());

      mTrackInfo.DrawSliders(dc, (WaveTrack *)t, r, index);
   }

   r = trackRect;

   if (t->GetKind() == Track::Wave && !t->GetMinimized()) {
      int offset;

      offset = 8;
      
      if (r.y + 22 + 12 < rec.y + rec.height - 19)
         dc->DrawText(TrackSubText(t), r.x + offset, r.y + 22);
         
      if (r.y + 38 + 12 < rec.y + rec.height - 19)
         dc->DrawText(GetSampleFormatStr
                      (((WaveTrack *) t)->GetSampleFormat()), r.x + offset,
                      r.y + 38);
                      
   } else if (t->GetKind() == Track::Note) {
      wxRect midiRect;
      mTrackInfo.GetTrackControlsRect(trackRect, midiRect);
      ((NoteTrack *) t)->DrawLabelControls(*dc, midiRect);

   }
}

void TrackPanel::DrawOutsideOfTrack(Track * t, wxDC * dc, const wxRect r)
{
   // Fill in area outside of the track
   AColor::TrackPanelBackground(dc, false);
   wxRect side = r;
   side.width = kLeftInset;
   dc->DrawRectangle(side);
   side = r;
   side.height = kTopInset;
   dc->DrawRectangle(side);
   side = r;
   side.x += side.width - kTopInset;
   side.width = kTopInset;
   dc->DrawRectangle(side);

   if (t->GetLinked()) {
      side = r;
      side.y += t->GetHeight() - 1;
      side.height = kTopInset + 1;
      dc->DrawRectangle(side);
   }
}

#ifdef EXPERIMENTAL_RULER_AUTOSIZE
void TrackPanel::UpdateVRulerRect()
{
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   if (t) {
      wxSize s=t->vrulerSize;
      while (t) {
         if (s.x < t->vrulerSize.x)
            s.x = t->vrulerSize.x;
         if (s.y < t->vrulerSize.y)
            s.y = t->vrulerSize.y;
         t = iter.Next();
      }
      if (vrulerSize != s) {
         vrulerSize = s;
         Refresh();
         mRuler->SetLeftOffset(GetLeftOffset()-1);  // bevel on AdornedRuler
         mRuler->Refresh();
      }
   }
}
#endif //EXPERIMENTAL_RULER_AUTOSIZE

/// The following function moves to the previous track
/// selecting and unselecting depending if you are on the start of a
/// block or not.

/// \todo Merge related functions, TrackPanel::OnPrevTrack and 
/// TrackPanel::OnNextTrack.
void TrackPanel::OnPrevTrack( bool shift )
{
   Track *t;
   Track *p;
   TrackListIterator iter( mTracks );
   bool tSelected,pSelected;

   t = GetFocusedTrack();   // Get currently focused track
   if( t == NULL )   // if there isn't one, focus on last
   {
      t = iter.Last();
      SetFocusedTrack( t );
      EnsureVisible( t );
      return;
   }

   if( shift )
   {
      p = mTracks->GetPrev( t, true ); // Get previous track
      if( p == NULL )   // On first track
      {
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( mTracks );
            for( Track *d = iter.First(); d; d = iter.Next( true ) )
            {
               p = d;
            }
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      tSelected = t->GetSelected();
      pSelected = p->GetSelected();
      if( tSelected & pSelected )
      {
         mTracks->Select( t, false );
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         return;
      }
      if( tSelected & !pSelected )
      {
         mTracks->Select( p, true );
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         return;
      }
      if( !tSelected & pSelected )
      {
         mTracks->Select( p, false );
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         return;
      }
      if( !tSelected & !pSelected )
      {
         mTracks->Select( t, true );
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         return;
      }
   }
   else
   {
      p = mTracks->GetPrev( t, true ); // Get next track
      if( p == NULL )   // On last track so stay there?
      {
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( mTracks );
            for( Track *d = iter.First(); d; d = iter.Next( true ) )
            {
               p = d;
            }
            SetFocusedTrack( p );   // Wrap to the first track
            EnsureVisible( p );
            return;
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      else
      {
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         return;
      }
   }
}

/// The following function moves to the next track,
/// selecting and unselecting depending if you are on the start of a
/// block or not.
void TrackPanel::OnNextTrack( bool shift )
{
   Track *t;
   Track *n;
   TrackListIterator iter( mTracks );
   bool tSelected,nSelected;

   t = GetFocusedTrack();   // Get currently focused track
   if( t == NULL )   // if there isn't one, focus on first
   {
      t = iter.First();
      SetFocusedTrack( t );
      EnsureVisible( t );
      return;
   }

   if( shift )
   {
      n = mTracks->GetNext( t, true ); // Get next track
      if( n == NULL )   // On last track so stay there
      {
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( mTracks );
            n = iter.First();
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      tSelected = t->GetSelected();
      nSelected = n->GetSelected();
      if( tSelected & nSelected )
      {
         mTracks->Select( t, false );
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         return;
      }
      if( tSelected & !nSelected )
      {
         mTracks->Select( n, true );
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         return;
      }
      if( !tSelected & nSelected )
      {
         mTracks->Select( n, false );
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         return;
      }
      if( !tSelected & !nSelected )
      {
         mTracks->Select( t, true );
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         return;
      }
   }
   else
   {
      n = mTracks->GetNext( t, true ); // Get next track
      if( n == NULL )   // On last track so stay there
      {
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( mTracks );
            n = iter.First();
            SetFocusedTrack( n );   // Wrap to the first track
            EnsureVisible( n );
            return;
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      else
      {
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         return;
      }
   }
}

void TrackPanel::OnToggle()
{
   Track *t;

   t = GetFocusedTrack();   // Get currently focused track
   if (!t)
      return;

   mTracks->Select( t, !t->GetSelected() );
   EnsureVisible( t );
   return;
}

// Make sure selection edge is in view                                       
void TrackPanel::ScrollIntoView(double pos)
{
   int w, h;
   GetTracksUsableArea( &w, &h );

   if( ( pos < mViewInfo->h ) ||
       ( pos > mViewInfo->h + ( ( w - 1 ) / mViewInfo->zoom ) ) )
   {
      mListener->TP_ScrollWindow( pos - ( ( w / 2 ) / mViewInfo->zoom ) );
      Refresh(false);
   }
}

void TrackPanel::ScrollIntoView(int x)
{
   ScrollIntoView(PositionToTime(x, GetLeftOffset()));
}

void TrackPanel::OnCursorLeft( bool shift, bool ctrl )
{
   Track *t;

   // If the last adjustment was very recent, we are
   // holding the key down and should move faster.
   wxLongLong curtime = ::wxGetLocalTimeMillis();
   int multiplier = 1;
   if( curtime - mLastSelectionAdjustment < 50 )
   {
      multiplier = 4;
   }
   mLastSelectionAdjustment = curtime;

   // Get currently focused track if there is one
   t = GetFocusedTrack();

   // Contract selection from the right to the left
   if( shift && ctrl )
   {
      // Reduce and constrain (counter-intuitive)
      mViewInfo->sel1 -= multiplier / mViewInfo->zoom;
      if( mViewInfo->sel1 < mViewInfo->sel0 )
      {
         mViewInfo->sel1 = mViewInfo->sel0;
      }

      // Make sure it's visible
      ScrollIntoView( mViewInfo->sel1 );

      // Make it happen
      Refresh( false );
   }
   // Extend selection toward the left
   else if( shift )
   {
      // If playing, reposition a long amount of time
      int token = GetProject()->GetAudioIOToken();
      if( token > 0 && gAudioIO->IsStreamActive( token ) )
      {
         gAudioIO->SeekStream(-mSeekLong);
         return;
      }

      // Expand and constrain
      mViewInfo->sel0 -= multiplier / mViewInfo->zoom;
      if( mViewInfo->sel0 < 0.0 )
      {
         mViewInfo->sel0 = 0.0;
      }

      // Make sure it's visible
      ScrollIntoView( mViewInfo->sel0 );

      // Make it happen
      Refresh( false );
   }
   // Move the cursor toward the left
   else
   {
      // If playing, reposition a short amount of time
      int token = GetProject()->GetAudioIOToken();
      if( token > 0 && gAudioIO->IsStreamActive( token ) )
      {
         gAudioIO->SeekStream(-mSeekShort);
         return;
      }
      
      // Already in cursor mode?
      if( mViewInfo->sel0 == mViewInfo->sel1 )
      {
         // Move and constrain
         mViewInfo->sel0 -= multiplier / mViewInfo->zoom;
         if( mViewInfo->sel0 < 0.0 )
         {
            mViewInfo->sel0 = 0.0;
         }
         mViewInfo->sel1 = mViewInfo->sel0;

         // Move the visual cursor
         DrawCursor();
      }
      else
      {
         // Transition to cursor mode
         mViewInfo->sel1 = mViewInfo->sel0;

         // Make it happen
         Refresh( false );
      }

      // Make sure it's visible
      ScrollIntoView( mViewInfo->sel0 );
   }

   MakeParentModifyState();
}

void TrackPanel::OnCursorRight( bool shift, bool ctrl )
{
   Track *t;

   // If the last adjustment was very recent, we are
   // holding the key down and should move faster.
   wxLongLong curtime = ::wxGetLocalTimeMillis();
   int multiplier = 1;
   if( curtime - mLastSelectionAdjustment < 50 )
   {
      multiplier = 4;
   }
   mLastSelectionAdjustment = curtime;

   // Get currently focused track if there is one
   t = GetFocusedTrack();

   // Contract selection from the left to the right
   if( shift && ctrl )
   {
      // Reduce and constrain (counter-intuitive)
      mViewInfo->sel0 += multiplier / mViewInfo->zoom;
      if( mViewInfo->sel0 > mViewInfo->sel1 )
      {
         mViewInfo->sel0 = mViewInfo->sel1;
      }

      // Make sure new position is in view
      ScrollIntoView( mViewInfo->sel0 );

      // Make it happen
      Refresh( false );
   }
   // Extend selection toward the right
   else if( shift )
   {
      // If playing, reposition a long amount of time
      int token = GetProject()->GetAudioIOToken();
      if( token > 0 && gAudioIO->IsStreamActive( token ) )
      {
         gAudioIO->SeekStream(mSeekLong);
         return;
      }

      // Expand and constrain
      mViewInfo->sel1 += multiplier/mViewInfo->zoom;
      double end = mTracks->GetEndTime();
      if( mViewInfo->sel1 > end )
      {
         mViewInfo->sel1 = end;
      }

      // Make sure new position is in view
      ScrollIntoView( mViewInfo->sel1 );

      // Make it happen
      Refresh( false );
   }
   // Move the cursor toward the right
   else
   {
      // If playing, reposition a short amount of time
      int token = GetProject()->GetAudioIOToken();
      if( token > 0 && gAudioIO->IsStreamActive( token ) )
      {
         gAudioIO->SeekStream(mSeekShort);
         return;
      }

      // Already in cursor mode?
      if (mViewInfo->sel0 == mViewInfo->sel1)
      {
         // Move and constrain
         mViewInfo->sel1 += multiplier / mViewInfo->zoom;
         double end = mTracks->GetEndTime();
         if( mViewInfo->sel1 > end )
         {
            mViewInfo->sel1 = end;
         }
         mViewInfo->sel0 = mViewInfo->sel1;

         // Move the visual cursor
         DrawCursor();
      }
      else
      {
         // Transition to cursor mode
         mViewInfo->sel0 = mViewInfo->sel1;

         // Make it happen
         Refresh( false );
      }

      // Make sure new position is in view
      ScrollIntoView( mViewInfo->sel1 );
   }

   MakeParentModifyState();
}

void TrackPanel::OnBoundaryMove(bool left, bool boundaryContract)
{
  // Move the left/right selection boundary, to either expand or contract the selection
  // left=true: operate on left boundary; left=false: operate on right boundary
  // boundaryContract=true: contract region; boundaryContract=false: expand region.

   Track *t;

   // If the last adjustment was very recent, we are
   // holding the key down and should move faster.
   wxLongLong curtime = ::wxGetLocalTimeMillis();
   int multiplier = 1;
   if( curtime - mLastSelectionAdjustment < 50 )
   {
      multiplier = 4;
   }
   mLastSelectionAdjustment = curtime;

   int token = GetProject()->GetAudioIOToken();
   if( token > 0 && gAudioIO->IsStreamActive( token ) )
   {
      double indicator = gAudioIO->GetStreamTime();
      if (left) {
         mViewInfo->sel0 = indicator;
         if(mViewInfo->sel1 < mViewInfo->sel0)
            mViewInfo->sel1 = mViewInfo->sel0;
      } 
      else
      {
         mViewInfo->sel1 = indicator;
      };

      MakeParentModifyState();
      Refresh(false);
   }
   else
   { 
      // Get currently focused track if there is one
      t = GetFocusedTrack();

      // BOUNDARY MOVEMENT
      // Contract selection from the right to the left
      if( boundaryContract )
      {
         if (left) {
            // Reduce and constrain left boundary (counter-intuitive)
            mViewInfo->sel0 += multiplier / mViewInfo->zoom;
            if( mViewInfo->sel0 > mViewInfo->sel1 )
            {
               mViewInfo->sel0 = mViewInfo->sel1;
            }
            // Make sure it's visible
            ScrollIntoView( mViewInfo->sel0 );
         } 
         else 
         {
            // Reduce and constrain right boundary (counter-intuitive)
            mViewInfo->sel1 -= multiplier / mViewInfo->zoom;
            if( mViewInfo->sel1 < mViewInfo->sel0 )
            {
               mViewInfo->sel1 = mViewInfo->sel0;
            }
            // Make sure it's visible
            ScrollIntoView( mViewInfo->sel1 );
         }
      }
      // BOUNDARY MOVEMENT
      // Extend selection toward the left
      else 
      {
         if (left) {
            // Expand and constrain left boundary
            mViewInfo->sel0 -= multiplier / mViewInfo->zoom;
            if( mViewInfo->sel0 < 0.0 )
            {
               mViewInfo->sel0 = 0.0;
            }
            // Make sure it's visible
            ScrollIntoView( mViewInfo->sel0 );
         }
         else
         {
            // Expand and constrain right boundary
            mViewInfo->sel1 += multiplier/mViewInfo->zoom;
            double end = mTracks->GetEndTime();
            if( mViewInfo->sel1 > end )
            {
               mViewInfo->sel1 = end;
            }
         };
      }
      // Make it happen
      Refresh( false );
      MakeParentModifyState();
   }
}

void TrackPanel::OnCursorMove(bool forward, bool jump, bool longjump )
   // Move the cursor forward or backward, while paused or while playing. 
   // forward=true: Move cursor forward; forward=false: Move cursor backwards
   // jump=false: Move cursor determined by zoom; jump=true: Use seek times
   // longjump=false: Use mSeekShort; longjump=true: Use mSeekLong
   {
   // If the last adjustment was very recent, we are
   // holding the key down and should move faster.
   wxLongLong curtime = ::wxGetLocalTimeMillis();
   int multiplier = 1;
   if( curtime - mLastSelectionAdjustment < 50 )
   {
      multiplier = 4;
   }
   mLastSelectionAdjustment = curtime;

   float direction = -1;
   if (forward) {
     direction = 1;
   };

   float mSeek;
   if (jump) {
     if (!longjump) {
       mSeek = mSeekShort;
     } else {
       mSeek = mSeekLong;
     };
   } else {
     mSeek = multiplier / mViewInfo->zoom;
   };
   mSeek *= direction;

   // If playing, reposition a short amount of time
   int token = GetProject()->GetAudioIOToken();
   if( token > 0 && gAudioIO->IsStreamActive( token ) )
   {
      gAudioIO->SeekStream(mSeek);
   } 
   else 
   {      
      // Already in cursor mode?
      if( mViewInfo->sel0 == mViewInfo->sel1 )
      {
         // Move and constrain
	mViewInfo->sel0 += mSeek;
	if( !forward && mViewInfo->sel0 < 0.0 )
	{
            mViewInfo->sel0 = 0.0;
	}
	double end = mTracks->GetEndTime();
	if( forward && mViewInfo->sel0 > end)
	{
            mViewInfo->sel0 = end;
	}
        mViewInfo->sel1 = mViewInfo->sel0;

	// Move the visual cursor
	DrawCursor();
      }
      else
      {
         // Transition to cursor mode
         mViewInfo->sel1 = mViewInfo->sel0;

         // Make it happen
         Refresh( false );
      }

      // Make sure it's visible
      ScrollIntoView( mViewInfo->sel0 );
      MakeParentModifyState();
   };
}

//The following functions operate controls on specified tracks,
//This will pop up the track panning dialog for specified track
void TrackPanel::OnTrackPan()
{
   Track *t = GetFocusedTrack();
   if (!t || (t->GetKind() != Track::Wave)) return;

   //Find out which track we are on.
   int tracknum = FindTrackNum(t);
   if(tracknum)
      {
         LWSlider *slider = mTrackInfo.mPans[tracknum-1];
         if( slider->ShowDialog() )
         {
            SetTrackPan(t, slider);
         }
      }
}

void TrackPanel::OnTrackPanLeft()
{
   Track *t = GetFocusedTrack();
   if (!t || (t->GetKind() != Track::Wave)) return;

   //Find out which track we are on.
   int tracknum = FindTrackNum(t);
   if(tracknum)
      {
         LWSlider *slider = mTrackInfo.mPans[tracknum-1];
         slider->Decrease( 1 );
         SetTrackPan(t, slider);
      }
}

void TrackPanel::OnTrackPanRight()
{
   Track *t = GetFocusedTrack();
   if (!t || (t->GetKind() != Track::Wave)) return;

   //Find out which track we are on.
   int tracknum = FindTrackNum(t);
   if(tracknum)
      {
         LWSlider *slider = mTrackInfo.mPans[tracknum-1];
         slider->Increase( 1 );
         SetTrackPan(t, slider);
      }
}

void TrackPanel::SetTrackPan(Track * t, LWSlider * s)
{
   float newValue = s->Get();

   WaveTrack *link = (WaveTrack *)mTracks->GetLink(t);
   ((WaveTrack*)t)->SetPan(newValue);
   if (link)
      link->SetPan(newValue);

   MakeParentPushState(_("Adjusted Pan"), _("Pan"), true );

   Refresh(false);
}

/// This will pop up the track gain dialog for specified track
void TrackPanel::OnTrackGain()
{
   Track *t = GetFocusedTrack();
   if (!t || (t->GetKind() != Track::Wave)) return;

   //Find out which track we are on.
   int tracknum = FindTrackNum(t);
   if(tracknum)
      {
         LWSlider *slider = mTrackInfo.mGains[tracknum-1];
         if( slider->ShowDialog() )
         {
            SetTrackGain(t, slider);
         }
      }
}

void TrackPanel::OnTrackGainInc()
{
   Track *t = GetFocusedTrack();
   if (!t || (t->GetKind() != Track::Wave)) return;

   //Find out which track we are on.
   int tracknum = FindTrackNum(t);
   if(tracknum)
      {
         LWSlider *slider = mTrackInfo.mGains[tracknum-1];
         slider->Increase( 1 );
         SetTrackGain(t, slider);
      }
}

void TrackPanel::OnTrackGainDec()
{
   Track *t = GetFocusedTrack();
   if (!t || (t->GetKind() != Track::Wave)) return;

   //Find out which track we are on.
   int tracknum = FindTrackNum(t);
   if(tracknum)
      {
         LWSlider *slider = mTrackInfo.mGains[tracknum-1];
         slider->Decrease( 1 );
         SetTrackGain(t, slider);
      }
}

void TrackPanel::SetTrackGain(Track * t, LWSlider * s)
{
   float newValue = s->Get();

   WaveTrack *link = (WaveTrack *)mTracks->GetLink(t);
   ((WaveTrack*)t)->SetGain(newValue);
   if (link)
      link->SetGain(newValue);

   MakeParentPushState(_("Adjusted gain"), _("Gain"), true );

   Refresh(false);
}

void TrackPanel::OnTrackMenu(Track *t)
{
   if(!t) {
      t = GetFocusedTrack();
      if(!t) return;
   }

   mPopupMenuTarget = t;

   bool canMakeStereo = false;
   Track *next = mTracks->GetNext(t);
   
   wxMenu *theMenu = NULL;
   if (t->GetKind() == Track::Time)
      theMenu = mTimeTrackMenu;
   
   if (t->GetKind() == Track::Wave) {
      theMenu = mWaveTrackMenu;
      if (next && !t->GetLinked() && !next->GetLinked()
            && t->GetKind() == Track::Wave
            && next->GetKind() == Track::Wave)
         canMakeStereo = true;
      
      theMenu->Enable(OnMergeStereoID, canMakeStereo);
      theMenu->Enable(OnSplitStereoID, t->GetLinked());
      theMenu->Check(OnChannelMonoID, t->GetChannel() == Track::MonoChannel);
      theMenu->Check(OnChannelLeftID, t->GetChannel() == Track::LeftChannel);
      theMenu->Check(OnChannelRightID, t->GetChannel() == Track::RightChannel);
      theMenu->Enable(OnChannelMonoID, !t->GetLinked());
      theMenu->Enable(OnChannelLeftID, !t->GetLinked());
      theMenu->Enable(OnChannelRightID, !t->GetLinked());
      
      int display = ((WaveTrack *) t)->GetDisplay();
      
      theMenu->Enable(OnWaveformID, display != WaveTrack::WaveformDisplay);
      theMenu->Enable(OnWaveformDBID,
                        display != WaveTrack::WaveformDBDisplay);
      theMenu->Enable(OnSpectrumID, display != WaveTrack::SpectrumDisplay);
#ifdef LOGARITHMIC_SPECTRUM
      theMenu->Enable(OnSpectrumLogID, display != WaveTrack::SpectrumLogDisplay);
#endif
      theMenu->Enable(OnPitchID, display != WaveTrack::PitchDisplay);
      
      WaveTrack * track = (WaveTrack *)t;
      SetMenuCheck(*mRateMenu, IdOfRate((int) track->GetRate()));
      SetMenuCheck(*mFormatMenu, IdOfFormat(track->GetSampleFormat()));

      bool unsafe = IsUnsafe();
      for (int i = OnRate8ID; i <= OnFloatID; i++) {
         theMenu->Enable(i, !unsafe);
      }
   }
   
   if (t->GetKind() == Track::Note)
      theMenu = mNoteTrackMenu;
   
   if (t->GetKind() == Track::Label)
      theMenu = mLabelTrackMenu;
   
   if (theMenu) {
      theMenu->Enable(OnMoveUpID, mTracks->CanMoveUp(t));
      theMenu->Enable(OnMoveDownID, mTracks->CanMoveDown(t));
      
#ifdef __WXMAC__
      ::InsertMenu((MenuRef) mRateMenu->GetHMenu(), -1);
      ::InsertMenu((MenuRef) mFormatMenu->GetHMenu(), -1);
#endif

      //We need to find the location of the menu rectangle.
      wxRect r = FindTrackRect(t,true);
      wxRect titleRect;
      mTrackInfo.GetTitleBarRect(r,titleRect);

      PopupMenu(theMenu, titleRect.x + 1,
                  titleRect.y + titleRect.height + 1);
      
#ifdef __WXMAC__
      ::DeleteMenu(mFormatMenu->MacGetMenuId());
      ::DeleteMenu(mRateMenu->MacGetMenuId());
#endif
   }

   SetCapturedTrack(NULL);

   Refresh(false);
}

void TrackPanel::OnTrackMute(bool shiftDown, Track *t)
{
   if(!t) {
      t = GetFocusedTrack();
      if(!t) return;
   }

   if (shiftDown) 
      {
         // Cosmetic...  
         // Shift-click mutes this track and unmutes other tracks.
         TrackListIterator iter(mTracks);
         Track *i = iter.First();
         while (i) {
            if (i == t) {
               i->SetMute(true);
            }
            else {
               i->SetMute(false);
            }
            i->SetSolo(false);
            i = iter.Next();
         }
      }
   else {
      // Normal click toggles this track.
      t->SetMute(!t->GetMute());

      if( IsSimpleSolo() )
      {
         TrackListIterator iter(mTracks);
         Track *i = iter.First();
         int nPlaying=0;

         // We also set a solo indicator if we have just one track playing.
         // otherwise clear solo on everything.
         while (i) {
            if( !i->GetMute())
               nPlaying += 1;
            i = iter.Next();
         }

         i = iter.First();
         while (i) {
            i->SetSolo( (nPlaying==1) && !i->GetMute() );
            i = iter.Next();
         }
      }
   }

   Refresh(false);
}


void TrackPanel::OnTrackSolo(bool shiftDown, Track *t)
{
   if(!t) {
      t = GetFocusedTrack();
      if(!t) return;
   }

   bool bSoloMultiple = !IsSimpleSolo() ^ shiftDown;

   // Standard and Simple solo have opposite defaults:
   //   Standard - Behaves as individual buttons, shift=radio buttons
   //   Simple   - Behaves as radio buttons, shift=individual
   // In addition, Simple solo will mute/unmute tracks 
   // when in standard radio button mode.
   if ( bSoloMultiple ) 
   {
      t->SetSolo( !t->GetSolo() );
   }
   else 
   {
      // Normal click solo this track only, mute everything else.
      // OR unmute and unsolo everything.
      TrackListIterator iter(mTracks);
      Track *i = iter.First();
      bool bWasSolo = t->GetSolo();
      while (i) {
         i->SetSolo( (i==t) && !bWasSolo);
         if( IsSimpleSolo() )
            i->SetMute( (i!=t) && !bWasSolo);
         i = iter.Next();
      }
      //t->SetSolo(!t->GetSolo());
   }


   Refresh(false);

}

void TrackPanel::OnTrackClose()
{
   Track *t = GetFocusedTrack();
   if(!t) return;

   if( gAudioIO->IsStreamActive( GetProject()->GetAudioIOToken() ) )
   {
      mListener->TP_DisplayStatusMessage( _( "Can't delete track with active audio" ) );
      wxBell();
      return;
   }

   RemoveTrack( t );

   SetCapturedTrack( NULL );

   // BG: There are no more tracks on screen
   if( mTracks->IsEmpty() )
   {
      //BG: Set zoom to normal
      mViewInfo->zoom = 44100.0 / 512.0;

      //STM: Set selection to 0,0
      mViewInfo->sel0 = 0.0;
      mViewInfo->sel1 = 0.0;
      
      mListener->TP_RedrawScrollbars();
      mListener->TP_DisplayStatusMessage( wxT( "" ) ); //STM: Clear message if all tracks are removed
   }

   Refresh( false );
}


Track * TrackPanel::GetFirstSelectedTrack()
{

   TrackListIterator iter(mTracks);
 
   Track * t;
   for ( t = iter.First();t!=NULL;t=iter.Next())
      {
         //Find the first selected track
         if(t->GetSelected())
            {
               return t;
            }

      }
   //if nothing is selected, return the first track
   t = iter.First();

   if(t)
      return t;
   else
      return NULL;
}

void TrackPanel::EnsureVisible(Track * t)
{
   TrackListIterator iter(mTracks);
   Track *it = NULL;
   Track *nt = NULL;
   //int i = 0;

   SetFocusedTrack(t);

   int trackTop = 0;
   int trackHeight =0;

   for (it = iter.First(); it; it = iter.Next())
   {
      trackTop += trackHeight;
      trackHeight =  it->GetHeight();


      //find the second track if this is stereo
      if (it->GetLinked())
         {
            nt = iter.Next();
            trackHeight += nt->GetHeight();
         }
      else
         {
            nt = it;
         }


      //We have found the track we want to ensure is visible.
      if ((it == t) || (nt == t))
         {
            
            //Get the size of the trackpanel.
            int width, height;
            GetSize(&width, &height);

            // Debugging line: //std::cout << trackTop << " " << mViewInfo->vpos << " " << trackHeight << " " << height << std::endl;
            if( trackTop < mViewInfo->vpos || trackTop + trackHeight > mViewInfo->vpos + height)
               {   
                  mListener->TP_ScrollUpDown(-mViewInfo->vpos);
                  mListener->TP_ScrollUpDown(trackTop / mViewInfo->scrollStep);
                  break;
               }
         }
   }
   Refresh(false);
}

void TrackPanel::DrawBordersAroundTrack(Track * t, wxDC * dc,
                                        const wxRect r, const int vrul,
                                        const int labelw)
{
   int h1 = r.y + t->GetHeight() - kTopInset;

   // Borders around track and label area
   dc->SetPen(*wxBLACK_PEN);
   dc->DrawLine(r.x, r.y, r.x + r.width - 1, r.y);      // top
   dc->DrawLine(r.x, r.y, r.x, r.y + r.height - 1);     // left
   dc->DrawLine(r.x, r.y + r.height - 2, r.x + r.width - 1, r.y + r.height - 2);        // bottom
   dc->DrawLine(r.x + r.width - 2, r.y, r.x + r.width - 2, r.y + r.height - 1); // right
   // LL:  This is needed for label, time, and midi tracks
   dc->DrawLine(vrul, r.y, vrul, h1 - 2);                       // between vrulr and track.
   dc->DrawLine(labelw, r.y, labelw, r.y + r.height - 1);       // between vruler and TrackInfo

   if (t->GetLinked()) {
      dc->DrawLine(vrul, h1 - 2, r.x + r.width - 1, h1 - 2);
      dc->DrawLine(vrul, h1 + kTopInset, r.x + r.width - 1,
                   h1 + kTopInset);
   }
}

void TrackPanel::DrawShadow(Track * /* t */ , wxDC * dc, const wxRect r)
{
   AColor::Dark(dc, false);
   // bottom
   dc->DrawLine(r.x + 0, r.y + r.height - 1,
                r.x + 1, r.y + r.height - 1);
   // right
   dc->DrawLine(r.x + r.width - 1, r.y + 0,
                r.x + r.width - 1, r.y + 1);   

   // shadow
   dc->SetPen(*wxBLACK_PEN);
   // bottom
   dc->DrawLine(r.x + 1,       r.y + r.height - 1,
                r.x + r.width, r.y + r.height - 1);
   // right
   dc->DrawLine(r.x + r.width - 1, r.y + 1,
                r.x + r.width - 1, r.y + r.height);
}

/// Returns the string to be displayed in the track label
/// indicating whether the track is mono, left, right, or 
/// stereo and what sample rate it's using.
wxString TrackPanel::TrackSubText(Track * t)
{
   wxString s = wxString::Format(wxT("%dHz"),
                                 (int) (((WaveTrack *) t)->GetRate() +
                                        0.5));
   if (t->GetLinked())
      s = _("Stereo, ") + s;
   else {
      if (t->GetChannel() == Track::MonoChannel)
         s = _("Mono, ") + s;
      else if (t->GetChannel() == Track::LeftChannel)
         s = _("Left, ") + s;
      else if (t->GetChannel() == Track::RightChannel)
         s = _("Right, ") + s;
   }

   return s;
}

/// Handle the menu options that change a track between
/// left channel, right channel, and mono.
int channels[] = { Track::LeftChannel, Track::RightChannel,
   Track::MonoChannel
};

const wxChar *channelmsgs[] = { _("Left Channel"), _("Right Channel"),
   _("Mono")
};

void TrackPanel::OnChannelChange(wxCommandEvent & event)
{
   int id = event.GetId();
   wxASSERT(id >= OnChannelLeftID && id <= OnChannelMonoID);
   wxASSERT(mPopupMenuTarget);
   mPopupMenuTarget->SetChannel(channels[id - OnChannelLeftID]);
   MakeParentPushState(wxString::Format(_("Changed '%s' to %s"),
                                        mPopupMenuTarget->GetName().
                                        c_str(),
                                        channelmsgs[id -
                                                    OnChannelLeftID]),
                       _("Channel"));
   mPopupMenuTarget = NULL;
   Refresh(false);
}

/// Split a stereo track into two tracks...
void TrackPanel::OnSplitStereo(wxCommandEvent &event)
{
   wxASSERT(mPopupMenuTarget);
   Track *partner = mTracks->GetLink(mPopupMenuTarget);
   if (partner)
      partner->SetTeamed(false);
   mPopupMenuTarget->SetLinked(false);
   MakeParentPushState(wxString::Format(_("Split stereo track '%s'"),
                                        mPopupMenuTarget->GetName().
                                        c_str()),
                       _("Split"));

   Refresh(false);
}

/// Merge two tracks into one stereo track ??
void TrackPanel::OnMergeStereo(wxCommandEvent &event)
{
   wxASSERT(mPopupMenuTarget);
   mPopupMenuTarget->SetLinked(true);
   Track *partner = mTracks->GetLink(mPopupMenuTarget);
   if (partner) {
      // Set partner's parameters to match target.
      partner->Merge(*mPopupMenuTarget);

      mPopupMenuTarget->SetChannel(Track::LeftChannel);
      partner->SetChannel(Track::RightChannel);
      partner->SetTeamed(true);
      MakeParentPushState(wxString::Format(_("Made '%s' a stereo track"),
                                           mPopupMenuTarget->GetName().
                                           c_str()),
                          _("Make Stereo"));
   } else
      mPopupMenuTarget->SetLinked(false);

   Refresh(false);
}

///  Set the Display mode based on the menu choice in the Track Menu.
///  Note that gModes MUST BE IN THE SAME ORDER AS THE MENU CHOICES!!
///  const wxChar *gModes[] = { wxT("waveform"), wxT("waveformDB"),
///  wxT("spectrum"), wxT("pitch") };
void TrackPanel::OnSetDisplay(wxCommandEvent & event)
{
   int id = event.GetId();
   wxASSERT(id >= OnWaveformID && id <= OnPitchID);
   wxASSERT(mPopupMenuTarget
            && mPopupMenuTarget->GetKind() == Track::Wave);

#ifdef LOGARITHMIC_SPECTRUM
   WaveTrack *wt = (WaveTrack *) mPopupMenuTarget;
   if (wt->GetDisplay()!= id - OnWaveformID) {
      WaveClipList::Node* it;
      for(it=wt->GetClipIterator(); it; it=it->GetNext()) {
         WaveClip *clip = it->GetData();
         clip->mSpecPxCache->valid = false;
      }
      wt->SetDisplay(id - OnWaveformID);
    }
#else
   ((WaveTrack *) mPopupMenuTarget)->SetDisplay(id - OnWaveformID);
#endif
   Track *partner = mTracks->GetLink(mPopupMenuTarget);
   if (partner)
      ((WaveTrack *) partner)->SetDisplay(id - OnWaveformID);
   MakeParentModifyState();
   mPopupMenuTarget = NULL;
   Refresh(false);
}

/// Sets the sample rate for a track, and if it is linked to
/// another track, that one as well.
void TrackPanel::SetRate(Track * pTrack, double rate)
{
   ((WaveTrack *) pTrack)->SetRate(rate);
   Track *partner = mTracks->GetLink(pTrack);
   if (partner)
      ((WaveTrack *) partner)->SetRate(rate);
   MakeParentPushState(wxString::Format(_("Changed '%s' to %d Hz"),
                                        pTrack->GetName().c_str(), rate),
                       _("Rate Change"));
}

/// Handles the selection from the Format submenu of the
/// track menu.
void TrackPanel::OnFormatChange(wxCommandEvent & event)
{
   int id = event.GetId();
   wxASSERT(id >= On16BitID && id <= OnFloatID);
   wxASSERT(mPopupMenuTarget
            && mPopupMenuTarget->GetKind() == Track::Wave);

   sampleFormat newFormat = int16Sample;

   switch (id) {
   case On16BitID:
      newFormat = int16Sample;
      break;
   case On24BitID:
      newFormat = int24Sample;
      break;
   case OnFloatID:
      newFormat = floatSample;
      break;
   default:
      // ERROR -- should not happen
      break;
   }

   ((WaveTrack *) mPopupMenuTarget)->ConvertToSampleFormat(newFormat);
   Track *partner = mTracks->GetLink(mPopupMenuTarget);
   if (partner)
      ((WaveTrack *) partner)->ConvertToSampleFormat(newFormat);

   MakeParentPushState(wxString::Format(_("Changed '%s' to %s"),
                                        mPopupMenuTarget->GetName().
                                        c_str(),
                                        GetSampleFormatStr(newFormat)),
                       _("Format Change"));

   SetMenuCheck( *mFormatMenu, id );
   mPopupMenuTarget = NULL;
   MakeParentRedrawScrollbars();
   Refresh(false);
}

/// Converts a format enumeration to a wxWindows menu item Id.
int TrackPanel::IdOfFormat( int format )
{
   switch (format) {
   case int16Sample:
      return On16BitID;
   case int24Sample:
      return On24BitID;
   case floatSample:
      return OnFloatID;
   default:
      // ERROR -- should not happen
      wxASSERT( false );
      break;
   }
   return OnFloatID;// Compiler food.
}

/// Puts a check mark at a given position in a menu, clearing all other check marks.
void TrackPanel::SetMenuCheck( wxMenu & menu, int newId )
{
   wxMenuItemList & list = menu.GetMenuItems();
   wxMenuItem * item;
   int id;

   for ( wxwxMenuItemListNode * node = list.GetFirst(); node; node = node->GetNext() )
   {
      item = node->GetData();
      id = item->GetId();
      menu.Check( id, id==newId );
   }
}

const int nRates=7;

///  gRates MUST CORRESPOND DIRECTLY TO THE RATES AS LISTED IN THE MENU!!
///  IN THE SAME ORDER!!
int gRates[nRates] = { 8000, 11025, 16000, 22050, 44100, 48000, 96000 };

/// This function handles the selection from the Rate
/// submenu of the track menu, except for "Other" (/see OnRateOther).
void TrackPanel::OnRateChange(wxCommandEvent & event)
{
   int id = event.GetId();
   wxASSERT(id >= OnRate8ID && id <= OnRate96ID);
   wxASSERT(mPopupMenuTarget
            && mPopupMenuTarget->GetKind() == Track::Wave);

   SetMenuCheck( *mRateMenu, id );
   SetRate(mPopupMenuTarget, gRates[id - OnRate8ID]);

   mPopupMenuTarget = NULL;
   MakeParentRedrawScrollbars();

   Refresh(false);
}

/// Converts a sampling rate to a wxWindows menu item id
int TrackPanel::IdOfRate( int rate )
{
   for(int i=0;i<nRates;i++) {
      if( gRates[i] == rate ) 
         return i+OnRate8ID;
   }
   return OnRateOtherID;
}

void TrackPanel::OnRateOther(wxCommandEvent &event)
{
   wxASSERT(mPopupMenuTarget
            && mPopupMenuTarget->GetKind() == Track::Wave);

   wxString defaultStr;
   defaultStr.Printf(wxT("%d"),
                     (int) (((WaveTrack *) mPopupMenuTarget)->GetRate() +
                            0.5));

   /// \todo Remove artificial constants!!
   /// \todo Make a real dialog box out of this!!
   double theRate;
   do {
      wxString rateStr =
          wxGetTextFromUser(_("Enter a sample rate in Hz (per second) between 1 and 100000:"),
                            _("Set Rate"), defaultStr);

      // AS: Exit if they type in nothing.
      if (wxT("") == rateStr)
         return;

      if (rateStr.ToDouble(&theRate) && theRate >= 1 && theRate <= 100000)
         break;
      else
         wxMessageBox(_("Invalid rate."));

   } while (1);

   SetMenuCheck( *mRateMenu, event.GetId() );
   SetRate(mPopupMenuTarget, theRate);

   mPopupMenuTarget = NULL;
   MakeParentRedrawScrollbars();
   Refresh(false);
}

void TrackPanel::OnSetTimeTrackRange(wxCommandEvent & /*event*/)
{
   TimeTrack *t = (TimeTrack*)mPopupMenuTarget;

   if (t) {
      long lower = t->GetRangeLower();
      long upper = t->GetRangeUpper();
      
      lower = wxGetNumberFromUser(_("Change lower speed limit (%) to:"),
                                  _("Lower speed limit"),
                                  _("Lower speed limit"),
                                  lower,
                                  13,
                                  1200);

      upper = wxGetNumberFromUser(_("Change upper speed limit (%) to:"),
                                  _("Upper speed limit"),
                                  _("Upper speed limit"),
                                  upper,
                                  lower+1,
                                  1200);

      if( lower >= 13 && upper <= 1200 && lower < upper ) {
         t->SetRangeLower(lower);
         t->SetRangeUpper(upper);
         MakeParentPushState(wxString::Format(_("Set range to '%d' - '%d'"),
                                              lower,
                                              upper),
                             _("Set Range"));
         Refresh(false);
      }
   }
   mPopupMenuTarget = NULL;
}

/// AS: Move a track up or down, depending.
void TrackPanel::OnMoveTrack(wxCommandEvent & event)
{
   wxASSERT(event.GetId() == OnMoveUpID || event.GetId() == OnMoveDownID);
   if (mTracks->Move(mPopupMenuTarget, OnMoveUpID == event.GetId())) {
      MakeParentPushState(wxString::Format(_("Moved '%s' %s"),
                                           mPopupMenuTarget->GetName().
                                           c_str(),
                                           event.GetId() ==
                                           OnMoveUpID ? _("up") :
                                           _("down")),
                          _("Move Track"));
      Refresh(false);
   }
}

/// This only applies to MIDI tracks.  Presumably, it shifts the
/// whole sequence by an octave.
void TrackPanel::OnChangeOctave(wxCommandEvent & event)
{
   wxASSERT(event.GetId() == OnUpOctaveID
            || event.GetId() == OnDownOctaveID);
   wxASSERT(mPopupMenuTarget->GetKind() == Track::Note);
   NoteTrack *t = (NoteTrack *) mPopupMenuTarget;

   bool bDown = (OnDownOctaveID == event.GetId());
   t->SetBottomNote(t->GetBottomNote() + ((bDown) ? -12 : 12));

   MakeParentModifyState();
   Refresh(false);
}

void TrackPanel::OnSetName(wxCommandEvent &event)
{
   Track *t = mPopupMenuTarget;

   if (t) {
      wxString defaultStr = t->GetName();
      wxString newName = wxGetTextFromUser(_("Change track name to:"),
                                           _("Track Name"), defaultStr);
      if (newName != wxT(""))
         t->SetName(newName);
      MakeParentPushState(wxString::Format(_("Renamed '%s' to '%s'"),
                                           defaultStr.c_str(),
                                           newName.c_str()),
                          _("Name Change"));

      Refresh(false);
   }
}

/// Cut selected text if cut menu item is selected
void TrackPanel::OnCutSelectedText(wxCommandEvent &event)
{
   LabelTrack *lt = (LabelTrack *)mPopupMenuTarget;
   if (lt->CutSelectedText()) {
      MakeParentPushState(_("Modified Label"),
                          _("Label Edit"),
                          true /* consolidate */);
   }
   RefreshTrack(lt, true);
}

/// Copy selected text if copy menu item is selected
void TrackPanel::OnCopySelectedText(wxCommandEvent &event)
{
   LabelTrack *lt = (LabelTrack *)mPopupMenuTarget;
   lt->CopySelectedText();
   RefreshTrack(lt, true);
}

/// paste selected text if p`aste menu item is selected
void TrackPanel::OnPasteSelectedText(wxCommandEvent &event)
{
   LabelTrack *lt = (LabelTrack *)mPopupMenuTarget;
   if (lt->PasteSelectedText(mViewInfo->sel0, mViewInfo->sel1)) {
      MakeParentPushState(_("Modified Label"),
                          _("Label Edit"),
                          true /* consolidate */);
   }
   RefreshTrack(lt, true);
}

// Small helper class to enumerate all fonts in the system
// We use this because the default implementation of
// wxFontEnumerator::GetFacenames() has changed between wx2.6 and 2.8
class TrackPanelFontEnumerator : public wxFontEnumerator
{
public:
   TrackPanelFontEnumerator(wxArrayString* fontNames) :
      mFontNames(fontNames) {}
   
   virtual bool OnFacename(const wxString& font)
   {
      mFontNames->Add(font);
      return true;
   }
   
private:
   wxArrayString* mFontNames;
};

void TrackPanel::OnSetFont(wxCommandEvent &event)
{
   wxArrayString facenames;
   TrackPanelFontEnumerator fontEnumerator(&facenames);
   fontEnumerator.EnumerateFacenames(wxFONTENCODING_SYSTEM, false);

   wxString facename = gPrefs->Read(wxT("/GUI/LabelFontFacename"), wxT(""));
   long fontsize = gPrefs->Read(wxT("/GUI/LabelFontSize"), 12);

   wxDialog dlg(this, wxID_ANY, wxString(_("Label Track Font")));
   ShuttleGui S(&dlg, eIsCreating);
   wxListBox *lb;
   wxSpinCtrl *sc;

   S.StartVerticalLay(true);
   {
      S.StartMultiColumn(2, wxEXPAND);
      {
         S.SetStretchyRow(0);
         S.SetStretchyCol(1);

         S.AddPrompt(_("Face name"));
         lb = new wxListBox(&dlg, wxID_ANY,
                            wxDefaultPosition,
                            wxDefaultSize,
                            facenames,
                            wxLB_SINGLE);
         lb->SetSelection(facenames.Index(facename));
         S.AddWindow(lb, wxALIGN_LEFT | wxEXPAND | wxALL);

         S.AddPrompt(_("Face size"));
         sc = new wxSpinCtrl(&dlg, wxID_ANY,
                             wxString::Format(wxT("%d"), fontsize),
                             wxDefaultPosition,
                             wxDefaultSize,
                             wxSP_ARROW_KEYS,
                             8, 48, fontsize);
         S.AddWindow(sc, wxALIGN_LEFT | wxALL);
      }
      S.EndMultiColumn();
      S.AddStandardButtons();
   }
   S.EndVerticalLay();

   dlg.Layout();
   dlg.Fit();
   dlg.CenterOnParent();
   if (dlg.ShowModal() == wxID_CANCEL) {
      return;
   }
   
   gPrefs->Write(wxT("/GUI/LabelFontFacename"), lb->GetStringSelection());
   gPrefs->Write(wxT("/GUI/LabelFontSize"), sc->GetValue());

   LabelTrack::ResetFont();

   Refresh(false);
}

/// Determines which track is under the mouse 
///  @param mouseX - mouse X position.
///  @param mouseY - mouse Y position.
///  @param label  - true iff the X Y position is relative to side-panel with the labels in it.
///  @param link - true iff we should consider a hit in any linked track as a hit.
///  @param *trackRect - returns track rectangle.
///  @param *trackNum  - returns track number.
Track *TrackPanel::FindTrack(int mouseX, int mouseY, bool label, bool link,
                              wxRect * trackRect, int *trackNum)
{
   wxRect r;
   r.x = 0;
   r.y = -mViewInfo->vpos;
   r.y += kTopInset;
   GetSize(&r.width, &r.height);

   if (label) {
      r.width = GetLeftOffset();
   } else {
      r.x = GetLeftOffset();
      r.width -= GetLeftOffset();
   }

   TrackListIterator iter(mTracks);

   int n = 1;

   for (Track * t = iter.First(); t;
        r.y += t->GetHeight(), n++, t = iter.Next()) {
      r.height = t->GetHeight();

      if (link && t->GetLinked()) {
         Track *link = mTracks->GetLink(t);
         r.height += link->GetHeight();
      }
      

      //Determine whether the mouse is inside 
      //the current rectangle.  If so, recalculate
      //the proper dimensions and return.
      if (r.Inside(mouseX, mouseY)) {
         if (trackRect) {
            r.y -= kTopInset;
            if (label) {
               r.x += kLeftInset;
               r.width -= kLeftInset;
               r.y += kTopInset;
               r.height -= kTopInset;
            }
            *trackRect = r;
         }
         if (trackNum)
            *trackNum = n;
         return t;
      }
   }

   if (mouseY >= r.y && trackNum)
      *trackNum = n - 1;

   return NULL;
}


/// This tells you the (1-based) index of the track you pass to it.
int TrackPanel::FindTrackNum(Track * target)
{
   if(!target) return 0;

   TrackListIterator iter(mTracks);
   int n = 1;

   for (Track * t = iter.First(); t;
        n++, t = iter.Next()) 
      {
         if(target == t)
            {
               return n;
            }

      }

   return 0;
}

/// This finds the rectangle of a given track, either the
/// of the label 'adornment' or the track itself
wxRect TrackPanel::FindTrackRect(Track * target, bool label)
{
   if(!target) return wxRect(0,0,0,0);

   bool linked = target->GetLinked();

   wxRect r;
   r.x = 0;
   r.y = -mViewInfo->vpos;
   r.y += kTopInset;
   GetSize(&r.width, &r.height);

   TrackListIterator iter(mTracks);

   int n = 1;

   for (Track * t = iter.First(); t;
        r.y += t->GetHeight(), n++, t = iter.Next()) 
      {
         r.height = t->GetHeight();
         
         if (linked && t->GetLinked())
            {
               Track *link = mTracks->GetLink(t);
               r.height += link->GetHeight();
            }
         
         if ( t == target)
            {
               r.y -= kTopInset;
               if (label) 
                  {
                     r.x += kLeftInset;
                     r.width -= kLeftInset;
                     r.y += kTopInset;
                     r.height -= kTopInset;
                  }
               return r;
            }

      }
   return r;
}

#ifdef EXPERIMENTAL_RULER_AUTOSIZE
int TrackPanel::GetVRulerWidth() const 
{
//return 36;
   return vrulerSize.x;
}
#endif //EXPERIMENTAL_RULER_AUTOSIZE

/// Displays the bounds of the selection in the status bar.
void TrackPanel::DisplaySelection()
{
   if (!mListener)
      return;

   // DM: Note that the Selection Bar can actually MODIFY the selection
   // if snap-to mode is on!!!
   mListener->TP_DisplaySelection();
}

bool TrackPanel::MoveClipToTrack(WaveClip *clip,
                                 WaveTrack* src, WaveTrack* dst)
{
   WaveClip *clip2 = NULL;
   WaveTrack *src2 = NULL;
   WaveTrack *dst2 = NULL;

   // Make sure we have the first track of two stereo tracks
   // with both source and destination
   if (!src->GetLinked() && mTracks->GetLink(src)) {
      src = (WaveTrack*)mTracks->GetLink(src);
      if (mCapturedClipArray.GetCount() == 2) {
         if (mCapturedClipArray[0].clip == clip)
            clip = mCapturedClipArray[1].clip;
         else
            clip = mCapturedClipArray[0].clip;
      }
   }
   if (!dst->GetLinked() && mTracks->GetLink(dst))
      dst = (WaveTrack*)mTracks->GetLink(dst);

   if (mCapturedClipArray.GetCount() == 2) {
      if (mCapturedClipArray[0].clip == clip)
         clip2 = mCapturedClipArray[1].clip;
      else
         clip2 = mCapturedClipArray[0].clip;
   }

   // Get the second track of two stereo tracks
   src2 = (WaveTrack*)mTracks->GetLink(src);
   dst2 = (WaveTrack*)mTracks->GetLink(dst);

   if ((src2 && !dst2) || (dst2 && !src2))
      return false; // cannot move stereo- to mono track or other way around

   if (!dst->CanInsertClip(clip))
      return false;

   if (clip2) {
      if (!dst2->CanInsertClip(clip2))
         return false;
   }

   src->MoveClipToTrack(clip, dst);
   if (src2)
      src2->MoveClipToTrack(clip2, dst2);

   if (mCapturedClipArray.GetCount() == 2) {
      if (mCapturedClipArray[0].clip == clip) {
         mCapturedClipArray[0].track = dst;
         mCapturedClipArray[1].track = dst2;
      }
      else {
         mCapturedClipArray[0].track = dst2;
         mCapturedClipArray[1].track = dst;
      }
   }
   else {
      mCapturedClipArray[0].track = dst;
   }

   return true;
}

Track *TrackPanel::GetFocusedTrack()
{
   return mAx->GetFocus();
}

void TrackPanel::SetFocusedTrack( Track *t )
{
   // Make sure we always have the first linked track of a stereo track
   if (t && !t->GetLinked() && mTracks->GetLink(t))
      t = (WaveTrack*)mTracks->GetLink(t);

   AudacityProject *p = GetActiveProject();
   
   if (p && p->HasKeyboardCapture()) {
      wxCommandEvent e(EVT_RELEASE_KEYBOARD);
      e.SetEventObject(this);
      GetParent()->GetEventHandler()->ProcessEvent(e);
   }

   if (t && t->GetKind() == Track::Label) {
      wxCommandEvent e(EVT_CAPTURE_KEYBOARD);
      e.SetEventObject(this);
      GetParent()->GetEventHandler()->ProcessEvent(e);
   }

   mAx->SetFocus( t );
   Refresh( false );
}

void TrackPanel::OnSetFocus(wxFocusEvent & event)
{
   SetFocusedTrack( GetFocusedTrack() );
   Refresh( false );
}

void TrackPanel::OnKillFocus(wxFocusEvent & event)
{
   Refresh( false);
}

/**********************************************************************

  TrackInfo code is destined to move out of this file.
  Code should become a lot cleaner when we have sizers.  

**********************************************************************/

TrackInfo::TrackInfo(wxWindow * pParentIn)
{
   //To prevent flicker, we create an initial set of 16 sliders
   //which won't ever be shown.
   pParent = pParentIn;
   int i;
   for(i=0; i<16; i++)
      MakeMoreSliders();
}

TrackInfo::~TrackInfo()
{
   unsigned int i;
   for(i=0; i<mGains.Count(); i++)
      delete mGains[i];
   for(i=0; i<mPans.Count(); i++)
      delete mPans[i];
}

#ifdef EXPERIMENTAL_RULER_AUTOSIZE
int TrackInfo::GetTitleWidth() const 
{
   return 100;
}
#endif //EXPERIMENTAL_RULER_AUTOSIZE

void TrackInfo::GetCloseBoxRect(const wxRect r, wxRect & dest) const
{
   dest.x = r.x;
   dest.y = r.y;
   dest.width = 16;
   dest.height = 16;
}

void TrackInfo::GetTitleBarRect(const wxRect r, wxRect & dest) const
{
   dest.x = r.x + 16;
   dest.y = r.y;
   dest.width = GetTitleWidth() - r.x - 16;
   dest.height = 16;
}

void TrackInfo::GetMuteSoloRect(const wxRect r, wxRect & dest, bool solo, bool bHasSoloButton) const
{
#ifdef NOT_USED
   dest.x = r.x + 8;
   dest.y = r.y + 50;
   dest.width = 36;
   dest.height = 16;

   if (solo)
      dest.x += 36 + 8;
#else
   dest.x = r.x ;
   dest.y = r.y + 50;
   dest.width = 48;
   dest.height = 16;

   if( !bHasSoloButton )
   {
      dest.width +=48;
   }
   else if (solo)
   {
      dest.x += 48;
   }
#endif

}

void TrackInfo::GetGainRect(const wxRect r, wxRect & dest) const
{
   dest.x = r.x + 7;
   dest.y = r.y + 70;
   dest.width = 84;
   dest.height = 25;
}

void TrackInfo::GetPanRect(const wxRect r, wxRect & dest) const
{
   dest.x = r.x + 7;
   dest.y = r.y + 100;
   dest.width = 84;
   dest.height = 25;
}

void TrackInfo::GetMinimizeRect(const wxRect r, wxRect &dest, bool minimized) const
{
   dest.x = r.x + 1;
   dest.width = 94;
   dest.y = r.y + r.height - 18;
   dest.height = 15;
}

void TrackInfo::DrawBordersWithin(wxDC * dc, const wxRect r, bool bHasMuteSolo )
{
   dc->SetPen(*wxBLACK_PEN);
   // These black lines are actually within TrackInfo...
   dc->DrawLine(r.x, r.y + 16, GetTitleWidth(), r.y + 16);      // title bar
   dc->DrawLine(r.x + 16, r.y, r.x + 16, r.y + 16);     // close box

   if( bHasMuteSolo && (r.height > (66+18) ))
   {
      dc->DrawLine(r.x, r.y + 50, GetTitleWidth(), r.y + 50);  // bevel above mute/solo
      dc->DrawLine(r.x+48 , r.y+50, r.x+48, r.y + 66);         // line between mute/solo
      dc->DrawLine(r.x, r.y + 66, GetTitleWidth(), r.y + 66);  // bevel below mute/solo
   }

   dc->DrawLine(r.x, r.y + r.height - 19, GetTitleWidth(), r.y + r.height - 19);  // minimize bar
}

#ifdef EXPERIMENTAL_RULER_AUTOSIZE
void TrackInfo::DrawBackground(wxDC * dc, const wxRect r, bool bSelected,
   bool bHasMuteSolo, const int labelw, const int vrul)
{
   // fill in label
   wxRect fill = r;
   fill.width = labelw-4;
   AColor::MediumTrackInfo(dc, bSelected);
   dc->DrawRectangle(fill); 

   if( bHasMuteSolo )
   {
      fill=wxRect( r.x+1, r.y+17, vrul-6, 32); 
      AColor::BevelTrackInfo( *dc, true, fill );

      fill=wxRect( r.x+1, r.y+67, fill.width, r.height-87); 
      AColor::BevelTrackInfo( *dc, true, fill );
   }
   else
   {
      fill=wxRect( r.x+1, r.y+17, vrul-6, r.height-37); 
      AColor::BevelTrackInfo( *dc, true, fill );
   }
}
#else //!EXPERIMENTAL_RULER_AUTOSIZE
void TrackInfo::DrawBackground(wxDC * dc, const wxRect r, bool bSelected,
   bool bHasMuteSolo, const int labelw)
{
   // fill in label
   wxRect fill = r;
   fill.width = labelw - r.x+1;
   AColor::MediumTrackInfo(dc, bSelected);
   dc->DrawRectangle(fill); 

   if( bHasMuteSolo )
   {
      fill=wxRect( r.x+1, r.y+17, fill.width - 39, 32); 
      AColor::BevelTrackInfo( *dc, true, fill );

      fill=wxRect( r.x+1, r.y+67, fill.width, r.height-87); 
      AColor::BevelTrackInfo( *dc, true, fill );
   }
   else
   {
      fill=wxRect( r.x+1, r.y+17, fill.width - 39, r.height-37); 
      AColor::BevelTrackInfo( *dc, true, fill );
   }
}
#endif //EXPERIMENTAL_RULER_AUTOSIZE

void TrackInfo::GetTrackControlsRect(const wxRect r, wxRect & dest) const
{
   wxRect top;
   wxRect bot;

   GetTitleBarRect(r, top);
   GetMinimizeRect(r, bot, false);

   dest.x = r.x;
   dest.width = GetTitleWidth() - dest.x;
   dest.y = top.GetBottom() + 2;
   dest.height = bot.GetTop() - top.GetBottom() - 2;
}

void TrackInfo::DrawCloseBox(wxDC * dc, const wxRect r, bool down)
{
   const int xSize=7;
   const int offset=5;

#ifdef EXPERIMENTAL_THEMING
   wxPen pen( theTheme.Colour( clrTrackPanelText ));
   dc->SetPen( pen );
#else
   dc->SetPen(*wxBLACK_PEN);
#endif

   // close "x"
   dc->DrawLine(r.x + offset  ,         r.y + offset, r.x + offset+xSize  , r.y + offset+xSize);
   dc->DrawLine(r.x + offset+1,         r.y + offset, r.x + offset+xSize+1, r.y + offset+xSize);
   dc->DrawLine(r.x + xSize + offset  , r.y + offset, r.x + offset,         r.y + offset+xSize);
   dc->DrawLine(r.x + xSize + offset-1, r.y + offset, r.x + offset-1,       r.y + offset+xSize);
   wxRect bev;
   GetCloseBoxRect(r, bev);
   bev.Inflate(-1, -1);
   AColor::BevelTrackInfo(*dc, !down, bev);
}

void TrackInfo::DrawTitleBar(wxDC * dc, const wxRect r, Track * t,
                              bool down)
{
   wxRect bev;
   GetTitleBarRect(r, bev);
   bev.Inflate(-1, -1);

   // Draw title text
   SetTrackInfoFont(dc);
   wxString titleStr = t->GetName();
   int allowableWidth = GetTitleWidth() - 38 - kLeftInset;
   long textWidth, textHeight;
   dc->GetTextExtent(titleStr, &textWidth, &textHeight);
   while (textWidth > allowableWidth) {
      titleStr = titleStr.Left(titleStr.Length() - 1);
      dc->GetTextExtent(titleStr, &textWidth, &textHeight);
   }
   // wxGTK leaves little scraps (antialiasing?) of the
   // characters if they are repeatedly drawn.  This
   // happens when holding down mouse button and moving
   // in and out of the title bar.  So clear it first.
   AColor::MediumTrackInfo(dc, t->GetSelected());
   dc->DrawRectangle(bev);
   dc->DrawText(titleStr, r.x + 19, r.y + 2);

   // Pop-up triangle
#ifdef EXPERIMENTAL_THEMING
   wxPen pen( theTheme.Colour( clrTrackPanelText ));
   dc->SetPen( pen );
#else
   dc->SetPen(*wxBLACK_PEN);
#endif

   int xx = r.x + GetTitleWidth() - 16 - kLeftInset;
   int yy = r.y + 5;
   int triWid = 11;
   while (triWid >= 1) {
      dc->DrawLine(xx, yy, xx + triWid, yy);
      xx++;
      yy++;
      triWid -= 2;
   }

   AColor::BevelTrackInfo(*dc, !down, bev);
}

/// Draw the Mute or the Solo button, depending on the value of solo.
void TrackInfo::DrawMuteSolo(wxDC * dc, const wxRect r, Track * t,
                              bool down, bool solo, bool bHasSoloButton)
{
   wxRect bev;
   if( solo && !bHasSoloButton )
      return;
   GetMuteSoloRect(r, bev, solo, bHasSoloButton);
   bev.Inflate(-1, -1);
   
   if (bev.y + bev.height >= r.y + r.height - 19)
      return; // don't draw mute and solo buttons, because they don't fit into track label
      
   AColor::MediumTrackInfo( dc, t->GetSelected() );
   if( solo )
   {
      if( t->GetSolo() )
      {
         AColor::Solo(dc, t->GetSolo(), t->GetSelected());
      }
   }
   else
   {
      if( t->GetMute() )
      {
         AColor::Mute(dc, t->GetMute(), t->GetSelected(), t->GetSolo());
      }
   }
   //(solo) ? AColor::Solo(dc, t->GetSolo(), t->GetSelected()) :
   //    AColor::Mute(dc, t->GetMute(), t->GetSelected(), t->GetSolo());
   dc->SetPen( *wxTRANSPARENT_PEN );//No border!
   dc->DrawRectangle(bev);

   long textWidth, textHeight;
   wxString str = (solo) ? _("Solo") : _("Mute");

   SetTrackInfoFont(dc);
   dc->GetTextExtent(str, &textWidth, &textHeight);
   dc->DrawText(str, bev.x + (bev.width - textWidth) / 2, bev.y);

   AColor::BevelTrackInfo(*dc, (solo?t->GetSolo():t->GetMute()) == down, bev);

   if (solo && !down) {
      // Update the mute button, which may be grayed out depending on
      // the state of the solo button.
      DrawMuteSolo(dc, r, t, false, false, bHasSoloButton);
   }
}

void TrackInfo::DrawMinimize(wxDC * dc, const wxRect r, Track * t, bool down, bool minimized)
{
   wxRect bev;
   GetMinimizeRect(r, bev, minimized);
    
   AColor::MediumTrackInfo(dc, t->GetSelected());
   dc->DrawRectangle(bev);
    

   // Calculate center
   int x = bev.x + bev.width / 2;
   int y = bev.y + bev.height / 2;

   wxPoint pts[3];

   if (minimized)
   {
      pts[0].x = x - 4;
      pts[0].y = y - 3;
      pts[1].x = x + 4;
      pts[1].y = y - 3;
      pts[2].x = x;
      pts[2].y = y + 3;

   } else
   {
      pts[0].x = x;
      pts[0].y = y - 3;
      pts[1].x = x + 4;
      pts[1].y = y + 3;
      pts[2].x = x - 4;
      pts[2].y = y + 3;
   }

#ifdef EXPERIMENTAL_THEMING
   wxPen pen( theTheme.Colour( clrTrackPanelText ));
   wxBrush brush( theTheme.Colour( clrTrackPanelText ));
   dc->SetPen( pen );
   dc->SetBrush( brush );
#else
   AColor::Dark(dc, t->GetSelected());
#endif
   
   dc->DrawPolygon(3, pts);

   AColor::BevelTrackInfo(*dc, !down, bev);
}

void TrackInfo::MakeMoreSliders()
{
   wxRect r(0, 0, 1000, 1000);
   wxRect gainRect;
   wxRect panRect;

   GetGainRect(r, gainRect);
   GetPanRect(r, panRect);

   /* i18n-hint: Title of the Gain slider, used to adjust the volume */
   LWSlider *slider = new LWSlider(pParent, _("Gain"),
                                   wxPoint(gainRect.x, gainRect.y),
                                   wxSize(gainRect.width, gainRect.height),
                                   DB_SLIDER);
   mGains.Add(slider);
   
   /* i18n-hint: Title of the Pan slider, used to move the sound left or right stereoscopically */
   slider = new LWSlider(pParent, _("Pan"),
                         wxPoint(panRect.x, panRect.y),
                         wxSize(panRect.width, panRect.height),
                         PAN_SLIDER);
   mPans.Add(slider);
}

void TrackInfo::EnsureSufficientSliders(int index)
{
   while (mGains.Count() < (unsigned int)index+1 ||
          mPans.Count() < (unsigned int)index+1)
      MakeMoreSliders();
}

void TrackInfo::DrawSliders(wxDC *dc, WaveTrack *t, wxRect r, int index)
{
   wxRect gainRect;
   wxRect panRect;

   EnsureSufficientSliders( index );

   GetGainRect(r, gainRect);
   GetPanRect(r, panRect);

   if (gainRect.y + gainRect.height < r.y + r.height - 19) {
      mGains[index]->Move(wxPoint(gainRect.x, gainRect.y));
      mGains[index]->Set(t->GetGain());
      mGains[index]->OnPaint(*dc, t->GetSelected());
   }

   if (panRect.y + panRect.height < r.y + r.height - 19) {
      mPans[index]->Move(wxPoint(panRect.x, panRect.y));
      mPans[index]->Set(t->GetPan());
      mPans[index]->OnPaint(*dc, t->GetSelected());
   }
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
// arch-tag: 5bb3d18e-9ba7-47c3-beef-29f5d791442a

