/**********************************************************************

  Audacity: A Digital Audio Editor

  AudioIO.h

  Dominic Mazzoni

  Use the PortAudio library to play and record sound

**********************************************************************/

#ifndef __AUDACITY_AUDIO_IO__
#define __AUDACITY_AUDIO_IO__

#include "portaudio.h"

#include "Audacity.h"

#if USE_PORTMIXER
#include "portmixer.h"
#endif

#include <wx/string.h>
#include <wx/thread.h>

#include "WaveTrack.h"
#include "SampleFormat.h"

class AudioIO;
class RingBuffer;
class Mixer;
class Resample;
class TimeTrack;
class AudioThread;
class Meter;
class TimeTrack;

extern AudioIO *gAudioIO;

void InitAudioIO();
void DeinitAudioIO();
wxString DeviceName(const PaDeviceInfo* info);

class AUDACITY_DLL_API AudioIOListener {
public:
   AudioIOListener() {}
   virtual ~AudioIOListener() {}
   
   virtual void OnAudioIORate(int rate) = 0;
   virtual void OnAudioIOStartRecording() = 0;
   virtual void OnAudioIOStopRecording() = 0;
   virtual void OnAudioIONewBlockFiles(const wxString& blockFileLog) = 0;
};

class AudioIO {

 public:
   AudioIO();
   ~AudioIO();
   
   /** \brief Start up Portaudio for capture and recording as needed for
    * input monitoring and software playthrough only
    *
    * This uses the Default project sample format, current sample rate, and 
    * selected number of input channels to open the recording device and start
    * reading input data. If software playthrough is enabled, it also opens
    * the output device in stereo to play the data through */
   void StartMonitoring(double sampleRate);

   /** \brief Start recording or playing back audio
    *
    * Allocates buffers for recording and playback, gets the Audio thread to
    * fill them, and sets the stream rolling.
    * If successful, returns a token identifying this particular stream
    * instance.  For use with IsStreamActive() below */
   int StartStream(WaveTrackArray playbackTracks, WaveTrackArray captureTracks,
                   TimeTrack *timeTrack, double sampleRate,
                   double t0, double t1,
                   AudioIOListener* listener,
                   bool playLooped = false,
                   double cutPreviewGapStart = 0.0,
                   double cutPreviewGapLen = 0.0);

   /** \brief Stop recording, playback or input monitoring.
    *
    * Does quite a bit of housekeeping, including switching off monitoring,
    * flushing recording buffers out to wave tracks, and applies latency
    * correction to recorded tracks if necessary */
   void StopStream();
   /** \brief Move the playback / recording position of the current stream
    * by the specified amount from where it is now */
   void SeekStream(double seconds) { mSeek = seconds; };
   
   /** \brief  Returns true if audio i/o is busy starting, stopping, playing,
    * or recording.
    *
    * When this is false, it's safe to start playing or recording */
   bool IsBusy();

   /** \brief Returns true if the audio i/o is running at all, but not during
    * cleanup
    *
    * Doesn't return true if the device has been closed but some disk i/o or
    * cleanup is still going on. If you want to know if it's safe to start a
    * new stream, use IsBusy() */
   bool IsStreamActive();
   bool IsStreamActive(int token);

   /** \brief Returns true if the stream is active, or even if audio I/O is
    * busy cleaning up its data or writing to disk.
    *
    * This is used by TrackPanel to determine when a track has been completely
    * recorded, and it's safe to flush to disk. */
   bool IsAudioTokenActive(int token);

   /** \brief Returns true if we're monitoring input (but not recording or 
    * playing actual audio) */
   bool IsMonitoring();

   /** \brief Pause and un-pause playback and recording */
   void SetPaused(bool state);
   /** \brief Find out if playback / recording is currently paused */
   bool IsPaused();

   /* Mixer services are always available.  If no stream is running, these
    * methods use whatever device is specified by the preferences.  If a
    * stream *is* running, naturally they manipulate the mixer associated
    * with that stream.  If no mixer is available, they are emulated
    * (a gain is applied to input and output samples).
    */
   void SetMixer(int inputSource, float inputVolume,
                 float playbackVolume);
   void GetMixer(int *inputSource, float *inputVolume,
                 float *playbackVolume);
   wxArrayString GetInputSourceNames();
   void HandleDeviceChange();

   /** \brief Set the current VU meters - this should be done once after 
    * each call to StartStream currently */
   void SetMeters(Meter *inputMeter, Meter *outputMeter);
   
   /** \brief Get a list of sample rates the current output (playback) device
    * supports.
    *
    * If no information about available sample rates can be fetched,
    * an empty list is returned.
    *
    * You can explicitely give the name of the device.  If you don't
    * give it, the currently selected device from the preferences will be used.
    *
    * You may also specify a rate for which to check in addition to the
    * standard rates.
    */
   static wxArrayLong GetSupportedPlaybackRates(wxString devName = wxT(""),
                                                double rate = 0.0);

   /** \brief Get a list of sample rates the current input (recording) device
    * supports.
    *
    * If no information about available sample rates can be fetched,
    * an empty list is returned.
    *
    * You can explicitely give the name of the device.  If you don't
    * give it, the currently selected device from the preferences will be used.
    *
    * You may also specify a rate for which to check in addition to the
    * standard rates.
    */
   static wxArrayLong GetSupportedCaptureRates(wxString devName = wxT(""),
                                               double rate = 0.0);

   /** \brief Get a list of sample rates the current input/output device
    * combination supports.
    *
    * Since there is no concept (yet) for different input/output
    * sample rates, this currently returns only sample rates that are
    * supported on both the output and input device. If no information
    * about available sample rates can be fetched, it returns a default
    * list.
    * You can explicitely give the names of the playDevice/recDevice.
    * If you don't give them, the selected devices from the preferences
    * will be used.
    * You may also specify a rate for which to check in addition to the
    * standard rates.
    */
   static wxArrayLong GetSupportedSampleRates(wxString playDevice = wxT(""),
                                              wxString recDevice = wxT(""),
                                              double rate = 0.0);

   /** \brief Get a supported sample rate which can be used a an optimal
    * default.
    *
    * Currently, this uses the first supported rate in the list
    * [44100, 48000, highest sample rate].
    */
   static int GetOptimalSupportedSampleRate();

   /** \brief The time the stream has been playing for
    *
    * This is given in seconds based on starting at t0
    * When playing looped, this will start from t0 again,
    * too. So the returned time should be always between
    * t0 and t1
    */
   double GetStreamTime();

   sampleFormat GetCaptureFormat() { return mCaptureFormat; }
   int GetNumCaptureChannels() { return mNumCaptureChannels; }

   /** \brief Array of common audio sample rates
    *
    * These are the rates we will always support, regardless of hardware support
    * for them (by resampling in audacity if needed) */
   static const int StandardRates[];
   /** \brief How many standard sample rates there are */
   static const int NumStandardRates;

   /** \brief Get diagnostic information on all the available audio I/O devices
    *
    */
   wxString GetDeviceInfo();

private:
   long GetBestRate(bool capturing, double sampleRate);

   bool StartPortAudioStream(double sampleRate,
                             unsigned int numPlaybackChannels,
                             unsigned int numCaptureChannels,
                             sampleFormat captureFormat);

   void FillBuffers();

   int GetCommonlyAvailPlayback();
   int GetCommonlyAvailCapture();

   double NormalizeStreamTime(double absoluteTime) const;

   AudioThread        *mThread;
   Resample          **mResample;
   RingBuffer        **mCaptureBuffers;
   WaveTrackArray      mCaptureTracks;
   RingBuffer        **mPlaybackBuffers;
   WaveTrackArray      mPlaybackTracks;
   Mixer             **mPlaybackMixers;
   int                 mStreamToken;
   int                 mStopStreamCount;
   static int          mNextStreamToken;
   double              mFactor;
   double              mRate;
   double              mT;
   double              mT0;
   double              mT1;
   double              mTime;
   double              mWarpedT1;
   double              mSeek;
   double              mPlaybackRingBufferSecs;
   double              mCaptureRingBufferSecs;
   double              mMaxPlaybackSecsToCopy;
   double              mMinCaptureSecsToCopy;
   bool                mPaused;
#if USE_PORTAUDIO_V19
   PaStream           *mPortStreamV19;
#else
   PortAudioStream    *mPortStreamV18;
   volatile bool       mInCallbackFinishedState;
#endif
   bool                mSoftwarePlaythrough;
   bool                mPauseRec;
   float               mSilenceLevel;
   unsigned int        mNumCaptureChannels;
   unsigned int        mNumPlaybackChannels;
   sampleFormat        mCaptureFormat;
   float              *mTempFloats;
   int                 mLostSamples;
   volatile bool       mAudioThreadShouldCallFillBuffersOnce;
   volatile bool       mAudioThreadFillBuffersLoopRunning;
   volatile bool       mAudioThreadFillBuffersLoopActive;
   volatile double     mLastRecordingOffset;
   PaError             mLastPaError;

   Meter              *mInputMeter;
   Meter              *mOutputMeter;
   bool                mUpdateMeters;
   bool                mUpdatingMeters;

   #if USE_PORTMIXER
   PxMixer            *mPortMixer;
   float               mPreviousHWPlaythrough;
   #endif

   bool                mEmulateMixerOutputVol;
   bool                mEmulateMixerInputVol;
   float               mMixerOutputVol;
   float               mMixerInputVol;

   bool                mPlayLooped;
   double              mCutPreviewGapStart;
   double              mCutPreviewGapLen;
   
   AudioIOListener*    mListener;

   friend class AudioThread;

   friend void InitAudioIO();
   friend void DeinitAudioIO();

   TimeTrack *mTimeTrack;
   
#if USE_PORTAUDIO_V19
   friend int audacityAudioCallback(
                const void *inputBuffer, void *outputBuffer,
                unsigned long framesPerBuffer,
                const PaStreamCallbackTimeInfo *timeInfo,
                PaStreamCallbackFlags statusFlags, void *userData );
#else
   friend int audacityAudioCallback(
                void *inputBuffer, void *outputBuffer,
                unsigned long framesPerBuffer,
                PaTimestamp outTime, void *userData );
#endif

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
// arch-tag: 5b5316f5-6078-469b-950c-9da893cd62c9

