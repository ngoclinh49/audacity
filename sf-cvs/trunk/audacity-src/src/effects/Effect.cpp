/**********************************************************************

  Audacity: A Digital Audio Editor

  Effect.cpp

  Dominic Mazzoni

*******************************************************************//**

\class Effect
\brief Base class for many of the effects in Audacity.

*//****************************************************************//**

\class EffectDialog
\brief New (Jun-2006) base class for effects dialogs.  Likely to get 
greater use in future.

*//*******************************************************************/

#include "../Audacity.h"

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/sizer.h>
#include <wx/timer.h>

#include "Effect.h"
#include "../AudioIO.h"
#include "../Mix.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../WaveTrack.h"

//
// public static methods
//

EffectArray Effect::mEffects;
int Effect::sNumEffects = 0;
double Effect::sDefaultGenerateLen = 30.0;

// Some static variables used for Repeat Last Effect.
int Effect::LastType=0;
int Effect::LastIndex=0;
Effect * Effect::pLastEffect=NULL;

wxString Effect::StripAmpersand(const wxString& str)
{
   wxString strippedStr = str;
   strippedStr.Replace(wxT("&"), wxT(""));
   return strippedStr;
}

void Effect::RegisterEffect(Effect *f, int NewFlags)
{
   f->mID = sNumEffects;
   sNumEffects++;
   if( NewFlags != 0)
      f->SetEffectFlags( NewFlags );

   // Insert the effect into the list in alphabetical order
   // A linear search is good enough as long as there are
   // only a few dozen or even a few hundred effects.
   wxString name = StripAmpersand(f->GetEffectName());
   int len = mEffects.GetCount();
   int i;
   for(i=0; i<len; i++)
      if (name.CmpNoCase(StripAmpersand(mEffects[i]->GetEffectName())) < 0) {
         mEffects.Insert(f, i);
         break;
      }
   if (i==len)
      mEffects.Add(f);
}

void Effect::UnregisterEffects()
{
   for(int i=0; i<sNumEffects; i++)
      delete mEffects[i];

   mEffects.Clear();
}

int Effect::GetNumEffects()
{
   return sNumEffects;
}

Effect *Effect::GetEffect(int ID)
{
   for(int i=0; i<sNumEffects; i++)
      if (mEffects[i]->mID == ID)
         return mEffects[i];
   
   return NULL;
}

EffectArray *Effect::GetEffects(int flags /* = ALL_EFFECTS */)
{
   EffectArray *results = new EffectArray();

   int len = mEffects.GetCount();
   for(int i=0; i<len; i++) {
      int g = mEffects[i]->GetEffectFlags();
      if ((flags & g) == g)
         results->Add(mEffects[i]);
   }

   return results;
}

//
// public methods
//

Effect::Effect()
{
   mWaveTracks = NULL;
   // Can change effect flags later (this is the new way)
   // OR using the old way, over-ride GetEffectFlags().
   mFlags = BUILTIN_EFFECT | PROCESS_EFFECT | ADVANCED_EFFECT ;
}

bool Effect::DoEffect(wxWindow *parent, int flags,
                      double projectRate,
                      TrackList *list,
                      TrackFactory *factory,
                      double *t0, double *t1)
{
   wxASSERT(*t0 <= *t1);

   if (mWaveTracks) {
      delete mWaveTracks;
      mWaveTracks = NULL;
   }

   mFactory = factory;
   mProjectRate = projectRate;
   mParent = parent;
   mTracks = list;
   mT0 = *t0;
   mT1 = *t1;
   CountWaveTracks();

   if (!Init())
      return false;

   // Don't prompt user if we are dealing with a 
   // effect that is already configured, e.g. repeating
   // the last effect on a different selection.
   if( (flags & CONFIGURED_EFFECT) == 0)
   {
      if (!PromptUser())
         return false;
   }

   bool returnVal = true;
   bool skipFlag = CheckWhetherSkipEffect();
   if (skipFlag == false) {
      GetActiveProject()->ProgressShow(GetEffectName(), GetEffectAction());
      returnVal = Process();
      GetActiveProject()->ProgressHide();
   }

   End();

   delete mWaveTracks;
   mWaveTracks = NULL;

   if (returnVal) {
      *t0 = mT0;
      *t1 = mT1;
   }
   
   return returnVal;
}

bool Effect::TotalProgress(double frac)
{
   return !GetActiveProject()->ProgressUpdate((int)(frac*1000 + 0.5));
}

bool Effect::TrackProgress(int whichTrack, double frac)
{
   return TotalProgress((whichTrack+frac)/mNumTracks);
}

bool Effect::TrackGroupProgress(int whichGroup, double frac)
{
   return TotalProgress((whichGroup+frac)/mNumGroups);
}

void Effect::CountWaveTracks()
{
   mNumTracks = 0;
   mNumGroups = 0;
   mWaveTracks = new TrackList();
   
   TrackListIterator iter(mTracks);
   Track *t = iter.First();
   
   while(t) {
      if (!t->GetSelected()) {
         t = iter.Next();
         continue;
      }
      
      if (t->GetKind() == Track::Wave) {
         mWaveTracks->Add(t);
         mNumTracks++;
         if (!t->GetLinked())
            mNumGroups++;
      }
      t = iter.Next();
   }
}

