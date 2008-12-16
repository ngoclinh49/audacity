/**********************************************************************

Audacity: A Digital Audio Editor

SBSMSEffect.cpp

Clayton Otey

This abstract class contains all of the common code for an
effect that uses SBSMS to do its processing (TimeScale)

**********************************************************************/

#include "../Audacity.h"

#if USE_SBSMS

#include <math.h>

#include "SBSMSEffect.h"
#include "../WaveTrack.h"
#include "../Project.h"

struct resampleBuf {
   audio *buf;
   double ratio;
   sampleCount block;
   sampleCount offset;
   sampleCount end;
   float *leftBuffer;
   float *rightBuffer;
   WaveTrack *leftTrack;
   WaveTrack *rightTrack;
};

long samplesCB(audio *chdata, long numFrames, void *userData)
{ 
   sbsmsInfo *si = (sbsmsInfo*) userData;
   long n_read = si->rs->read(chdata, numFrames);
   return n_read;
}

real stretchCB(long nProcessed, void *userData)
{
   sbsmsInfo *si = (sbsmsInfo*) userData;
   real t0 = (real)nProcessed/(real)si->samplesToProcess;
   real stretch = si->stretch0 + (si->stretch1-si->stretch0)*t0;
   return stretch;
}

real ratioCB(long nProcessed, void *userData)
{
   sbsmsInfo *si = (sbsmsInfo*) userData;
   real t0 = (real)nProcessed/(real)si->samplesToProcess;
   real stretch = si->stretch0 + (si->stretch1-si->stretch0)*t0;
   real t1;
   if(stretch == si->stretch0)
      t1 = 1.0/stretch;
   else
      t1 = log(stretch/si->stretch0)/(stretch-si->stretch0);
   
   real ratio = si->ratio0 + (si->ratio1-si->ratio0)*t1*(real)nProcessed/(real)si->samplesToGenerate;
   
   return ratio;
}

void EffectSBSMS :: setParameters(double rateStart, double rateEnd, double pitchStart, double pitchEnd, int quality, bool bPreAnalyze)
{
   this->rateStart = rateStart;
   this->rateEnd = rateEnd;
   this->pitchStart = pitchStart;
   this->pitchEnd = pitchEnd;
   this->quality = quality;
   this->bPreAnalyze = bPreAnalyze;
}

bool EffectSBSMS :: bInit = FALSE;

long resampleCB(void *cb_data, sbsms_resample_frame *data) 
{
   resampleBuf *r = (resampleBuf*) cb_data;
   
   long blockSize = r->leftTrack->GetBestBlockSize(r->offset);
   
   //Adjust the block size if it is the final block in the track
   if (r->offset + blockSize > r->end)
      blockSize = r->end - r->offset;
   
   // Get the samples from the tracks and put them in the buffers.
   r->leftTrack->Get((samplePtr)(r->leftBuffer), floatSample, r->offset, blockSize);
   r->rightTrack->Get((samplePtr)(r->rightBuffer), floatSample, r->offset, blockSize);
   
   // convert to sbsms audio format
   for(int i=0; i<blockSize; i++) {
      r->buf[i][0] = r->leftBuffer[i];
      r->buf[i][1] = r->rightBuffer[i];
   }
   
   r->offset += blockSize;
   data->in = r->buf;
   data->size = blockSize;
   data->ratio0 = r->ratio;
   data->ratio1 = r->ratio;
   
   return blockSize;
}

