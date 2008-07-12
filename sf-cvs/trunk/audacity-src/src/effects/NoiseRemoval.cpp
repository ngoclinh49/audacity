/**********************************************************************

  Audacity: A Digital Audio Editor

  NoiseRemoval.cpp

  Dominic Mazzoni

*******************************************************************//**

\class EffectNoiseRemoval
\brief A two-pass effect to remove background noise.

  The first pass is done over just noise.  For each windowed sample
  of the sound, we take a FFT and then statistics are tabulated for
  each frequency band - specifically the maximum level achieved by
  at least (n) sampling windows in a row, for various values of (n).

  During the noise removal phase, we start by setting a gain control
  for each frequency band such that if the sound has exceeded the
  previously-determined threshold, the gain is set to 0, otherwise
  the gain is set lower (e.g. -18 dB), to suppress the noise.
  Then frequency-smoothing is applied so that a single frequency is
  never suppressed or boosted in isolation, and then time-smoothing
  is applied so that the gain for each frequency band moves slowly.
  Lookahead is employed; this effect is not designed for real-time
  but if it were, there would be a significant delay.

  The gain controls are applied to the complex FFT of the signal,
  and then the inverse FFT is applied, followed by a Hanning window;
  the output signal is then pieced together using overlap/add of
  half the window size.

*//****************************************************************//**

\class NoiseRemovalDialog
\brief Dialog used with EffectNoiseRemoval

*//*******************************************************************/

#include "../Audacity.h"

#include "NoiseRemoval.h"

#include "../Envelope.h"
#include "../FFT.h"
#include "../WaveTrack.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../FileNames.h"

#include <math.h>

#if defined(__WXMSW__) && !defined(__CYGWIN__)
#include <float.h>
#define finite(x) _finite(x)
#endif

#include <wx/file.h>
#include <wx/ffile.h>
#include <wx/bitmap.h>
#include <wx/brush.h>
#include <wx/button.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "../AudacityApp.h"
#include "../PlatformCompatibility.h"

EffectNoiseRemoval::EffectNoiseRemoval()
{
   mWindowSize = 2048;
   mSpectrumSize = 1 + mWindowSize / 2;

   gPrefs->Read(wxT("/CsPresets/NoiseGain"),
                &mNoiseGain, -24.0);
   gPrefs->Read(wxT("/CsPresets/NoiseFreqSmoothing"),
                &mFreqSmoothingHz, 150.0);
   gPrefs->Read(wxT("/CsPresets/NoiseAttackDecayTime"),
                &mAttackDecayTime, 0.15);

   mMinSignalTime = 0.05f;
   mHasProfile = false;
   mDoProfile = true;

   mNoiseThreshold = new float[mSpectrumSize];

   // This sequence is safe, even if not in CleanSpeechMode
   wxGetApp().SetCleanSpeechNoiseGate(mNoiseThreshold);
   wxGetApp().SetCleanSpeechNoiseGateExpectedCount(
      mSpectrumSize * sizeof(float));
   CleanSpeechMayReadNoisegate();

   Init();
}

EffectNoiseRemoval::~EffectNoiseRemoval()
{
   delete [] mNoiseThreshold;
}

void EffectNoiseRemoval::CleanSpeechMayReadNoisegate()
{
   int halfWindowSize = mWindowSize / 2;

   //lda-131a always try to get noisegate.nrp if in CleanSpeechMode
   // and it exists
   AudacityProject * project = GetActiveProject();
   if (project == NULL) {
      int mode = gPrefs->Read(wxT("/Batch/CleanSpeechMode"), 0L);
      if (mode == 0) {
         return;
      }
   }

   // Try to open the file.
   if( !wxDirExists( FileNames::NRPDir() ))
      return;

   // if file doesn't exist, return quietly.
   wxString fileName = FileNames::NRPFile();
   if( !wxFileExists( fileName ))
      return;

   wxFFile noiseGateFile(fileName, wxT("rb"));
   bool flag = noiseGateFile.IsOpened();
   if (flag != true)
      return;

   // Now get its data.
   int expectedCount = halfWindowSize * sizeof(float);
   int count = noiseGateFile.Read(mNoiseThreshold, expectedCount);
   noiseGateFile.Close();
   if (count == expectedCount) {
      for (int i = halfWindowSize; i < mSpectrumSize; ++i) {
         mNoiseThreshold[i] = float(0.0);  // only partly filled by Read?
      }
      mHasProfile = true;
      mDoProfile = false;
   }
}

