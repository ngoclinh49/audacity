/**********************************************************************

  Audacity: A Digital Audio Editor

  Filter.cpp

  Dominic Mazzoni

**********************************************************************/

#include <math.h>

#include <wx/msgdlg.h>
#include <wx/textdlg.h>

#include "Filter.h"
#include "../WaveTrack.h"
#include "../FFT.h"

EffectFilter::EffectFilter()
{
}

bool EffectFilter::Begin(wxWindow * parent)
{
   wxMessageBox("Work in progress: this will be a generic FFT filter.  "
                "Right now it just applies one fixed filter.");

   return true;
}

bool EffectFilter::DoIt(WaveTrack * t, sampleCount start, sampleCount len)
{
   sampleCount s = start;
   sampleCount idealBlockLen = 65536;
   int windowSize = 256;

   sampleType *buffer = new sampleType[idealBlockLen];

   sampleType *window1 = new sampleType[windowSize];
   sampleType *window2 = new sampleType[windowSize];
   sampleType *thisWindow = window1;
   sampleType *lastWindow = window2;

   int i;

   for (i = 0; i < windowSize; i++)
      lastWindow[i] = 0;

   while (len) {
      int block = idealBlockLen;
      if (block > len)
         block = len;

      t->Get(buffer, s, block);

      for (i = 0; i < block; i += windowSize / 2) {
         int wlen = i + windowSize;
         int wcopy = windowSize;
         if (i + wcopy > block)
            wcopy = block - i;

         int j;
         for (j = 0; j < wcopy; j++)
            thisWindow[j] = buffer[i + j];
         for (j = wcopy; j < windowSize; j++)
            thisWindow[j] = 0;

         Filter(windowSize, thisWindow);

         for (j = 0; j < windowSize / 2; j++)
            buffer[i + j] = thisWindow[j] + lastWindow[windowSize / 2 + j];

         sampleType *tempP = thisWindow;
         thisWindow = lastWindow;
         lastWindow = tempP;
      }

      if (len > block && len > windowSize / 2)
         block -= windowSize / 2;

      t->Set(buffer, s, block);

      len -= block;
      s += block;
   }

   delete[]buffer;
   delete[]window1;
   delete[]window2;

   return true;
}

void EffectFilter::Filter(sampleCount len, sampleType * buffer)
{
   float *inr = new float[len];
   float *ini = new float[len];
   float *outr = new float[len];
   float *outi = new float[len];

   int i;

   for (i = 0; i < len; i++)
      inr[i] = buffer[i] / 32767.;

   WindowFunc(2, len, inr);

   FFT(len, false, inr, NULL, outr, outi);

   //  RealFFT(len, inr, outr, outi);

   // Apply filter

   int half = len / 2;
   for (i = 0; i <= half; i++) {
      int j = len - i;

      outr[i] = outr[i] * sin(float (i) / half);
      outi[i] = outi[i] * sin(float (i) / half);

      if (i != 0 && i != len / 2) {
         outr[j] = outr[j] * sin(float (i) / half);
         outi[j] = outi[j] * sin(float (i) / half);
      }
   }

   FFT(len, true, outr, outi, inr, ini);

   for (i = 0; i < len; i++)
      buffer[i] = sampleType(inr[i] * 32767);
}