float TrapFloat(float x, float min, float max)
{
   if (x <= min)
      return min;
   else if (x >= max)
      return max;
   else
      return x;
}

double TrapDouble(double x, double min, double max)
{
   if (x <= min)
      return min;
   else if (x >= max)
      return max;
   else
      return x;
}

long TrapLong(long x, long min, long max)
{
   if (x <= min)
      return min;
   else if (x >= max)
      return max;
   else
      return x;
}

wxString Effect::GetPreviewName()
{
   return _("Pre&view");
}

void Effect::Preview()
{
   if (gAudioIO->IsBusy())
      return;

   // Mix the first 3 seconds of audio from all of the tracks
   double previewLen = 3.0;
   gPrefs->Read(wxT("/AudioIO/EffectsPreviewLen"), &previewLen);
   
   WaveTrack *mixLeft = NULL;
   WaveTrack *mixRight = NULL;
   double rate = mProjectRate;
   double t0 = mT0;
   double t1 = t0 + previewLen;

   if (t1 > mT1)
      t1 = mT1;

   if (t1 <= t0)
      return;

   if (!::MixAndRender(mWaveTracks, mFactory, rate, floatSample, t0, t1,
                       &mixLeft, &mixRight))
      return;

   // Apply effect

   TrackList *saveWaveTracks = mWaveTracks;
   mWaveTracks = new TrackList();
   mWaveTracks->Add(mixLeft);
   if (mixRight)
      mWaveTracks->Add(mixRight);

   t0 = 0.0;
   t1 = mixLeft->GetEndTime();

   double t0save = mT0;
   double t1save = mT1;
   mT0 = t0;
   mT1 = t1;

   // Effect is already inited; we call Process, End, and then Init
   // again, so the state is exactly the way it was before Preview
   // was called.
   Process();
   End();
   Init();

   mT0 = t0save;
   mT1 = t1save;

   WaveTrackArray playbackTracks;
   WaveTrackArray recordingTracks;
   playbackTracks.Add(mixLeft);
   if (mixRight)
      playbackTracks.Add(mixRight);

   // Start audio playing
   
   int token =
      gAudioIO->StartStream(playbackTracks, recordingTracks, NULL,
                            rate, t0, t1, NULL);

   if (token) {
      wxBusyCursor busy;
      ::wxMilliSleep((int)((t1-t0)*1000));

      while(gAudioIO->IsStreamActive(token)) {
         ::wxMilliSleep(100);
      }
      gAudioIO->StopStream();

      while(gAudioIO->IsBusy()) {
         ::wxMilliSleep(100);
      }
   }
   else {
      wxMessageBox(_("Error while opening sound device. Please check the output device settings and the project sample rate."),
                   _("Error"), wxOK | wxICON_EXCLAMATION, mParent);
   }

   delete mWaveTracks;
   delete mixLeft;
   if (mixRight)
      delete mixRight;

   mWaveTracks = saveWaveTracks;
}

EffectDialog::EffectDialog(wxWindow * parent,
                           const wxString & title,
                           int type,
                           int flags)
: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, flags)
{
   mType = type;
}

void EffectDialog::Init()
{
   ShuttleGui S(this, eIsCreating);
   
   S.SetBorder(5);
   S.StartVerticalLay(true);
   {
      PopulateOrExchange(S);

      S.SetBorder(5);
      S.StartHorizontalLay(wxALIGN_BOTTOM | wxALIGN_CENTER, false);
      {
         if (mType == PROCESS_EFFECT)
         {
            S.StartHorizontalLay(wxALIGN_LEFT, false);
            {
               S.Id(ID_EFFECT_PREVIEW).AddButton(_("Pre&view"));
            }
            S.EndHorizontalLay();

            S.SetBorder(10);
            S.StartHorizontalLay(wxALIGN_LEFT, false);
            {
               // Everyone needs a little space sometime
            }
            S.EndHorizontalLay();
            S.SetBorder(5);
         }

         S.StartHorizontalLay(wxALIGN_CENTER, false);
         {
#if defined(__WXGTK20__) || defined(__WXMAC__)
            S.Id(wxID_CANCEL)
             .AddButton(_("&Cancel"))
             ->Show( mType == PROCESS_EFFECT || mType == INSERT_EFFECT);
            S.Id(wxID_OK).AddButton(_("&OK"))->SetDefault();
#else
            S.Id(wxID_OK).AddButton(_("&OK"))->SetDefault();
            S.Id(wxID_CANCEL)
             .AddButton(_("&Cancel"))
             ->Show( mType == PROCESS_EFFECT || mType == INSERT_EFFECT);
#endif
         }
      }
      S.EndHorizontalLay();
   }
   S.EndVerticalLay();
   GetSizer()->AddSpacer(5);
   Layout();
   Fit();
   SetMinSize(GetSize());
   Center();
}

/// This is a virtual function which will be overridden to
/// provide the actual parameters that we want for each
/// kind of dialog.
void EffectDialog::PopulateOrExchange(ShuttleGui & S)
{
   return;
}

bool EffectDialog::TransferDataToWindow()
{
   return true;
}

bool EffectDialog::TransferDataFromWindow()
{
   return true;
}

bool EffectDialog::Validate()
{
   return true;
}

void EffectDialog::OnPreview(wxCommandEvent & event)
{
   return;
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
// arch-tag: 113bebd2-dbcf-4fc5-b3b4-b52d6ac4efb2