void EffectNoiseRemoval::CleanSpeechMayWriteNoiseGate()
{
   AudacityProject * project = GetActiveProject();
   if( !project || !project->GetCleanSpeechMode() )
      return;

   // code borrowed from ThemeBase::SaveComponents() - MJS
   // IF directory doesn't exist THEN create it
   if( !wxDirExists( FileNames::NRPDir() ))
   {
      /// \bug 1 in wxWidgets documentation; wxMkDir returns false if 
      /// directory didn't exist, even if it successfully creates it.
      /// so we create and then test if it exists instead.
      /// \bug 2 in wxWidgets documentation; wxMkDir has only one argument
      /// under MSW
#ifdef __WXMSW__
      wxMkDir( FileNames::NRPDir().fn_str() );
#else
      wxMkDir( FileNames::NRPDir().fn_str(), 0700 );
#endif
      if( !wxDirExists( FileNames::NRPDir() ))
      {
         wxMessageBox(
            wxString::Format( 
            _("Could not create directory:\n  %s"),
               FileNames::NRPDir().c_str() ));
         return;
      }
   }

   wxString fileName = FileNames::NRPFile();
   fileName = PlatformCompatibility::GetLongFileName(fileName);
   wxFFile noiseGateFile(fileName, wxT("wb"));
   bool flag = noiseGateFile.IsOpened();
   if (flag == true) {
      int expectedCount = (mWindowSize / 2) * sizeof(float);
      // FIX-ME: Should we check return value on Write?
      noiseGateFile.Write(mNoiseThreshold, expectedCount);
      noiseGateFile.Close();
   }
   else {
      wxMessageBox(
         wxString::Format( 
         _("Could not open file:\n  %s"), fileName.c_str() ));
      return;
   }
}

#define MAX_NOISE_LEVEL  30
bool EffectNoiseRemoval::Init()
{
   mLevel = gPrefs->Read(wxT("/CsPresets/Noise_Level"), 3L);
   if ((mLevel < 0) || (mLevel > MAX_NOISE_LEVEL)) {  // corrupted Prefs?
      mLevel = 0;  //Off-skip
      gPrefs->Write(wxT("/CsPresets/Noise_Level"), mLevel);
   }
   return true;
}

bool EffectNoiseRemoval::CheckWhetherSkipEffect()
{
   bool rc = (mLevel == 0);
   return rc;
}

bool EffectNoiseRemoval::PromptUser()
{
   NoiseRemovalDialog dlog(this, mParent);
   dlog.mGain = -mNoiseGain;
   dlog.mFreq = mFreqSmoothingHz;
   dlog.mTime = mAttackDecayTime;

   if( !mHasProfile )
   {
      AudacityProject * p = GetActiveProject();
      if (p->GetCleanSpeechMode())
         CleanSpeechMayReadNoisegate();
   }

   // We may want to twiddle the levels if we are setting
   // from an automation dialog, the only case in which we can
   // get here without any wavetracks.
   bool bAllowTwiddleSettings = (mWaveTracks==NULL); 

   if (mHasProfile || bAllowTwiddleSettings ) {
      dlog.m_pButton_Preview->Enable(mWaveTracks != NULL);
      dlog.m_pButton_RemoveNoise->SetDefault();
   } else {
      dlog.m_pButton_Preview->Enable(false);
      dlog.m_pButton_RemoveNoise->Enable(false);
   }

   dlog.TransferDataToWindow();
   dlog.CentreOnParent();
   dlog.ShowModal();
   
   if (dlog.GetReturnCode() == 0) {
      return false;
   }

   mNoiseGain = -dlog.mGain;
   mFreqSmoothingHz = dlog.mFreq;
   mAttackDecayTime = dlog.mTime;
   gPrefs->Write(wxT("/CsPresets/NoiseGain"), mNoiseGain);
   gPrefs->Write(wxT("/CsPresets/NoiseFreqSmoothing"), mFreqSmoothingHz);
   gPrefs->Write(wxT("/CsPresets/NoiseAttackDecayTime"), mAttackDecayTime);

   mDoProfile = (dlog.GetReturnCode() == 1);
   return true;
}
   
