/**********************************************************************

  Audacity: A Digital Audio Editor

  Effect.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_EFFECT__
#define __AUDACITY_EFFECT__

#include <wx/dynarray.h>
#include <wx/string.h>

class wxFrame;
class wxProgressDialog;

#ifdef __WXMAC__
#include "WaveTrack.h"
#else
#include "../WaveTrack.h"
#endif

class Effect;

WX_DEFINE_ARRAY(Effect *, EffectArray);

class Effect {

 //
 // public static methods
 //
 // Used by the outside program to register the list of effects and retrieve
 // them by index number, usually when the user selects one from a menu.
 //
 public:
   static int RegisterEffect(Effect * f);
   static int GetNumEffects();
   static Effect *GetEffect(int i);

 // 
 // public methods
 //
 // Used by the outside program to determine properties of an effect and
 // apply the effect to one or more tracks.
 // 
 public:
   // Each subclass of Effect should override this method.
   // This name will go in the menu bar;
   // append "..." if your effect pops up a dialog
   virtual wxString GetEffectName() = 0;
   
   // Each subclass of Effect should override this method.
   // This name will go in the progress dialog, but can be used
   // elsewhere, and it should describe what is being done.
   // For example, if the effect is "Filter", the action is
   // "Filtering", or if the effect is "Bass Boost", the action
   // is "Boosting Bass Frequencies".
   virtual wxString GetEffectAction() = 0;

   // Returns true on success.  Will only operate on tracks that
   // have the "selected" flag set to true, which is consistent with
   // Audacity's standard UI.
   bool DoEffect(wxWindow *parent, TrackList *list, double t0, double t1);

 //
 // protected virtual methods
 //
 // Each subclass of Effect overrides one or more of these methods to
 // do its processing.
 //
 protected:
   // The constructor.  Called once at the beginning of the program.
   // Avoid allocating memory or doing time-consuming processing here.
   Effect();
 
   // Called once each time an effect is called.  Perform any initialization;
   // make sure that the effect can be performed on the selected tracks and
   // return false otherwise
   virtual bool Init() {
      return true;
   }

   // If necessary, open a dialog to get parameters from the user.
   // This method will not always be called (for example if a user
   // repeats an effect) but if it is called, it will be called
   // after Init.
   virtual bool PromptUser() {
      return true;
   }
      
   // Actually do the effect here.
   virtual bool Process() = 0;

   // clean up any temporary memory
   virtual void End() {
   }

 //
 // protected data
 //
 // The Effect base class will set these variables, some or all of which
 // may be needed by any particular subclass of Effect.
 //
 protected:
   wxWindow   *mParent;
   TrackList  *mTracks;      // the complete list of all tracks
   TrackList  *mWaveTracks;  // effects which do not add or remove tracks: use this
   double      mT0;
   double      mT1;

 //
 // protected methods
 //
 // These methods can be used by subclasses of Effect in order to display a
 // progress dialog or perform common calculations
 //
 protected:
   // The starting and ending points of effects are normally stored in
   // seconds.  This converts it to samples for a particular track.
   // Pass a track in t and get back the starting sample and number of
   // samples to process in s0 and slen, respectively.
   void GetSamples(WaveTrack *t, sampleCount *s0, sampleCount *slen);
 
   // The Progress methods all return true if the user has cancelled;
   // you should exit immediately if this happens (cleaning up memory
   // is okay, but don't try to undo).
 
   // Pass a fraction between 0.0 and 1.0
   bool TotalProgress(double frac);
   
   // Pass a fraction between 0.0 and 1.0, for the current track
   // (when doing one track at a time)
   bool TrackProgress(int whichTrack, double frac);
 
   // Pass a fraction between 0.0 and 1.0, for the current track group
   // (when doing stereo groups at a time)
   bool TrackGroupProgress(int whichGroup, double frac);
 
 //
 // private methods
 //
 // Used only by the base Effect class
 //
 private:
   void CountWaveTracks();
 
 //
 // private data
 //
 // Used only by the base Effect class
 //
 private:
   static EffectArray *mEffects;
   
   int mNumTracks;
   int mNumGroups;
   
   wxProgressDialog *mProgress;
};

// Utility functions

float TrapFloat(float x, float min, float max);
double TrapDouble(double x, double min, double max);
long TrapLong(long x, long min, long max);

#endif
