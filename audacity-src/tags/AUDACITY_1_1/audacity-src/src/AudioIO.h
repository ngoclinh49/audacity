/**********************************************************************

  Audacity: A Digital Audio Editor

  AudioIO.h

  Dominic Mazzoni

  Use the PortAudio library to play and record sound

**********************************************************************/

#ifndef __AUDACITY_AUDIO_IO__
#define __AUDACITY_AUDIO_IO__

#include "portaudio.h"

#include <wx/string.h>
#include <wx/timer.h>

#include "WaveTrack.h"

class AudioIO;
class AudacityProject;

extern AudioIO *gAudioIO;

void InitAudioIO();

class AudioIOTimer:public wxTimer {
 public:
   virtual void Notify();
};

struct AudioIOBuffer {
   int          ID;
   int          len;
   sampleType  *data;
};

class AudioIO {

 public:
   AudioIO();
   ~AudioIO();

   bool StartPlay(AudacityProject * project,
                  TrackList * tracks, double t0, double t1);

   bool StartRecord(AudacityProject * project,
                    TrackList * tracks, double t0, double t1);

   void Stop();
   void HardStop();
   bool IsBusy();
   bool IsPlaying();
   bool IsRecording();
   double GetIndicator();

   AudacityProject *GetProject();

   void OnTimer();

 private:

   bool Start();

   bool OpenDevice();
   void FillBuffers();   

   AudacityProject     *mProject;
   TrackList           *mTracks;
   double              mRate;
   double              mT;
   double              mRecT;
   double              mT0;
   double              mT1;
   bool                mHardStop;
   
   PortAudioStream    *mPortStream;

   int                 mNumInChannels;
   int                 mNumOutChannels;
   
   WaveTrack         **mInTracks;

   AudioIOTimer        mTimer;
   
   sampleCount         mBufferSize;
   int                 mInID;
   int                 mOutID;
   unsigned int        mMaxBuffers;
   unsigned int        mInitialNumOutBuffers;
   unsigned int        mNumOutBuffers;
   unsigned int        mNumInBuffers;
   AudioIOBuffer      *mOutBuffer;
   AudioIOBuffer      *mInBuffer;
   
   int                 mInUnderruns;
   int                 mRepeats;
   int                 mLastChecksum;
   double              mRepeatPoint;
   
   friend int audacityAudioCallback(
		void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer,
		PaTimestamp outTime, void *userData );
};

#endif