bool EffectSBSMS::Process()
{
   if(!bInit) {
      sbsms_init(4096);
      bInit = TRUE;
   }
   
   bool bGoodResult = true;
   
   //Iterate over each track
   this->CopyInputWaveTracks(); // Set up mOutputWaveTracks.
   TrackListIterator iter(mOutputWaveTracks);
   WaveTrack* leftTrack = (WaveTrack*)(iter.First());
   mCurTrackNum = 0;

   double len = leftTrack->GetEndTime() - leftTrack->GetStartTime();   
   double maxDuration = 0.0;
   
   while (leftTrack != NULL) {
      //Get start and end times from track
      mCurT0 = leftTrack->GetStartTime();
      mCurT1 = leftTrack->GetEndTime();
      
      //Set the current bounds to whichever left marker is
      //greater and whichever right marker is less
      mCurT0 = wxMax(mT0, mCurT0);
      mCurT1 = wxMin(mT1, mCurT1);
      
      // Process only if the right marker is to the right of the left marker
      if (mCurT1 > mCurT0) {
         sampleCount start;
         sampleCount end;
         start = leftTrack->TimeToLongSamples(mCurT0);
         end = leftTrack->TimeToLongSamples(mCurT1);
         
         WaveTrack* rightTrack = NULL;
         if (leftTrack->GetLinked()) {
            double t;
            rightTrack = (WaveTrack*)(iter.Next());
            
            //Adjust bounds by the right tracks markers
            t = rightTrack->GetStartTime();
            t = wxMax(mT0, t);
            mCurT0 = wxMin(mCurT0, t);
            t = rightTrack->GetEndTime();
            t = wxMin(mT1, t);
            mCurT1 = wxMax(mCurT1, t);
            
            //Transform the marker timepoints to samples
            start = leftTrack->TimeToLongSamples(mCurT0);
            end = leftTrack->TimeToLongSamples(mCurT1);
            
            mCurTrackNum++; // Increment for rightTrack, too.	
         }
         
         // SBSMS has a fixed sample rate - we just convert to its sample rate and then convert back
         float srIn = leftTrack->GetRate();
         float srSBSMS = 44100.0;
         
         // the resampler needs a callback to supply its samples
         resampleBuf rb;
         sampleCount maxBlockSize = leftTrack->GetMaxBlockSize();
         rb.block = maxBlockSize;
         rb.buf = (audio*)calloc(rb.block,sizeof(audio));
         rb.leftTrack = leftTrack;
         rb.rightTrack = rightTrack?rightTrack:leftTrack;
         rb.leftBuffer = (float*)calloc(maxBlockSize,sizeof(float));
         rb.rightBuffer = (float*)calloc(maxBlockSize,sizeof(float));
         rb.offset = start;
         rb.end = end;
         rb.ratio = srSBSMS/srIn;
         Resampler *resampler = new Resampler(resampleCB, &rb);
         
         // Samples in selection
         sampleCount samplesIn = end-start;
         
         // Samples for SBSMS to process after resampling
         sampleCount samplesToProcess = (sampleCount) ((real)samplesIn*(srSBSMS/srIn));
         
         // Samples in output after resampling back
         real stretch2;
         if(rateStart == rateEnd)
            stretch2 = 1.0/rateStart;
         else
            stretch2 = 1.0/(rateEnd-rateStart)*log(rateEnd/rateStart);
         sampleCount samplesToGenerate = (sampleCount) ((real)samplesToProcess * stretch2);
         sampleCount samplesOut = (sampleCount) ((real)samplesIn * stretch2);
         double duration =  (mCurT1-mCurT0) * stretch2;
         if(duration > maxDuration)
            maxDuration = duration;
         
         sbsmsInfo si;
         si.rs = resampler;
         si.samplesToProcess = samplesToProcess;
         si.samplesToGenerate = samplesToGenerate;
         si.stretch0 = rateStart;
         si.stretch1 = rateEnd;
         si.ratio0 = pitchStart;
         si.ratio1 = pitchEnd;
         
         sbsms *sbsmser = sbsms_create(&samplesCB,&stretchCB,&ratioCB,rightTrack?2:1,quality,bPreAnalyze,true);
         pitcher *pitch = pitch_create(sbsmser,&si,srIn/srSBSMS);
         
         WaveTrack* outputLeftTrack = mFactory->NewWaveTrack(leftTrack->GetSampleFormat(),
                                                             leftTrack->GetRate());
         WaveTrack* outputRightTrack = rightTrack?mFactory->NewWaveTrack(rightTrack->GetSampleFormat(),
                                                                         rightTrack->GetRate()):NULL;
         
         
         sampleCount blockSize = SBSMS_FRAME_SIZE[quality];
         audio *outBuf = (audio*)calloc(blockSize,sizeof(audio));
         float *outputLeftBuffer = (float*)calloc(blockSize*2,sizeof(float));
         float *outputRightBuffer = rightTrack?(float*)calloc(blockSize*2,sizeof(float)):NULL;
         
         long pos = 0;
         long outputCount = -1;
         
         // pre analysis
         real fracPre = 0.0f;
         if(bPreAnalyze) {
            fracPre = 0.05f;
            Resampler *resamplerPre;
            resampleBuf rbPre;
            rbPre.block = maxBlockSize;
            rbPre.buf = (audio*)calloc(rb.block,sizeof(audio));
            rbPre.leftTrack = leftTrack;
            rbPre.rightTrack = rightTrack?rightTrack:leftTrack;
            rbPre.leftBuffer = (float*)calloc(maxBlockSize,sizeof(float));
            rbPre.rightBuffer = (float*)calloc(maxBlockSize,sizeof(float));
            rbPre.offset = start;
            rbPre.end = end;
            rbPre.ratio = srSBSMS/srIn;
            resamplerPre = new Resampler(resampleCB, &rbPre);
            si.rs = resamplerPre;
            
            long pos = 0;
            long lastPos = 0;
            long ret = 0;
            while(lastPos<samplesToProcess) {
               ret = sbsms_pre_analyze(&samplesCB,&si,sbsmser);
               lastPos = pos;
               pos += ret;
               real completion = (real)lastPos/(real)samplesToProcess;
               if (TrackProgress(0,fracPre*completion))
                  return false;
            }
            sbsms_pre_analyze_complete(sbsmser);
            free(rbPre.buf);
            free(rbPre.leftBuffer);
            free(rbPre.rightBuffer);
            sbsms_reset(sbsmser);
            si.rs = resampler;
         }
         
         // process
         while(pos<samplesOut && outputCount) {
            long frames;
            if(pos+blockSize>samplesOut) {
               frames = samplesOut - pos;
            } else {
               frames = blockSize;
            }
            
            outputCount = pitch_process(outBuf, frames, pitch);
            for(int i = 0; i < outputCount; i++) {
               outputLeftBuffer[i] = outBuf[i][0];
               if(rightTrack) outputRightBuffer[i] = outBuf[i][1];
            }
            pos += outputCount;
            outputLeftTrack->Append((samplePtr)outputLeftBuffer, floatSample, outputCount);
            if(rightTrack) outputRightTrack->Append((samplePtr)outputRightBuffer, floatSample, outputCount);
            
            double frac = (double)pos/(double)samplesOut;
            int nWhichTrack = mCurTrackNum;
            if(rightTrack) {
               nWhichTrack = 2*(mCurTrackNum/2);
               if (frac < 0.5)
                  frac *= 2.0; // Show twice as far for each track, because we're doing 2 at once. 
               else {
                  nWhichTrack++;
                  frac -= 0.5;
                  frac *= 2.0; // Show twice as far for each track, because we're doing 2 at once. 
               }
            }
            if (TrackProgress(nWhichTrack, fracPre + (1.0-fracPre)*frac))
               return false;
         }
         outputLeftTrack->Flush();
         if(rightTrack) outputRightTrack->Flush();
         
         leftTrack->HandleClear(mCurT0, mCurT1, false, false);
         leftTrack->HandlePaste(mCurT0, outputLeftTrack);
         if(rightTrack) {
            rightTrack->HandleClear(mCurT0, mCurT1, false, false);
            rightTrack->HandlePaste(mCurT0, outputRightTrack);
         }
         
         delete outputLeftTrack;
         if(rightTrack) delete outputRightTrack;
         free(outputLeftBuffer);
         if(rightTrack) free(outputRightBuffer);
         free(outBuf);
         free(rb.leftBuffer);
         free(rb.rightBuffer);
         free(rb.buf);
         delete resampler;
         sbsms_destroy(sbsmser);
         pitch_destroy(pitch);
      }
      
      //Iterate to the next track
      leftTrack = (WaveTrack*)(iter.Next());
      mCurTrackNum++;
   }
   
   this->ReplaceProcessedWaveTracks(bGoodResult); 
   
#ifdef EXPERIMENTAL_FULL_LINKING
   AudacityProject *p = (AudacityProject*)mParent;
   if( p && p->IsSticky() ){
      leftTrack = (WaveTrack*)(iter.First());
      double newLen = leftTrack->GetEndTime() - leftTrack->GetStartTime();
      double timeAdded = newLen-len;
      double sel = mCurT1-mCurT0;
      double percent = (sel/(timeAdded+sel))*100 - 100;
      if ( !(HandleGroupChangeSpeed(percent, mCurT0, mCurT1)) ) bGoodResult = false;
   }
#endif

   // Update selection
   mT0 = mCurT0;
   mT1 = mCurT0 + maxDuration;
   
   return bGoodResult;
}

#endif
