/**********************************************************************

  Audacity: A Digital Audio Editor

  Experimental.h

  Dominic Mazzoni
  James Crook

  Used for includes and #defines for experimental features.

  When the features become mainstream the include files will 
  move out of here and into the files which need them.  The
  #defines will then be retired.



  JKC: This file solves a problem of how to avoid forking the
  code base when working on new features e.g:
    - Additional displays in Audacity
    - Modular architecture.
  Add #defines in here for the new features, and make your code
  conditional on those #defines.

**********************************************************************/

#ifndef __EXPERIMENTAL__
#define __EXPERIMENTAL__

//Uncomment the next #define to enable experimental features.
#define EXPERIMENTAL_FEATURES

// Comment out the next two lines if you want to disable 'experimental theming'
// Work in progress, June-2008.
//#define EXPERIMENTAL_THEMING
#define EXPERIMENTAL_THEME_PREFS

//The next few enable different kinds of label/wavetrack pairing.
//You should probably only enable one at a time.
//The next line enables a menu-based system
//#define EXPERIMENTAL_STICKY_TRACKS
//The next block enables a positional-based system.
//LABEL_LINKING only links label tracks, while FULL_LINKING links wave and label tracks
#define EXPERIMENTAL_POSITION_LINKING
#ifdef EXPERIMENTAL_POSITION_LINKING
   #define EXPERIMENTAL_FULL_LINKING
   //#define EXPERIMENTAL_LABEL_LINKING
#endif

//Next line enables Mic monitoring at times when it was previously off.
//More work is needed as after recording or playing it results in an 
//unwanted record-cursor on the wave track.
//#define EXPERIMENTAL_EXTRA_MONITORING

//#define EXPERIMENTAL_ROLL_UP_DIALOG
//#define RIGHT_ALIGNED_TEXTBOXES
//#define EXPERIMENTAL_VOICE_DETECTION

//uncomment this line to enable On-Demand loading of PCM files.  
//for now it is incomplete so it will load instantly but 
//the display of summary data will not be there (one can still see sample data if zoomed in)
#define EXPERIMENTAL_ONDEMAND

//FFmpeg integration. Some FFmpeg-related changes are NOT ifdef'ed,
//as they are harmless (like - a few mockup methods in Import* classes)
// To enable ffmpeg support #define USE_FFMPEG in config*.h, as for any other
// optional importer library (windows users: edit win/configwin.h)

// Effect categorisation. Adds support for arranging effects in categories
// and displaying those categories as submenus in the Effect menu.
// This was a 2008 GSoC project that was making good progress at the half-way point
// but then the student didn't contribute after that.  It needs a bit of work to finish it off.
// As a minimum, if this is turned on for a release,
// it should have an easy mechanism to disable it at run-time, such as a menu item or a pref,
// preferrably disabled until other work is done.  Martyn 22/12/2008.
//#define EFFECT_CATEGORIES

// Andreas Micheler, 20.Nov 2007: 
// A spectrumLogF-like view mode with notes quantization.
// Just select the "Find Notes" checkbox in the spectrum prefs 
// to activate it instead of the Spectrum log(f) mode.
//#define EXPERIMENTAL_FIND_NOTES

// AM, 22.Nov 2007
// Skip Points support in the spectrum view mode.
//#define EXPERIMENTAL_FFT_SKIP_POINTS

// AM, 22.Nov 2007: 
// A Frequency Grid for the Spectrum Log(f) & Find Notes modes
//#define EXPERIMENTAL_FFT_Y_GRID

// AM, 22.Nov 2007: 
// Saves the default view mode for wave tracks.
#define EXPERIMENTAL_SAVE_DEFAULT_VIEW

#ifdef EXPERIMENTAL_FEATURES
   // The first experimental feature is a notebook that adds
   // a tabbed divider to the project.
   //#define EXPERIMENTAL_NOTEBOOK
   // The notebook in turn can contain:
   // 1. The Nyquist Inspector, which is a browser for the objects in 
   // Audacity.
   //#define EXPERIMENTAL_NYQUIST_INSPECTOR
   // 2. The Vocal Studio, a screen for working with vocal sounds
   // particularly vowel sounds.
   //#define EXPERIMENTAL_VOCAL_STUDIO
   // 3. The Audacity Tester is an extended version of the benchmarks
   // display.  The crucial idea is to be able to compare waveforms
   // where effects have been applied by audacity but using different 
   // block-sizes.  This should give high confidence that we don't
   // suffer from end-effects on buffers, e.g. losing one sample on
   // each buffer.
   //#define EXPERIMENTAL_AUDACITY_TESTER

   // A long term plan is to use dso's and dlls for Audacity extensions
   // These are 'WX' plug ins that manage their own displays using
   // wxWindows.
   //#define EXPERIMENTAL_WX_PLUG_INS
#endif

//If you want any of these files, ask JKC.  They are not
//yet checked in to Audacity CVS as of 10-Oct-2004
#ifdef EXPERIMENTAL_NOTEBOOK
#include "widgets/GuiFactory.h"
#include "widgets/APanel.h"
extern void AddPages(   AudacityProject * pProj, GuiFactory & Factory,  wxNotebook  * pNotebook );
#endif

#ifdef EXPERIMENTAL_NYQUIST_INSPECTOR
#include "NyquistAdapter.h"
#endif

#ifdef EXPERIMENTAL_AUDACITY_TESTER
#endif

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
// arch-tag:

