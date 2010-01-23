/**********************************************************************

  Audacity: A Digital Audio Editor

  AudioIO.cpp

  Dominic Mazzoni

**********************************************************************/

#ifdef __WXMAC__
#include <Events.h>
#endif

#include <wx/textdlg.h>
#include <wx/msgdlg.h>

#include "AudioIO.h"
#include "Project.h"
#include "Track.h"
#include "WaveTrack.h"
#include "Mix.h"
#include "APalette.h"
#include "Prefs.h"

AudioIO *gAudioIO;

#ifdef __WXGTK__
bool gLinuxFirstTime = true;
char gLinuxDevice[256] = "/dev/dsp";
#endif

#ifdef BOUNCE
#include "Bounce.h"
Bounce *gBounce = NULL;
#endif

void InitAudioIO()
{
   gAudioIO = new AudioIO();

}

AudioIO::AudioIO()
{
   mProject = NULL;
   mTracks = NULL;
   mStop = false;
   mHardStop = false;
   mRecordLeft = NULL;
   mRecordRight = NULL;
   // Run our timer function once every 100 ms, i.e. 10 times/sec
   mTimer.Start(100, FALSE);
}

AudioIO::~AudioIO()
{
}

bool AudioIO::OpenPlaybackDevice(AudacityProject * project)
{
   mPlayNode.device = SND_DEVICE_AUDIO;
   mPlayNode.write_flag = SND_WRITE;
   mPlayNode.format.channels = 2;
   mPlayNode.format.mode = SND_MODE_PCM;
   mPlayNode.format.bits = 16;
   mPlayNode.format.srate = project->GetRate();
   wxString deviceStr = gPrefs->Read("/AudioIO/PlaybackDevice", "");
#ifdef __WXGTK__
   if (deviceStr == "")
      deviceStr = "/dev/dsp";
#endif
   strcpy(mPlayNode.u.audio.devicename, deviceStr.c_str());
   strcpy(mPlayNode.u.audio.interfacename, "");
   mPlayNode.u.audio.descriptor = 0;
   mPlayNode.u.audio.protocol = SND_COMPUTEAHEAD;
   if (mDuplex)
      mPlayNode.u.audio.latency = 3.0;
   else
      mPlayNode.u.audio.latency = 1.0;
   mPlayNode.u.audio.granularity = 0.0;

   long flags = 0;
   int err = snd_open(&mPlayNode, &flags);

   if (err)
      return false;

   return true;
}

bool AudioIO::StartPlay(AudacityProject * project,
                        TrackList * tracks, double t0, double t1)
{
   if (mProject)
      return false;

   mRecording = false;
   mTracks = tracks;
   mT0 = t0;
   mT1 = t1;
   mT = mT0;

   if (!OpenPlaybackDevice(project)) {
      wxMessageBox("Error opening audio device.\n"
                   "(Change the device in the Preferences dialog.)");

      return false;
   }

   mStopWatch.Start(0);

#ifdef __WXMAC__
   mStartTicks = TickCount();
#endif

   mTicks = 0;

   // Do this last because this is what signals the timer to go
   mProject = project;

#ifdef BOUNCE
   gBounce = new Bounce(project, 0, "Bounce", wxPoint(150, 150));
   gBounce->SetProject(project);
#endif

   return true;
}

void AudioIO::Finish()
{
   if (!mHardStop) {
      mProject->GetAPalette()->SetPlay(false);
      mProject->GetAPalette()->SetStop(false);
      mProject->GetAPalette()->SetRecord(false);
   }

   mStop = false;
   mHardStop = false;

   if (!mRecording || (mRecording && mDuplex))
      snd_close(&mPlayNode);

   if (mRecording) {
      snd_close(&mRecordNode);
      if (!mHardStop)
         mProject->TP_PushState();
   }
   // TODO mProject->SoundDone();
   mProject = NULL;
   mRecordLeft = NULL;
   mRecordRight = NULL;

#ifdef BOUNCE
   if (gBounce) {
      delete gBounce;
      gBounce = NULL;
   }
#endif
}