bool EffectNoiseRemoval::TransferParameters( Shuttle & shuttle )
{  
   //shuttle.TransferDouble(wxT("Gain"), mNoiseGain, 0.0);
   //shuttle.TransferDouble(wxT("Freq"), mFreqSmoothingHz, 0.0);
   //shuttle.TransferDouble(wxT("Time"), mAttackDecayTime, 0.0);
   return true;
}

bool EffectNoiseRemoval::Process()
{
   if (!mDoProfile && !mHasProfile)
      CleanSpeechMayReadNoisegate();
   
   // If we still don't have a profile we have a problem.
   // This should only happen in CleanSpeech.
   if(!mDoProfile && !mHasProfile) {
      wxMessageBox(
        _("Attempt to run Noise Removal without a noise profile.\n"));
      return false;
   }

   Initialize();

   // This same code will both remove noise and profile it,
   // depending on 'mDoProfile'
   this->CopyInputWaveTracks(); // Set up mOutputWaveTracks.
   bool bGoodResult = true;

   TrackListIterator iter(mOutputWaveTracks);
   WaveTrack *track = (WaveTrack *) iter.First();
   int count = 0;
   while (track) {
      double trackStart = track->GetStartTime();
      double trackEnd = track->GetEndTime();
      double t0 = mT0 < trackStart? trackStart: mT0;
      double t1 = mT1 > trackEnd? trackEnd: mT1;

      if (t1 > t0) {
         sampleCount start = track->TimeToLongSamples(t0);
         sampleCount end = track->TimeToLongSamples(t1);
         sampleCount len = (sampleCount)(end - start);

         if (!ProcessOne(count, track, start, len)) {
            Cleanup();
            bGoodResult = false;
            break;
         }
      }
      track = (WaveTrack *) iter.Next();
      count++;
   }

   if (bGoodResult && mDoProfile) {
      CleanSpeechMayWriteNoiseGate();
      mHasProfile = true;
      mDoProfile = false;
   }

   if (bGoodResult)
      Cleanup();
   this->ReplaceProcessedWaveTracks(bGoodResult); 
   return bGoodResult;
}

void EffectNoiseRemoval::ApplyFreqSmoothing(float *spec)
{
   float *tmp = new float[mSpectrumSize];
   int i, j, j0, j1;

   for(i = 0; i < mSpectrumSize; i++) {
      j0 = wxMax(0, i - mFreqSmoothingBins);
      j1 = wxMin(mSpectrumSize-1, i + mFreqSmoothingBins);
      tmp[i] = 0.0;
      for(j = j0; j <= j1; j++) {
         tmp[i] += spec[j];
      }
      tmp[i] /= (j1 - j0 + 1);
   }

   for(i = 0; i < mSpectrumSize; i++)
      spec[i] = tmp[i];

   delete[] tmp;
}

void EffectNoiseRemoval::Initialize()
{
   int i;

   mSampleRate = mProjectRate;
   mFreqSmoothingBins = (int)(mFreqSmoothingHz * mWindowSize / mSampleRate);
   mAttackDecayBlocks = 1 +
      (int)(mAttackDecayTime * mSampleRate / (mWindowSize / 2));
   mOneBlockAttackDecay = (int)(mNoiseGain / (mAttackDecayBlocks - 1));
   mMinSignalBlocks =
      (int)(mMinSignalTime * mSampleRate / (mWindowSize / 2));
   mHistoryLen = (2 * mAttackDecayBlocks) - 1;

   if (mHistoryLen < mMinSignalBlocks)
      mHistoryLen++;

   mSpectrums = new float*[mHistoryLen];
   mGains = new float*[mHistoryLen];
   mRealFFTs = new float*[mHistoryLen];
   mImagFFTs = new float*[mHistoryLen];
   for(i = 0; i < mHistoryLen; i++) {
      mSpectrums[i] = new float[mSpectrumSize];
      mGains[i] = new float[mSpectrumSize];
      mRealFFTs[i] = new float[mWindowSize];
      mImagFFTs[i] = new float[mWindowSize];
   }

   mInWaveBuffer = new float[mWindowSize];
   mOutWaveBuffer = new float[mWindowSize];
   mOutImagBuffer = new float[mWindowSize];
   mOutOverlapBuffer = new float[mWindowSize];

   if (mDoProfile) {
      for (i = 0; i < mSpectrumSize; i++)
         mNoiseThreshold[i] = -999.0;
   }
}

void EffectNoiseRemoval::Cleanup()
{
   int i;

   if (mDoProfile) {
      ApplyFreqSmoothing(mNoiseThreshold);
   }

   for(i = 0; i < mHistoryLen; i++) {
      delete[] mSpectrums[i];
      delete[] mGains[i];
      delete[] mRealFFTs[i];
      delete[] mImagFFTs[i];
   }
   delete[] mSpectrums;
   delete[] mGains;
   delete[] mRealFFTs;
   delete[] mImagFFTs;

   delete[] mInWaveBuffer;
   delete[] mOutWaveBuffer;
   delete[] mOutImagBuffer;
   delete[] mOutOverlapBuffer;
}

void EffectNoiseRemoval::StartNewTrack()
{
   int i, j;

   for(i = 0; i < mHistoryLen; i++) {
      for(j = 0; j < mSpectrumSize; j++) {
         mSpectrums[i][j] = -999.0;
         mGains[i][j] = mNoiseGain;
      }
      for(j = 0; j < mWindowSize; j++) {
         mRealFFTs[i][j] = 0.0;
         mImagFFTs[i][j] = 0.0;
      }
   }

   for(j = 0; j < mWindowSize; j++)
      mOutOverlapBuffer[j] = 0.0;

   mInputPos = 0;
   mInSampleCount = 0;
   mOutSampleCount = -(mWindowSize / 2) * (mHistoryLen - 1);
}

void EffectNoiseRemoval::ProcessSamples(sampleCount len, float *buffer)
{
   int i;

   while(len && mOutSampleCount < mInSampleCount) {
      int avail = wxMin(len, mWindowSize - mInputPos);
      for(i = 0; i < avail; i++)
         mInWaveBuffer[mInputPos + i] = buffer[i];
      buffer += avail;
      len -= avail;
      mInputPos += avail;

      if (mInputPos == mWindowSize) {
         FillFirstHistoryWindow();
         if (mDoProfile)
            GetProfile();
         else
            RemoveNoise();
         RotateHistoryWindows();

         // Rotate halfway for overlap-add
         for(i = 0; i < mWindowSize / 2; i++) {
            mInWaveBuffer[i] = mInWaveBuffer[i + mWindowSize / 2];
         }
         mInputPos = mWindowSize / 2;
      }
   }
}

void EffectNoiseRemoval::FillFirstHistoryWindow()
{
   int i;

   FFT(mWindowSize, false, mInWaveBuffer, NULL,
       mRealFFTs[0], mImagFFTs[0]);
   for(i = 0; i < mSpectrumSize; i++) {
      double power =
         mRealFFTs[0][i] * mRealFFTs[0][i] +
         mImagFFTs[0][i] * mImagFFTs[0][i];
      mSpectrums[0][i] = (float)(10.0 * log10(power));
      mGains[0][i] = mNoiseGain;
   }
}

void EffectNoiseRemoval::RotateHistoryWindows()
{
   int last = mHistoryLen - 1;
   int i;

   // Delete last window
   delete[] mSpectrums[last];
   delete[] mGains[last];
   delete[] mRealFFTs[last];
   delete[] mImagFFTs[last];   

   // Rotate each window forward
   for(i = last; i >= 1; i--) {
      mSpectrums[i] = mSpectrums[i-1];
      mGains[i] = mGains[i-1];
      mRealFFTs[i] = mRealFFTs[i-1];
      mImagFFTs[i] = mImagFFTs[i-1];
   }

   // Create new first window
   mSpectrums[0] = new float[mSpectrumSize];
   mGains[0] = new float[mSpectrumSize];
   mRealFFTs[0] = new float[mWindowSize];
   mImagFFTs[0] = new float[mWindowSize];
}

void EffectNoiseRemoval::FinishTrack()
{
   // Keep flushing empty input buffers through the history
   // windows until we've output exactly as many samples as
   // were input

   float *empty = new float[mWindowSize / 2];
   int i;
   for(i = 0; i < mWindowSize / 2; i++)
      empty[i] = 0.0;

   while (mOutSampleCount < mInSampleCount) {
      ProcessSamples(mWindowSize / 2, empty);
   }
}

void EffectNoiseRemoval::GetProfile()
{
   // The noise threshold for each frequency is the maximum
   // level achieved at that frequency for a minimum of
   // mMinSignalBlocks blocks in a row - the max of a min.

   int start = mHistoryLen - mMinSignalBlocks;
   int finish = mHistoryLen;
   int i, j;

   for (j = 0; j < mSpectrumSize; j++) {
      float min = mSpectrums[start][j];
      for (i = start+1; i < finish; i++) {
         if (mSpectrums[i][j] < min)
            min = mSpectrums[i][j];
      }
      if (min > mNoiseThreshold[j])
         mNoiseThreshold[j] = min;
   }

   mOutSampleCount += mWindowSize / 2;
}

void EffectNoiseRemoval::RemoveNoise()
{
   int center = mHistoryLen / 2;
   int start = center - mMinSignalBlocks/2;
   int finish = start + mMinSignalBlocks;
   int i, j;

   // Raise the gain for elements in the center of the sliding history
   for (j = 0; j < mSpectrumSize; j++) {
      float min = mSpectrums[start][j];
      for (i = start+1; i < finish; i++) {
         if (mSpectrums[i][j] < min)
            min = mSpectrums[i][j];
      }
      if (min > mNoiseThreshold[j] && mGains[center][j] < 0.0)
         mGains[center][j] = 0.0;
   }

   // Decay the gain in both directions;
   // note that mOneBlockAttackDecay is a negative number, like -1.0
   // dB of attenuation per block
   for (j = 0; j < mSpectrumSize; j++) {
      for (i = center + 1; i < mHistoryLen; i++) {
         if (mGains[i][j] < mGains[i - 1][j] + mOneBlockAttackDecay)
            mGains[i][j] = mGains[i - 1][j] + mOneBlockAttackDecay;
         if (mGains[i][j] < mNoiseGain)
            mGains[i][j] = mNoiseGain;
      }
      for (i = center - 1; i >= 0; i--) {
         if (mGains[i][j] < mGains[i + 1][j] + mOneBlockAttackDecay)
            mGains[i][j] = mGains[i + 1][j] + mOneBlockAttackDecay;
         if (mGains[i][j] < mNoiseGain)
            mGains[i][j] = mNoiseGain;
      }
   }

   // Apply frequency smoothing to output gain
   int out = mHistoryLen - 1;  // end of the queue

   ApplyFreqSmoothing(mGains[out]);

   // Apply gain to FFT
   for (j = 0; j < mSpectrumSize; j++) {
      float mult = pow(10.0, mGains[out][j] / 20.0);

      mRealFFTs[out][j] *= mult;
      mImagFFTs[out][j] *= mult;
      if (j > 0 && j < mSpectrumSize - 1) {
         mRealFFTs[out][mWindowSize - j] *= mult;
         mImagFFTs[out][mWindowSize - j] *= mult;
      }
   }

   // Invert the FFT into the output buffer
   FFT(mWindowSize, true, mRealFFTs[out], mImagFFTs[out],
       mOutWaveBuffer, mOutImagBuffer);

   // Hanning window
   WindowFunc(3, mWindowSize, mOutWaveBuffer);

   // Overlap-add
   for(j = 0; j < mWindowSize; j++)
      mOutOverlapBuffer[j] += mOutWaveBuffer[j];

   // Output the first half of the overlap buffer, they're done -
   // and then shift the next half over.
   if (mOutSampleCount >= 0) {
      mOutputTrack->Append((samplePtr)mOutOverlapBuffer, floatSample,
                           mWindowSize / 2);
   }
   mOutSampleCount += mWindowSize / 2;
   for(j = 0; j < mWindowSize / 2; j++) {
      mOutOverlapBuffer[j] = mOutOverlapBuffer[j + (mWindowSize / 2)];
      mOutOverlapBuffer[j + (mWindowSize / 2)] = 0.0;
   }
}

bool EffectNoiseRemoval::ProcessOne(int count, WaveTrack * track,
                                    sampleCount start, sampleCount len)
{
   if (track == NULL)
      return false;

   StartNewTrack();

   if (!mDoProfile)
      mOutputTrack = mFactory->NewWaveTrack(track->GetSampleFormat(),
                                            track->GetRate());

   sampleCount bufferSize = track->GetMaxBlockSize();
   float *buffer = new float[bufferSize];

   bool bLoopSuccess = true;
   sampleCount blockSize;
   sampleCount samplePos = start;
   while (samplePos < start + len) {
      //Get a blockSize of samples (smaller than the size of the buffer)
      blockSize = track->GetBestBlockSize(samplePos);

      //Adjust the block size if it is the final block in the track
      if (samplePos + blockSize > start + len)
         blockSize = start + len - samplePos;

      //Get the samples from the track and put them in the buffer
      track->Get((samplePtr)buffer, floatSample, samplePos, blockSize);

      mInSampleCount += blockSize;
      ProcessSamples(blockSize, buffer);

      samplePos += blockSize;

      // Update the Progress meter
      if (TrackProgress(count, (samplePos - start) / (double)len)) {
         bLoopSuccess = false;
         break;
      }
   }

   FinishTrack();
   delete [] buffer;

   if (!mDoProfile) {
      // Flush the output WaveTrack (since it's buffered)
      mOutputTrack->Flush();

      // Take the output track and insert it in place of the original
      // sample data
      if (bLoopSuccess) {
         track->Clear(mT0, mT1);
         track->Paste(mT0, mOutputTrack);
      }

      // Delete the outputTrack now that its data is inserted in place
      delete mOutputTrack;
      mOutputTrack = NULL;
   }

   return bLoopSuccess;
}

// WDR: class implementations

//----------------------------------------------------------------------------
// NoiseRemovalDialog
//----------------------------------------------------------------------------

// WDR: event table for NoiseRemovalDialog

enum {
   ID_BUTTON_GETPROFILE = 10001,
   ID_GAIN_SLIDER,
   ID_FREQ_SLIDER,
   ID_TIME_SLIDER,
   ID_GAIN_TEXT,
   ID_FREQ_TEXT,
   ID_TIME_TEXT,
};

#define GAIN_MIN 0
#define GAIN_MAX 48    // Corresponds to -48 dB

#define FREQ_MIN 0
#define FREQ_MAX 100    // Corresponds to 1000 Hz

#define TIME_MIN 0
#define TIME_MAX 100   // Corresponds to 1.00 seconds


BEGIN_EVENT_TABLE(NoiseRemovalDialog,wxDialog)
   EVT_BUTTON(wxID_OK, NoiseRemovalDialog::OnRemoveNoise)
   EVT_BUTTON(wxID_CANCEL, NoiseRemovalDialog::OnCancel)
   EVT_BUTTON(ID_EFFECT_PREVIEW, NoiseRemovalDialog::OnPreview)
   EVT_BUTTON(ID_BUTTON_GETPROFILE, NoiseRemovalDialog::OnGetProfile)
   EVT_SLIDER(ID_GAIN_SLIDER, NoiseRemovalDialog::OnGainSlider)
   EVT_SLIDER(ID_FREQ_SLIDER, NoiseRemovalDialog::OnFreqSlider)
   EVT_SLIDER(ID_TIME_SLIDER, NoiseRemovalDialog::OnTimeSlider)
   EVT_TEXT(ID_GAIN_TEXT, NoiseRemovalDialog::OnGainText)
   EVT_TEXT(ID_FREQ_TEXT, NoiseRemovalDialog::OnFreqText)
   EVT_TEXT(ID_TIME_TEXT, NoiseRemovalDialog::OnTimeText)
END_EVENT_TABLE()

NoiseRemovalDialog::NoiseRemovalDialog(EffectNoiseRemoval * effect, 
                                       wxWindow *parent) :
   EffectDialog( parent, _("Noise Removal"), PROCESS_EFFECT)
{
   m_pEffect = effect;
   
   // NULL out the control members until the controls are created.
   m_pButton_GetProfile = NULL;
   m_pButton_Preview = NULL;
   m_pButton_RemoveNoise = NULL;

   Init();

   m_pButton_Preview =
      (wxButton *)wxWindow::FindWindowById(ID_EFFECT_PREVIEW, this);
   m_pButton_RemoveNoise =
      (wxButton *)wxWindow::FindWindowById(wxID_OK, this);
}

void NoiseRemovalDialog::OnGetProfile( wxCommandEvent &event )
{
   EndModal(1);
}

void NoiseRemovalDialog::OnPreview(wxCommandEvent &event)
{
   // Save & restore parameters around Preview, because we didn't do OK.
   bool oldDoProfile = m_pEffect->mDoProfile;
   double oldGain = m_pEffect->mNoiseGain;
   double oldFreq = m_pEffect->mFreqSmoothingHz;
   double oldTime = m_pEffect->mAttackDecayTime;

   TransferDataFromWindow();

   m_pEffect->mDoProfile = false;
   m_pEffect->mNoiseGain = -mGain;
   m_pEffect->mFreqSmoothingHz =  mFreq;
   m_pEffect->mAttackDecayTime =  mTime;
   
   m_pEffect->Preview();
   
   m_pEffect->mNoiseGain = oldGain;
   m_pEffect->mFreqSmoothingHz =  oldFreq;
   m_pEffect->mAttackDecayTime =  oldTime;
   m_pEffect->mDoProfile = oldDoProfile;
}

void NoiseRemovalDialog::OnRemoveNoise( wxCommandEvent &event )
{
   EndModal(2);
}

void NoiseRemovalDialog::OnCancel(wxCommandEvent &event)
{
   EndModal(0);
}

void NoiseRemovalDialog::PopulateOrExchange(ShuttleGui & S)
{
   wxString step1Label;
   wxString step1Prompt;
   wxString step2Label;
   wxString step2Prompt;

   bool bCleanSpeechMode = false;

   AudacityProject * project = GetActiveProject();
   if( project && project->GetCleanSpeechMode() ) {
      bCleanSpeechMode = true;
   }

   if (bCleanSpeechMode) {
      // We're not marking these as translatable because most people
      // don't use CleanSpeech so it'd be a waste of time for most
      // translators
      step1Label = wxT("Preparation Step");
      step1Prompt = wxT("Listen carefully to section with some speech "
                        wxT("and some silence to check before/after.\n")
                        wxT("Select a few seconds of just noise ('thinner' ")
                        wxT("part of wave pattern usually between\nspoken ")
                        wxT("phrases or during pauses) so Audacity knows ")
                        wxT("what to filter out, then click"));
      step2Label = wxT("Actually Remove Noise");
      step2Prompt = wxT("Select what part of the audio you want filtered "
                        wxT("(Ctrl-A = All), chose how much noise\nyou want ")
                        wxT("filtered out with sliders below, and then click ")
                        wxT("'OK' to remove noise.\nFind best setting with ")
                        wxT("Ctrl-Z to Undo, Select All, and change ")
                        wxT("the slider positions."));
   }
   else {
      step1Label = _("Step 1");
      step1Prompt = _("Select a few seconds of just noise so Audacity knows what to filter out,\nthen click Get Noise Profile:");
      step2Label = _("Step 2");
      step2Prompt = _("Select all of the audio you want filtered, choose how much noise you want\nfiltered out, and then click 'OK' to remove noise.\n");
   }

   S.StartHorizontalLay(wxCENTER, false);
   {
      S.AddTitle(_("Noise Removal by Dominic Mazzoni"));
   }
   S.EndHorizontalLay();
   
   S.StartStatic(step1Label);
   {
      S.AddVariableText(step1Prompt);
      m_pButton_GetProfile = S.Id(ID_BUTTON_GETPROFILE).
         AddButton(_("Get Noise Profile"));
   }
   S.EndStatic();

   S.StartStatic(step2Label);
   {
      S.AddVariableText(step2Prompt);

      S.StartMultiColumn(3, wxEXPAND);
      S.SetStretchyCol(2);
      {
         mGainT = S.Id(ID_GAIN_TEXT).AddTextBox(_("Noise reduction (dB):"),
                                                wxT(""),
                                                0);
         S.SetStyle(wxSL_HORIZONTAL);
         mGainS = S.Id(ID_GAIN_SLIDER).AddSlider(wxT(""), 0, GAIN_MAX);
         mGainS->SetRange(GAIN_MIN, GAIN_MAX);
         mGainS->SetSizeHints(150, -1);

         mFreqT = S.Id(ID_FREQ_TEXT).AddTextBox(_("Frequency smoothing (Hz):"),
                                                wxT(""),
                                                0);
         S.SetStyle(wxSL_HORIZONTAL);
         mFreqS = S.Id(ID_FREQ_SLIDER).AddSlider(wxT(""), 0, FREQ_MAX);
         mFreqS->SetRange(FREQ_MIN, FREQ_MAX);
         mFreqS->SetSizeHints(150, -1);

         mTimeT = S.Id(ID_FREQ_TEXT).AddTextBox(_("Attack/decay time (secs):"),
                                                wxT(""),
                                                0);
         S.SetStyle(wxSL_HORIZONTAL);
         mTimeS = S.Id(ID_TIME_SLIDER).AddSlider(wxT(""), 0, TIME_MAX);
         mTimeS->SetRange(TIME_MIN, TIME_MAX);
         mTimeS->SetSizeHints(150, -1);
      }
      S.EndMultiColumn();
   }
   S.EndStatic();
}