bool AudioIO::StartRecord(AudacityProject * project, TrackList * tracks)
{
   if (mProject)
      return false;

   gPrefs->Read("/AudioIO/RecordStereo", &mRecordStereo, false);
   gPrefs->Read("/AudioIO/Duplex", &mDuplex, false);

   mRecordNode.device = SND_DEVICE_AUDIO;
   mRecordNode.write_flag = SND_READ;
   mRecordNode.format.channels = mRecordStereo ? 2 : 1;
   mRecordNode.format.mode = SND_MODE_PCM;
   mRecordNode.format.bits = 16;
   mRecordNode.format.srate = project->GetRate();
   wxString deviceStr = gPrefs->Read("/AudioIO/RecordingDevice", "");
#ifdef __WXGTK__
   if (deviceStr == "")
      deviceStr = "/dev/dsp";
#endif
   strcpy(mRecordNode.u.audio.devicename, deviceStr.c_str());
   strcpy(mRecordNode.u.audio.interfacename, "");
   mRecordNode.u.audio.descriptor = 0;
   mRecordNode.u.audio.protocol = SND_COMPUTEAHEAD;
   if (mDuplex)
      mRecordNode.u.audio.latency = 6.0;
   else {
#ifdef __WXMAC__
      mRecordNode.u.audio.latency = 1.0;
#else
      mRecordNode.u.audio.latency = 0.25;
#endif
   }
   mRecordNode.u.audio.granularity = mRecordNode.u.audio.latency;

   long flags = 0;
   int err = snd_open(&mRecordNode, &flags);

   if (err) {
      wxMessageBox("Error opening audio device.\n"
                   "(Change the device in the Preferences dialog.)");

      return false;
   }

   if (mDuplex) {
      if (!OpenPlaybackDevice(project)) {
         wxMessageBox("Error opening audio device.\n"
                      "Perhaps your sound card does not support\n"
                      "simultaneous playing and recording.\n"
                      "(Change the settings in the Preferences dialog.)");

         snd_close(&mRecordNode);

         return false;
      }
   }

   mRecordLeft = new WaveTrack(project->GetDirManager());
   mRecordLeft->rate = project->GetRate();
   mRecordLeft->selected = true;
   mRecordLeft->channel = (mRecordStereo ?
                           VTrack::LeftChannel : VTrack::MonoChannel);

   if (mRecordStereo) {
      mRecordLeft->linked = true;
      mRecordRight = new WaveTrack(project->GetDirManager());
      mRecordRight->rate = project->GetRate();
      mRecordRight->selected = true;
      mRecordRight->channel = VTrack::RightChannel;
   }

   project->SelectNone();

   tracks->Add(mRecordLeft);
   if (mRecordStereo)
      tracks->Add(mRecordRight);

   mRecording = true;
   mTracks = tracks;
   mT0 = 0.0;
   mT1 = 0.0;
   mT = 0.0;
   mRecT = 0.0;


   mStopWatch.Start(0);

#ifdef __WXMAC__
   mStartTicks = TickCount();
#endif

   mTicks = 0;

   // Do this last because this is what signals the timer to go
   mProject = project;

   mProject->RedrawProject();

   return true;
}

void AudioIO::OnTimer()
{
   if (!mProject)
      return;

   if (mRecording) {
      int block = snd_poll(&mRecordNode);

#ifdef __WXMAC__
      if (block > 22050) {
#else
      if (block > 0) {
#endif

         sampleType *in = new sampleType[block * 2];

         snd_read(&mRecordNode, in, block);

         if (mRecordStereo) {
            sampleType *left = new sampleType[block];
            sampleType *right = new sampleType[block];
            for (int i = 0; i < block; i++) {
               left[i] = in[2 * i];
               right[i] = in[2 * i + 1];
            }

            mRecordLeft->Append(left, block);
            mRecordRight->Append(right, block);
            delete[]left;
            delete[]right;
         } else {
            mRecordLeft->Append(in, block);
         }

         mProject->RedrawProject();

         delete[]in;
      }

      if (mStop) {
         Finish();
         return;
      }

      if (!mDuplex)
         return;
   }

   if (mStop) {
      Finish();
      return;
   }

   if (!mRecording && mT >= mT1) {
      if (GetIndicator() >= mT1) {
#ifdef BOUNCE
         if (gBounce) {
            delete gBounce;
            gBounce = NULL;
         }
#endif
         if (snd_flush(&mPlayNode) == SND_SUCCESS) {
            Finish();
            return;
         }
      } else
         return;
   }

   double deltat = mPlayNode.u.audio.latency;
   if (!mRecording && mT + deltat > mT1)
      deltat = mT1 - mT;

   int maxFrames = int (mProject->GetRate() * deltat);
   int block = snd_poll(&mPlayNode);

   if (block == 0)
      return;

   if (block > maxFrames)
      block = maxFrames;

   if (block < maxFrames) {
      deltat = block / mProject->GetRate();
   }

   Mixer *mixer = new Mixer(2, block, true);
   mixer->UseVolumeSlider(mProject->GetAPalette());
   mixer->Clear();

   TrackListIterator iter2(mTracks);
   int numSolo = 0;
   VTrack *vt = iter2.First();
   while (vt) {
      if (vt->GetKind() == VTrack::Wave && vt->solo)
         numSolo++;
      vt = iter2.Next();
   }

   TrackListIterator iter(mTracks);

   vt = iter.First();
   while (vt) {
      if (vt->GetKind() == VTrack::Wave) {
      
         VTrack *mt = vt;
      
         // We want to extract mute and solo information from
         // the top of the two tracks if they're linked
         // (i.e. a stereo pair only has one set of mute/solo buttons)
         VTrack *partner = mTracks->GetLink(vt);
         if (partner && !vt->linked)
            mt = partner;
         else
            mt = vt;

         // Cut if somebody else is soloing
         if (numSolo>0 && !mt->solo) {
            vt = iter.Next();
            continue;
         }
         
         // Cut if we're muted (unless we're soloing)
         if (mt->mute && !mt->solo) {
            vt = iter.Next();
            continue;
         }

         WaveTrack *t = (WaveTrack *) vt;
         
         switch (t->channel) {
         case VTrack::LeftChannel:
            mixer->MixLeft(t, mT, mT + deltat);
            break;
         case VTrack::RightChannel:
            mixer->MixRight(t, mT, mT + deltat);
            break;
         case VTrack::MonoChannel:
            mixer->MixMono(t, mT, mT + deltat);
            break;
         }
      }

      vt = iter.Next();
   }

   sampleType *outbytes = mixer->GetBuffer();
   snd_write(&mPlayNode, outbytes, block);

#ifndef __WXMAC__
   if (mT + deltat >= mT1)
      snd_flush(&mPlayNode);
#endif

   delete mixer;

   mT += deltat;
}

void AudioIO::Stop()
{
   mStop = true;
}

void AudioIO::HardStop()
{
   mProject = NULL;
   mStop = true;
   mHardStop = true;
   Finish();
}

bool AudioIO::IsBusy()
{
   return (mProject != NULL);
}

bool AudioIO::IsPlaying()
{
   if (mProject != NULL) {
      if (!mRecording || (mRecording && mDuplex))
         return true;
   }

   return false;
}

bool AudioIO::IsRecording()
{
   return (mProject != NULL && mRecording);
}

void AudioIOTimer::Notify()
{
   gAudioIO->OnTimer();
}

AudacityProject *AudioIO::GetProject()
{
   return mProject;
}

double AudioIO::GetIndicator()
{
   double i;

#ifdef __WXMAC__
   i = mT0 + ((TickCount() - mStartTicks) / 60.0);
#else
   i = mT0 + (mStopWatch.Time() / 1000.0);
#endif

   if (!mRecording && i > mT1)
      i = mT1;

   return i;
}