bool NoiseRemovalDialog::TransferDataToWindow()
{
   mGainS->SetValue((int)mGain);
   mFreqS->SetValue((int)(mFreq / 10));
   mTimeS->SetValue((int)(mTime * 100));

   mGainT->SetValue(wxString::Format(wxT("%d"), (int)mGain));
   mFreqT->SetValue(wxString::Format(wxT("%d"), (int)mFreq));
   mTimeT->SetValue(wxString::Format(wxT("%.2f"), mTime));

   return true;
}

bool NoiseRemovalDialog::TransferDataFromWindow()
{
   mGain = mGainS->GetValue();
   mFreq = mFreqS->GetValue() * 10;
   mTime = mTimeS->GetValue() * 0.01;

   return true;
}

void NoiseRemovalDialog::OnGainText(wxCommandEvent & event)
{
   long val;
   mGainT->GetValue().ToLong(&val);
   mGainS->SetValue(TrapLong(val, GAIN_MIN, GAIN_MAX));
}

void NoiseRemovalDialog::OnFreqText(wxCommandEvent & event)
{
   long val;
   mFreqT->GetValue().ToLong(&val);
   mFreqS->SetValue(TrapLong(val/10, FREQ_MIN, FREQ_MAX));
}

void NoiseRemovalDialog::OnTimeText(wxCommandEvent & event)
{
   long val;
   mTimeT->GetValue().ToLong(&val);
   mTimeS->SetValue(TrapLong(val*100, TIME_MIN, TIME_MAX));
}

void NoiseRemovalDialog::OnGainSlider(wxCommandEvent & event)
{
   mGain = mGainS->GetValue();
   mGainT->SetValue(wxString::Format(wxT("%d"), (int)mGain));
}

void NoiseRemovalDialog::OnFreqSlider(wxCommandEvent & event)
{
   mFreq = mFreqS->GetValue() * 10;
   mFreqT->SetValue(wxString::Format(wxT("%d"), (int)mFreq));
}

void NoiseRemovalDialog::OnTimeSlider(wxCommandEvent & event)
{
   mTime = mTimeS->GetValue() * 0.01;
   mTimeT->SetValue(wxString::Format(wxT("%.2f"), mTime));
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
// arch-tag: 685e0d8c-89eb-427b-8933-af606cf33c2b

