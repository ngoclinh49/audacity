/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackArtist.cpp

  Dominic Mazzoni

  This class handles the actual rendering of WaveTracks (both
  waveforms and spectra), NoteTracks, LabelTracks and TimeTracks.

  It's actually a little harder than it looks, because for
  waveforms at least it needs to cache the samples that are
  currently on-screen.

**********************************************************************/

#include "TrackArtist.h"

#include <float.h>
#include <math.h>

#include <wx/brush.h>
#include <wx/colour.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/gdicmn.h>
#include <wx/image.h>
#include <wx/pen.h>

#include "allegro.h"

#include "AColor.h"
#include "BlockFile.h"
#include "Envelope.h"
#include "Track.h"
#include "NoteTrack.h"
#include "WaveTrack.h"
#include "LabelTrack.h"
#include "TimeTrack.h"
#include "Prefs.h"
#include "Sequence.h"
#include "Spectrum.h"
#include "ViewInfo.h"
#include "widgets/Ruler.h"

const int octaveHeight = 62;
const int blackPos[5] = { 6, 16, 32, 42, 52 };
const int whitePos[7] = { 0, 9, 17, 26, 35, 44, 53 };
const int notePos[12] = { 1, 6, 11, 16, 21,
   27, 32, 37, 42, 47, 52, 57
};

TrackArtist::TrackArtist()
{
   AColor::Init();

   mInsetLeft   = 0;
   mInsetTop    = 0;
   mInsetRight  = 0;
   mInsetBottom = 0;

   blankBrush     .SetColour(214, 214, 214);
   unselectedBrush.SetColour(192, 192, 192);
   selectedBrush  .SetColour(148, 148, 170);
   sampleBrush    .SetColour( 50,  50, 200);
   selsampleBrush .SetColour( 50,  50, 200);
   dragsampleBrush.SetColour(  0,   0,   0);

   blankPen     .SetColour(214, 214, 214);
   unselectedPen.SetColour(192, 192, 192);
   selectedPen  .SetColour(148, 148, 170);
   samplePen    .SetColour( 50,  50, 200);
   selsamplePen .SetColour( 50,  50, 200);
   rmsPen       .SetColour(100, 100, 220);
   shadowPen    .SetColour(148, 148, 148);

   vruler = new Ruler();
}

TrackArtist::~TrackArtist()
{
   delete vruler;
}

void TrackArtist::SetInset(int left, int top, int right, int bottom)
{
   mInsetLeft   = left;
   mInsetTop    = top;
   mInsetRight  = right;
   mInsetBottom = bottom;
}

void TrackArtist::DrawTracks(TrackList * tracks,
                             wxDC & dc, wxRect & r,
                             wxRect & clip,
                             ViewInfo * viewInfo,
                             bool drawEnvelope,bool drawSamples,bool drawSliders)
{
   wxRect trackRect = r;

   TrackListIterator iter(tracks);
   Track *t = iter.First();
   while (t) {
      trackRect.height = t->GetHeight();

      if (trackRect.y < (clip.y + clip.height) &&
          trackRect.y + trackRect.height > clip.y) {

         wxRect rr = trackRect;
         rr.x += mInsetLeft;
         rr.y += mInsetTop;
         rr.width -= (mInsetLeft + mInsetRight);
         rr.height -= (mInsetTop + mInsetBottom);

         switch (t->GetKind()) {
         case Track::Wave:
            switch (((WaveTrack *)t)->GetDisplay()) {
            case WaveTrack::WaveformDisplay:
               DrawWaveform((WaveTrack *)t, dc, rr, viewInfo,
                            drawEnvelope, drawSamples, drawSliders, false);
               break;
            case WaveTrack::WaveformDBDisplay:
               DrawWaveform((WaveTrack *)t, dc, rr, viewInfo,
                            drawEnvelope,  drawSamples, drawSliders, true);
               break;
            case WaveTrack::SpectrumDisplay:
               DrawSpectrum((WaveTrack *)t, dc, rr, viewInfo, false);
               break;
            case WaveTrack::PitchDisplay:
               DrawSpectrum((WaveTrack *)t, dc, rr, viewInfo, true);
               break;
            }
            break;              // case Wave
         case Track::Note:
            DrawNoteTrack((NoteTrack *)t, dc, rr, viewInfo);
            break;
         case Track::Label:
            DrawLabelTrack((LabelTrack *)t, dc, rr, viewInfo);
            break;
         case Track::Time:
            DrawTimeTrack((TimeTrack *)t, dc, rr, viewInfo);
            break;
         }
      }

      t = iter.Next();
      trackRect.y += trackRect.height;
   }
}

void TrackArtist::DrawVRuler(Track *t, wxDC * dc, wxRect & r)
{
   if (t->GetKind() == Track::Wave
       && ((WaveTrack *) t)->GetDisplay() == 0) {
      wxRect bev = r;
      bev.Inflate(-1, -1);
      AColor::Bevel(*dc, true, bev);

      vruler->SetBounds(r.x, r.y+1, r.x + r.width, r.y + r.height-2);
      vruler->SetOrientation(wxVERTICAL);
      vruler->SetRange(1.0, -1.0);
      vruler->SetFormat(Ruler::RealFormat);

      vruler->Draw(*dc);
   }

   if (t->GetKind() == Track::Wave
       && ((WaveTrack *) t)->GetDisplay() == 1) {
      // Waveform (db)
      wxRect bev = r;
      bev.Inflate(-1, -1);
      AColor::Bevel(*dc, true, bev);
   }

   if (t->GetKind() == Track::Wave
       && ((WaveTrack *) t)->GetDisplay() == 2) {
      // Spectrum
      wxRect bev = r;
      bev.Inflate(-1, -1);
      AColor::Bevel(*dc, true, bev);

      if (r.height < 60)
         return;

      double rate = ((WaveTrack *) t)->GetRate();
      int windowSize = gPrefs->Read("/Spectrum/FFTSize", 256);
      int maxFreq = gPrefs->Read("/Spectrum/MaxFreq", 8000);
      int maxSamples = int (maxFreq * windowSize / rate + 0.5);
      if (maxSamples > windowSize / 2)
         maxSamples = windowSize / 2;
      maxFreq = int (maxSamples * rate / windowSize + 0.5);
      int minFreq = int (rate / windowSize);

      wxString num;
      long textWidth, textHeight;

      num.Printf("%dK", int (maxFreq / 1000 + 0.5));
      dc->GetTextExtent(num, &textWidth, &textHeight);
      dc->DrawText(num, r.x + r.width - 3 - textWidth, r.y + 2);
      num = "Hz";
      dc->GetTextExtent(num, &textWidth, &textHeight);
      dc->DrawText(num, r.x + r.width - 3 - textWidth,
                   r.y + textHeight + 2);

      num.Printf("%d", minFreq);
      dc->GetTextExtent(num, &textWidth, &textHeight);
      dc->DrawText(num, r.x + r.width - 3 - textWidth,
                   r.y + r.height - 2 - 2 * textHeight);

      num = "Hz";
      dc->GetTextExtent(num, &textWidth, &textHeight);
      dc->DrawText(num, r.x + r.width - 3 - textWidth,
                   r.y + r.height - 2 - textHeight);
   }

   if (t->GetKind() == Track::Wave
       && ((WaveTrack *) t)->GetDisplay() == 3) {
      // Pitch
      wxRect bev = r;
      bev.Inflate(-1, -1);
      AColor::Bevel(*dc, true, bev);
   }

   if (t->GetKind() == Track::Note) {

      dc->SetPen(*wxTRANSPARENT_PEN);
      dc->SetBrush(*wxWHITE_BRUSH);
      wxRect bev = r;
      bev.x++;
      bev.y++;
      bev.width--;
      bev.height--;
      dc->DrawRectangle(bev);

      r.y += 2;
      r.height -= 2;

      int bottomNote = ((NoteTrack *) t)->GetBottomNote();
      int bottom = r.height +
          ((bottomNote / 12) * octaveHeight + notePos[bottomNote % 12]);

      wxPen hilitePen;
      hilitePen.SetColour(120, 120, 120);
      wxBrush blackKeyBrush;
      blackKeyBrush.SetColour(70, 70, 70);

      dc->SetBrush(blackKeyBrush);

      int fontSize = 10;
#ifdef __WXMSW__
      fontSize = 8;
#endif

      wxFont labelFont(fontSize, wxSWISS, wxNORMAL, wxNORMAL);
      dc->SetFont(labelFont);

      for (int octave = 0; octave < 50; octave++) {
         int obottom = bottom - octave * octaveHeight;
         if (obottom < 0)
            break;

         dc->SetPen(*wxBLACK_PEN);
         for (int white = 0; white < 7; white++)
            if (r.y + obottom - whitePos[white] > r.y &&
                r.y + obottom - whitePos[white] < r.y + r.height)
               dc->DrawLine(r.x, r.y + obottom - whitePos[white],
                            r.x + r.width,
                            r.y + obottom - whitePos[white]);

         wxRect br = r;
         br.height = 5;
         br.x++;
         br.width = 17;
         for (int black = 0; black < 5; black++) {
            br.y = r.y + obottom - blackPos[black] - 4;
            if (br.y > r.y && br.y + br.height < r.y + r.height) {
               dc->SetPen(hilitePen);
               dc->DrawRectangle(br);
               dc->SetPen(*wxBLACK_PEN);
               dc->DrawLine(br.x + 1, br.y + br.height - 1,
                            br.x + br.width, br.y + br.height - 1);
               dc->DrawLine(br.x + br.width - 1, br.y + 1,
                            br.x + br.width - 1, br.y + br.height - 1);
            }
         }

         if (octave >= 2 && octave <= 9) {
            wxString s;
            s.Printf("C%d", octave - 2);
            long width, height;
            dc->GetTextExtent(s, &width, &height);
            if (r.y + obottom - height + 4 > r.y &&
                r.y + obottom + 4 < r.y + r.height) {
               dc->SetTextForeground(wxColour(60, 60, 255));
               dc->DrawText(s, r.x + r.width - width,
                            r.y + obottom - height + 2);
            }
         }
      }
   }
}

int TrackArtist::GetWaveYPos(float value, int height, bool dB)
{
   float sign = (value >= 0 ? 1 : -1);

   if (dB) {
      if (value == 0 || height == 0)
         return 0;
      float db = 10 * log10(fabs(value));
      // The smallest value we will see is -45.15 (10*log10(1/32768))
      float val = (db + 45.0) / 45.0;
      if (val < 0.0)
         val = float(0.0);
      if (val > 1.0)
         val = float(1.0);

      return (int) (sign * (height * val + 0.5));
   } else {
      if (value < -1.0)
         value = float(-1.0);
      if (value > 1.0)
         value = float(1.0);
      return (int) (value * height + sign * 0.5);
   }
}

int TrackArtist::GetWaveYPosUnclipped(float value, int height, bool dB)
{
   float sign = (value >= 0 ? 1 : -1);

   if (dB) {
      if (value == 0 || height == 0)
         return 0;
      float db = 10 * log10(fabs(value));
      // The smallest value we will see is -45.15 (10*log10(1/32768))
      float val = (db + 45.0) / 45.0;
      if (val < 0.0)
         val = float(0.0);
      return (int) (sign * (height * val + 0.5));
   } else {
      return (int) (value * height + sign * 0.5);
   }
}

void TrackArtist::DrawWaveform(WaveTrack *track,
                               wxDC & dc, wxRect & r,
                               ViewInfo * viewInfo,
                               bool drawEnvelope, bool drawSamples,bool drawSliders, bool dB)
{
   double h = viewInfo->h;          //The horizontal position in seconds
   double pps = viewInfo->zoom;     //points-per-second--the zoom level
   double sel0 = viewInfo->sel0;    //left selection bound
   double sel1 = viewInfo->sel1;    //right selection bound

   //Get the sequence of samples from the actual track structure
   Sequence *seq = track->GetSequence();
   double trackLen = track->GetEndTime() - track->GetStartTime();
   //sampleCount numSamples = seq->GetNumSamples();    //keep track of how many samples there are
   double tOffset = track->GetOffset();              //The offset of the track.


   //If the track isn't selected, make the selection be zero
   if (!track->GetSelected())
      sel0 = sel1 = 0.0;


   //Some bookkeeping time variables:
   double tpre = h - tOffset;                    // offset corrected time of selection beginning
   double tstep = 1.0 / pps;                     // Seconds per point
   double tpost = tpre + (r.width * tstep);      // offset corrected time of selection end

   double rate = track->GetRate();

   // Determine whether we should show individual sample or draw circular points as well
   bool showIndividualSamples = (pps / rate > 0.5);        //zoomed in a lot
   bool showPoints = (pps / rate > 3.0);                   //zoomed in even more

   //calculate actual selection bounds so that t0 > 0 and t1 < the end of the track
   double t0 = (tpre >= 0.0 ? tpre : 0.0);
   double t1 = (tpost < trackLen ? tpost : trackLen);

   //Make sure t1 (the right bound) is greater than 0
   if (t1 < 0.0) {
      t1 = 0.0;
   }

   //make sure t1 is greater than t0
   if (t0 > t1)
      t0 = t1;


   //calculate sample-based offset-corrected selection?
   int ssel0 = wxMax(0, int((sel0 - tOffset) * rate + 0.5));
   int ssel1 = wxMax(0, int((sel1 - tOffset) * rate + 0.5));

   //trim selection so that it only contains the actual samples
   if (ssel0 != ssel1 && ssel1 > (int)(0.5+trackLen*rate))
      ssel1 = (int)(0.5+trackLen*rate);

   //Draw a rectangle around the passed-in region (the selection)
   dc.SetBrush(blankBrush);
   dc.SetPen(blankPen);

   dc.DrawRectangle(r);

   //copy this rectangle away for future use.
   wxRect mid = r;

   //Do the following if tpre is less than t0 (e.g., if the track was shifted left of 0)
   if (t0 > tpre) {
      
      //adjust selection region by the negative offset.
      wxRect pre = r;
      pre.width = (int) ((t0 - tpre) * pps);
      mid.x += pre.width;
      mid.width -= pre.width;
      //dc.DrawRectangle(pre);        //STM:  Why is this commented out? 
   }

   //Do the following if the tpost is greater than t1 (e.g., if the track was shifted from 0)
   if (tpost > t1) {
      
      //adjust selection region by the offset
      wxRect post = r;
      post.x += (int) ((t1 - tpre) * pps);
      post.width = r.width - (post.x - r.x);

      mid.width -= post.width;
      if (post.x < r.x) {
         post.width -= (r.x - post.x);
         post.x = r.x;
      }

      //if (post.width > 0)        //STM: Why are these commented out?
      //dc.DrawRectangle(post);   
   }

   //ctr determines the horizontal center of the track
   int ctr = r.y + (r.height / 2);

   //initialize a bunch of pointers to data:
   int *heights = NULL;
   double *envValues = NULL;
   float *min=NULL, *max=NULL, *rms=NULL;
   sampleCount *where = NULL;

   if (mid.width > 0) {

      min = new float[mid.width];
      max = new float[mid.width];
      rms = new float[mid.width];
      where = new sampleCount[mid.width+1];

      //If there is nothing within these regions, clean up and bail out.  Otherwise, my
      //variables should be filled up with the appropriate numbers.
      if (!track->GetWaveDisplay(min, max, rms, where,
                                 mid.width, t0, pps)) {
         delete[] min;
         delete[] max;
         delete[] rms;
         delete[] where;
         return;
      }
      heights = new int[mid.width];
      envValues = new double[mid.width];
   }

   // Start working on the envelope
   track->GetEnvelope()->GetValues(envValues, mid.width, t0 + tOffset, tstep);

   double t = t0;
   int x;
   for (x = 0; x < mid.width; x++) {
      heights[x] = GetWaveYPosUnclipped(envValues[x], mid.height / 2, dB);
      t += tstep;
   }

   // Draw track area
   bool usingSelPen = false;
   dc.SetPen(unselectedPen);

   int quarterHeight = r.height/4;
   int halfHeight    = r.height/2;
   
   // 'fix' to prevent us possibly dividing by zero on very narrow tracks.
   if (halfHeight <1)
      halfHeight=1;

   int lowerLimit;
   for (x = 0; x < mid.width; x++) {

      bool sel = false;
      if (ssel0 <= where[x] && where[x + 1] < ssel1)
         sel = true;

      if (sel && !usingSelPen)
         dc.SetPen(selectedPen);
      else if (!sel && usingSelPen)
         dc.SetPen(unselectedPen);
      usingSelPen = sel;

      //JKC: Now draw the envelope background (for this x)
      //The lower limit gives us a second 'envelope'
      //boundary inside the first one.  
      //The visual of a second boundary means we still know 
      //how wide the envelope is even when it is being used
      //to amplify and it is wider than the panel is.
      //If people like this visual cue we can perhaps build
      //drag-to-amplify functionality on to it.
      lowerLimit = heights[x]-quarterHeight;
      if((drawEnvelope) && ( lowerLimit > 0 ))
      {
         int h1 = heights[x] % halfHeight;
         int l1 = lowerLimit % halfHeight;
         if( h1 >= l1 )
         {
            //Draw envelope with an inner envelope boundary.
            //(Uses the existing track background for central part)
            dc.DrawLine(mid.x + x, ctr - h1, mid.x + x,
                  ctr - l1);
            dc.DrawLine(mid.x + x, ctr + h1, mid.x + x,
                  ctr + l1);
         }
         else
         {
            dc.DrawLine(mid.x + x, ctr - halfHeight, mid.x + x,
                  ctr - l1);
//            dc.DrawLine(mid.x + x, ctr-h1, mid.x + x,
//                  ctr + h1);
            dc.DrawLine(mid.x + x, ctr + l1, mid.x + x,
                  ctr + halfHeight);
         }
      }
      else
      {
         //Original draw envelope code for when no inner 
         //boundary is visible.
         dc.DrawLine(mid.x + x, ctr - heights[x], mid.x + x,
                  ctr + heights[x]);
      }
   }

   //Draw the center line in red.
   //Disabled until someone
   // a) makes center line an option and 
   // b) gives configurable color for it.
#if 0
   if (mid.width > 0) {
      dc.SetPen(*wxRED_PEN);
      dc.DrawLine(mid.x, ctr, mid.x + mid.width, ctr);
   }
#endif

   // Draw samples
   dc.SetPen(samplePen);

   if (showIndividualSamples) {
      // We're zoomed in really far, so show points and curves

      if (mid.width > 0) {
         sampleCount s0 = (sampleCount) (t0 * rate + 0.5);
         sampleCount slen = (sampleCount) (mid.width * rate / pps + 0.5);

         if (s0 > 1) {
            s0--;
         }
         slen += 4;
         if (s0 + slen > seq->GetNumSamples())
            slen = seq->GetNumSamples() - s0;

         float *buffer = new float[slen];
         seq->Get((samplePtr)buffer, floatSample, s0, slen);
         int *xpos = new int[slen];
         int *ypos = new int[slen];

         sampleCount s;
         for (s = 0; s < slen; s++) {
            double tt = double (s0 + s) / rate + tOffset;
            double xx = ((tt - h) * pps + 0.5);

            if (xx < -10000)
               xx = -10000;
            if (xx > 10000)
               xx = 10000;

            xpos[s] = (int) xx;
            ypos[s] = ctr - GetWaveYPos(buffer[s] *
                                        track->GetEnvelope()->GetValue(tt),
                                        mid.height / 2, dB);
         }

         // Draw lines
         for (s = 0; s < slen - 1; s++) {
            dc.DrawLine(r.x + xpos[s], ypos[s],
                        r.x + xpos[s + 1], ypos[s + 1]);
         }

         if (showPoints) {
            // Draw points
            int tickSize= drawSamples ? 4 : 3;// Bigger ellipses when draggable.
            wxRect pr;
            pr.width = tickSize;
            pr.height = tickSize;
            //different colour when draggable.
            dc.SetBrush( drawSamples ? dragsampleBrush : sampleBrush);
            for (s = 0; s < slen; s++) {
               pr.x = r.x + xpos[s] - tickSize/2;
               pr.y = ypos[s] - tickSize/2;
               dc.DrawEllipse(pr);
            }
         }

         delete[]buffer;
         delete[]xpos;
         delete[]ypos;
      }
   }
   else if (mid.width > 0) {

      // Display a line representing the
      // min and max of the samples in this region
      t = t0;
      short *h1 = new short[mid.width];
      short *h2 = new short[mid.width];

      for (x = 0; x < mid.width; x++) {
         h1[x] = ctr - GetWaveYPos(min[x] *
                                   track->GetEnvelope()->GetValue(t + tOffset),
                                   mid.height / 2,
                                   dB);
         h2[x] = ctr - GetWaveYPos(max[x] *
                                   track->GetEnvelope()->GetValue(t + tOffset),
                                   mid.height / 2,
                                   dB);

         //JKC: This adjustment to h1[] and h2[] ensures that the drawn
         //waveform is continuous.
         if( x>0 )
         {
            if( h1[x] < h2[x-1] )
               h1[x]=h2[x-1]-1;
            if( h2[x] > h1[x-1] )
               h2[x]=h1[x-1]+1;
         }

         dc.DrawLine(mid.x + x, h2[x], mid.x + x, h1[x]+1 );
         
         t += tstep;
      }

      dc.SetPen(rmsPen);

      // Draw the RMS (root-mean-squared) of the samples within the
      // region
      t = t0;
      for (x = 0; x < mid.width; x++) {

         int r1 = ctr - GetWaveYPos(-rms[x] *
                                    track->GetEnvelope()->GetValue(t + tOffset),
                                    mid.height / 2,
                                    dB);
         int r2 = ctr - GetWaveYPos(rms[x] *
                                    track->GetEnvelope()->GetValue(t + tOffset),
                                    mid.height / 2,
                                    dB);

         // Ensure rms doesn't totally obscure samples further out.
         if (r1 > h1[x]-1)
            r1 = h1[x]-1;
         if (r2 < h2[x]+1)
            r2 = h2[x]+1;

         // Draw rms provided it's got some +ve width.
         if( r2 <= r1)
            dc.DrawLine(mid.x + x, r2, mid.x + x, r1+1 );

         t += tstep;
      }

      delete[] h1;
      delete[] h2;
   }

   if (drawEnvelope) {
      dc.SetPen(AColor::envelopePen);
      int MaxHeight = mid.height/2;

      for (x = 0; x < mid.width; x++) {
         // JKC: This 'if' draws the envelope boundary dotted if it goes off screen.
         if (heights[x] > MaxHeight )
         {
            // three dots on, one dot off, three on, etc...
            if( x%4 == 3 )
               continue;
            heights[x]=MaxHeight;
         }

         int z1 = ctr - heights[x] + 3 > ctr ? ctr : ctr - heights[x] + 3;
         int z2 = ctr + heights[x] - 3 < ctr ? ctr : ctr + heights[x] - 3;
         dc.DrawLine(mid.x + x, ctr - heights[x], mid.x + x, z1);
         dc.DrawLine(mid.x + x, ctr + heights[x], mid.x + x, z2);

      }
   }

   if (heights)
      delete[]heights;
   if (envValues)
      delete[]envValues;

   // Draw arrows on the left side if the track extends to the left of the
   // beginning of time.  :)
   if (h == 0.0 && tOffset < 0.0) {
      dc.SetPen(*wxWHITE_PEN);
      dc.DrawLine(r.x + 2, r.y + 6, r.x + 8, r.y + 6);
      dc.DrawLine(r.x + 2, r.y + 6, r.x + 6, r.y + 2);
      dc.DrawLine(r.x + 2, r.y + 6, r.x + 6, r.y + 10);

      dc.DrawLine(r.x + 2, r.y + r.height - 8, r.x + 8,
                  r.y + r.height - 8);
      dc.DrawLine(r.x + 2, r.y + r.height - 8, r.x + 6,
                  r.y + r.height - 4);
      dc.DrawLine(r.x + 2, r.y + r.height - 8, r.x + 6,
                  r.y + r.height - 12);
   }

   if (drawEnvelope) {
      wxRect envRect = r;
      envRect.height -= 2;
      track->GetEnvelope()->Draw(dc, envRect, h, pps, dB);
   }

   if (mid.width > 0) {
      delete[] min;
      delete[] max;
      delete[] rms;
      delete[] where;
   }
   
   if( drawSliders )
   {
      DrawTimeSlider(track,dc, r, viewInfo, true);  // directed right
      DrawTimeSlider(track,dc, r, viewInfo, false); // directed left
   }
}


void TrackArtist::DrawTimeSlider(WaveTrack *track,
                       wxDC & dc, wxRect & r, ViewInfo * viewInfo, bool rightwards)
{
   const int border=3; // 3 pixels all round.
   const int width=6; // width of the drag box.
   const int taper=6; // how much the box tapers by.
   const int barSpacing = 4; // how far apart the bars are.
   const int barWidth = 3;

   //Enough space to draw in?
   if( r.height <= ((taper+border + barSpacing) * 2 ))
      return;
   if( r.width <= (width*2 + border * 3))
      return;

   // The draggable box is tapered towards the direction you drag it.
   int leftTaper  = rightwards ? 0 : 6;
   int rightTaper = rightwards ? 6 : 0;

   int xLeft = rightwards ? (r.x +border) : (r.x + r.width - (border+width));
   int yTop  = r.y+border;
   int yBot  = r.y+r.height-border;

   dc.SetPen(*wxWHITE_PEN);
   dc.DrawLine( xLeft, yBot-leftTaper, xLeft, yTop+leftTaper );
   dc.DrawLine( xLeft, yTop+leftTaper, xLeft+width, yTop+rightTaper );
   dc.SetPen(*wxBLACK_PEN);
   dc.DrawLine( xLeft+width, yTop+rightTaper, xLeft+width, yBot-rightTaper );
   dc.DrawLine( xLeft+width, yBot-rightTaper, xLeft, yBot-leftTaper);

   int firstBar = yTop + taper + taper/2;
   int nBars    = (yBot-yTop-taper*3) / barSpacing +1;
   xLeft += (width-barWidth+1)/2;
   int y;
   for(int i=0;i<nBars;i++)
   {
      y = firstBar + barSpacing*i;
      dc.DrawLine( xLeft, y, xLeft+barWidth, y);
   }
}


void TrackArtist::DrawSpectrum(WaveTrack *track,
                               wxDC & dc, wxRect & r,
                               ViewInfo * viewInfo, bool autocorrelation)
{
   if(!viewInfo->bUpdateSpectrogram && viewInfo->bIsPlaying)
   {
      // BG: Draw (undecorated) waveform instead of spectrum
      DrawWaveform(track, dc, r, viewInfo, false, false, false, false);
      /*
      // BG: uncomment to draw grey instead of spectrum
      dc.SetBrush(unselectedBrush);
      dc.SetPen(unselectedPen);
      dc.DrawRectangle(r);
      */
      return;
   }

   double h = viewInfo->h;
   double pps = viewInfo->zoom;
   double sel0 = viewInfo->sel0;
   double sel1 = viewInfo->sel1;

   Sequence *seq = track->GetSequence();
   sampleCount numSamples = seq->GetNumSamples();
   double tOffset = track->GetOffset();
   double rate = track->GetRate();

   // if nothing is on the screen
   if ((sampleCount) (h * rate + 0.5) >= numSamples)
      return;

   if (!track->GetSelected())
      sel0 = sel1 = 0.0;

   int x = 0;
   double tpre = h - tOffset;
   double tstep = 1.0 / pps;

   double t0 = (tpre >= 0.0 ? tpre : 0.0);

   sampleCount ssel0 = (sampleCount) ((sel0 - tOffset) * rate + 0.5);
   sampleCount ssel1 = (sampleCount) ((sel1 - tOffset) * rate + 0.5);

   if (sel0 < tOffset)
       ssel0 = 0;
   if (sel1 < tOffset)
       ssel1 = 0;

   // We draw directly to a bit image in memory,
   // and then paint this directly to our offscreen
   // bitmap.  Note that this could be optimized even
   // more, but for now this is not bad.  -dmazzoni
   wxImage *image = new wxImage((int) r.width, (int) r.height);
   if (!image)
      return;
   unsigned char *data = image->GetData();

   float *freq = new float[r.width * r.height];
   sampleCount *where = new sampleCount[r.width+1];

   if (!track->GetSpectrogram(freq, where, r.width, r.height,
                              t0, pps, autocorrelation)) {
      delete image;
      delete[] where;
      delete[] freq;
      return;
   }

   bool isGrayscale = false;
   gPrefs->Read("/Spectrum/Grayscale", &isGrayscale, false);

   int i = 0;
   while (x < r.width) {
      sampleCount w0 = (sampleCount) ((tpre + x * tstep) * rate + 0.5);

      if (w0 < 0 || w0 >= numSamples) {
         for (int yy = 0; yy < r.height; yy++) {
            data[(yy * r.width + x) * 3] = 214;
            data[(yy * r.width + x) * 3 + 1] = 214;
            data[(yy * r.width + x) * 3 + 2] = 214;
         }
         x++;
         continue;
      }

      float *spec = &freq[r.height * i];

      for (int yy = 0; yy < r.height; yy++) {

         bool selflag = (ssel0 <= w0 && w0 < ssel1);

         unsigned char rv, gv, bv;

         GetColorGradient(spec[r.height - 1 - yy],
                          selflag, isGrayscale, &rv, &gv, &bv);

         data[(yy * r.width + x) * 3] = rv;
         data[(yy * r.width + x) * 3 + 1] = gv;
         data[(yy * r.width + x) * 3 + 2] = bv;
      }

      i++;
      x++;
   }

   wxBitmap converted = wxBitmap(image);

   //wxBitmap converted;
   //converted.Create(r.width, r.height);

   wxMemoryDC memDC;

   memDC.SelectObject(converted);

   dc.Blit(r.x, r.y, r.width, r.height, &memDC, 0, 0, wxCOPY, FALSE);

   //dc.DrawBitmap(converted, r.x, r.y);

   delete image;
   delete[] where;
   delete[] freq;
}

/*
Note: recall that Allegro attributes end in a type identifying letter.
 
In addition to standard notes, an Allegro_Note can denote a graphic.
A graphic is a note with a loud of zero (for quick testing) and an
attribute named "shapea" set to one of the following atoms:
    line
        from (time, pitch) to (time+dur, y1r), where y1r is an
          attribute
    rectangle
        from (time, pitch) to (time+dur, y1r), where y1r is an
          attribute
    triangle
        coordinates are (time, pitch), (x1r, y1r), (x2r, y2r)
        dur must be the max of x1r-time, x2r-time
    polygon
        coordinates are (time, pitch), (x1r, y1r), (x2r, y2r),
          (x3r, y3r), ... are coordinates (since we cannot represent
          arrays as attribute values, we just generate as many 
          attribute names as we need)
        dur must be the max of xNr-time for all N
    oval
        similar to rectangle
        Note: this oval has horizontal and vertical axes only
    text
        drawn at (time, pitch)
        duration should be zero (text is clipped based on time and duration,
          NOT based on actual coordinates)

and optional attributes as follows:
    linecolori is 0x00rrggbb format color for line or text foreground
    fillcolori is 0x00rrggbb format color for fill or text background
    linethicki is line thickness in pixels, 0 for no line
    filll is true to fill rectangle or draw text background (default is false)
    fonta is one of ['roman', 'swiss', 'modern'] (font, otherwise use default)
    weighta may be 'bold' (font) (default is normal)
    sizei is font size (default is 8)
    justifys is a string containing two letters, a horizontal code and a 
      vertical code. The horizontal code is as follows:
        l: the coordinate is to the left of the string (default)
        c: the coordinate is at the center of the string
        r: the coordinate is at the right of the string
      The vertical code is as follows:
        t: the coordinate is at the top of the string
        c: the coordinate is at the center of the string
        b: the coordinate is at the bottom of the string
        d: the coordinate is at the baseline of the string (default)
      Thus, -justifys:"lt" places the left top of the string at the point
        given by (pitch, time). The default value is "ld".
    
 */

/* Declare Static functions */
static char *IsShape(Allegro_note_ptr note);
static double LookupRealAttribute(Allegro_note_ptr note, Attribute attr, double def);
static long LookupIntAttribute(Allegro_note_ptr note, Attribute attr, long def);
static bool LookupLogicalAttribute(Allegro_note_ptr note, Attribute attr, bool def);
static const char *LookupStringAttribute(Allegro_note_ptr note, Attribute attr, const char *def);
static char *LookupAtomAttribute(Allegro_note_ptr note, Attribute attr, char *def);
static int PITCH_TO_Y(double p, int bottom);
static char *LookupAtomAttribute(Allegro_note_ptr note, Attribute attr, char *def);
static int PITCH_TO_Y(double p, int bottom);

// returns NULL if note is not a shape,
// returns atom (string) value of note if note is a shape
char *IsShape(Allegro_note_ptr note)
{
  Parameters_ptr parameters = note->parameters;
  while (parameters) {
    if (strcmp(parameters->parm.attr_name(), "shapea") == 0) {
      return parameters->parm.a;
    }
    parameters = parameters->next;
  }
  return NULL;
}

// returns value of attr, or default if not found
double LookupRealAttribute(Allegro_note_ptr note, Attribute attr, double def)
{
  Parameters_ptr parameters = note->parameters;
  while (parameters) {
    if (parameters->parm.attr_name() == attr + 1 &&
        parameters->parm.attr_type() == 'r') {
      return parameters->parm.r;
    }
    parameters = parameters->next;
  }
  return def;
}

// returns value of attr, or default if not found
long LookupIntAttribute(Allegro_note_ptr note, Attribute attr, long def)
{
  Parameters_ptr parameters = note->parameters;
  while (parameters) {
    if (parameters->parm.attr_name() == attr + 1 &&
        parameters->parm.attr_type() == 'i') {
      return parameters->parm.i;
    }
    parameters = parameters->next;
  }
  return def;
}

// returns value of attr, or default if not found
bool LookupLogicalAttribute(Allegro_note_ptr note, Attribute attr, bool def)
{
  Parameters_ptr parameters = note->parameters;
  while (parameters) {
    if (parameters->parm.attr_name() == attr + 1 &&
        parameters->parm.attr_type() == 'l') {
      return parameters->parm.l;
    }
    parameters = parameters->next;
  }
  return def;
}

// returns value of attr, or default if not found
const char *LookupStringAttribute(Allegro_note_ptr note, Attribute attr, const char *def)
{
  Parameters_ptr parameters = note->parameters;
  while (parameters) {
    if (parameters->parm.attr_name() == attr + 1 &&
        parameters->parm.attr_type() == 's') {
      return parameters->parm.s;
    }
    parameters = parameters->next;
  }
  return def;
}

// returns value of attr, or default if not found
char *LookupAtomAttribute(Allegro_note_ptr note, Attribute attr, char *def)
{
  Parameters_ptr parameters = note->parameters;
  while (parameters) {
    if (parameters->parm.attr_name() == attr + 1 &&
        parameters->parm.attr_type() == 'a') {
      return parameters->parm.s;
    }
    parameters = parameters->next;
  }
  return def;
}

#define TIME_TO_X(t) (r.x + (int) (((t) - h) * pps))
#define X_TO_TIME(xx) (((xx) - r.x) / pps + h)

// CLIP(x) changes x to lie between +/- CLIP_MAX due to graphics display problems
//  with very large coordinate values (this happens when you zoom in very far)
//  This will cause incorrect things to be displayed, but at these levels of zoom
//  you will only see a small fraction of the overall shape. Note that rectangles
//  and lines are clipped in a way that preserves correct graphics, so in
//  particular, line plots will be correct at any zoom (limited by floating point
//  precision).
#define CLIP_MAX 16000
#define CLIP(x) { long c = (x); if (c < -CLIP_MAX) c = -CLIP_MAX; \
                  if (c > CLIP_MAX) c = CLIP_MAX; (x) = c; }

#define RED(i) ( unsigned char )( (((i) >> 16) & 0xff) )
#define GREEN(i) ( unsigned char )( (((i) >> 8) & 0xff) )
#define BLUE(i) ( unsigned char )( ((i) & 0xff) )

//#define PITCH_TO_Y(p) (r.y + r.height - int(pitchht * ((p) + 0.5 - pitch0) + 0.5))

int PITCH_TO_Y(double p, int bottom)
{
   int octave = (((int) (p + 0.5)) / 12);
   int n = ((int) (p + 0.5)) % 12;
   
   return bottom - octave * octaveHeight - notePos[n] - 4;
}

void TrackArtist::DrawNoteTrack(NoteTrack *track,
                                wxDC &dc, wxRect &r,
                                ViewInfo *viewInfo)
{
  double h = viewInfo->h;
  double pps = viewInfo->zoom;
  double sel0 = viewInfo->sel0;
  double sel1 = viewInfo->sel1;

  double h1 = X_TO_TIME(r.x + r.width);

  Seq_ptr seq = track->mSeq;
  int visibleChannels = track->mVisibleChannels;

  if (!track->GetSelected())
	sel0 = sel1 = 0.0;

  int ctrpitch = 60;
  int pitch0;
  int pitchht = 4;

  int numPitches = r.height / pitchht;
  pitch0 = (ctrpitch - numPitches/2);

  int bottomNote = track->GetBottomNote();
  int bottom = r.height +
     ((bottomNote / 12) * octaveHeight + notePos[bottomNote % 12]);

  dc.SetBrush(blankBrush);
   dc.SetPen(blankPen);
   dc.DrawRectangle(r);

  wxPen blackStripePen;
  blackStripePen.SetColour(190, 190, 190);
   wxBrush blackStripeBrush;
   blackStripeBrush.SetColour(190, 190, 190);

   dc.SetBrush(blackStripeBrush);

   for (int octave = 0; octave < 50; octave++) {
      int obottom = r.y + bottom - octave * octaveHeight;

      if (obottom > r.y && obottom < r.y + r.height) {
         dc.SetPen(*wxBLACK_PEN);
         dc.DrawLine(r.x, obottom, r.x + r.width, obottom);
      }
      if (obottom - 26 > r.y && obottom - 26 < r.y + r.height) {
         dc.SetPen(blackStripePen);
         dc.DrawLine(r.x, obottom - 26, r.x + r.width, obottom - 26);
      }

      wxRect br = r;
      br.height = 5;
      for (int black = 0; black < 5; black++) {
         br.y = obottom - blackPos[black] - 4;
         if (br.y > r.y && br.y + br.height < r.y + r.height) {
            dc.SetPen(blackStripePen);
            dc.DrawRectangle(br);
         }
      }
   }

  dc.SetClippingRegion(r);
  int numEvents = seq->notes.len;
  int index;

  // NOTE: it would be better to put this in some global initialization
  // function rather than do lookups every time.
  char *line = symbol_table.insert_string("line");
  char *rectangle = symbol_table.insert_string("rectangle");
  char *triangle = symbol_table.insert_string("triangle");
  char *polygon = symbol_table.insert_string("polygon");
  char *oval = symbol_table.insert_string("oval");
  char *text = symbol_table.insert_string("text");
  char *texts = symbol_table.insert_string("texts");
  char *x1r = symbol_table.insert_string("x1r");
  char *x2r = symbol_table.insert_string("x2r");
  char *y1r = symbol_table.insert_string("y1r");
  char *y2r = symbol_table.insert_string("y2r");
  char *linecolori = symbol_table.insert_string("linecolori");
  char *fillcolori = symbol_table.insert_string("fillcolori");
  char *linethicki = symbol_table.insert_string("linethicki");
  char *filll = symbol_table.insert_string("filll");
  char *fonta = symbol_table.insert_string("fonta");
  char *roman = symbol_table.insert_string("roman");
  char *swiss = symbol_table.insert_string("swiss");
  char *modern = symbol_table.insert_string("modern");
  char *weighta = symbol_table.insert_string("weighta");
  char *bold = symbol_table.insert_string("bold");
  char *sizei = symbol_table.insert_string("sizei");
  char *justifys = symbol_table.insert_string("justifys");

  for(index=0; index<numEvents; index++) {

    if (seq->notes[index]->type == 'n') {
    
      Allegro_note_ptr note = (Allegro_note_ptr)(seq->notes[index]);
      
      if (visibleChannels & (1 << (seq->notes[index]->chan & 15))) {
        double x = note->time;
        double x1 = note->time + note->dur;
        if (x < h1 && x1 > h) { // omit if outside box
          char *shape = NULL;
          if (note->loud > 0.0 || !(shape = IsShape(note))) {

             int octave = (((int) (note->pitch + 0.5)) / 12);
             int n = ((int) (note->pitch + 0.5)) % 12;
             
             wxRect nr;
             nr.y = bottom - octave * octaveHeight - notePos[n]
                - 4;
             nr.height = 5;
             
             if (nr.y + nr.height >= 0 && nr.y < r.height) {
                
                if (nr.y + nr.height > r.height)
                   nr.height = r.height - nr.y;
                if (nr.y < 0) {
                   nr.height += nr.y;
                   nr.y = 0;
                }
                nr.y += r.y;
                
                nr.x = r.x + (int) ((note->time - h) * pps);
                nr.width = (int) (note->dur * pps) + 1;
                
                if (nr.x + nr.width >= r.x && nr.x < r.x + r.width) {
                   if (nr.x < r.x) {
                      nr.width -= (r.x - nr.x);
                      nr.x = r.x;
                   }
                   if (nr.x + nr.width > r.x + r.width)
                      nr.width = r.x + r.width - nr.x;
                   
                   AColor::MIDIChannel(&dc, note->chan + 1);
                   
                   if (note->time + note->dur >= sel0 && note->time <= sel1) {
                      dc.SetBrush(*wxWHITE_BRUSH);
                      dc.DrawRectangle(nr);
                   }
                   else {
                      dc.DrawRectangle(nr);
                      AColor::LightMIDIChannel(&dc, note->chan + 1);
                      dc.DrawLine(nr.x, nr.y, nr.x + nr.width-2, nr.y);
                      dc.DrawLine(nr.x, nr.y, nr.x, nr.y + nr.height-2);
                      AColor::DarkMIDIChannel(&dc, note->chan + 1);
                      dc.DrawLine(nr.x+nr.width-1, nr.y,
                                  nr.x+nr.width-1, nr.y+nr.height-1);
                      dc.DrawLine(nr.x, nr.y+nr.height-1,
                                  nr.x+nr.width-1, nr.y+nr.height-1);
                   }

                }
             }
             
          } else if (shape) {
            // draw a shape according to attributes
            // add 0.5 to pitch because pitches are plotted with height = pitchht,
            // thus, the center is raised by pitchht * 0.5
            int y = PITCH_TO_Y(note->pitch, bottom);
            long linecolor = LookupIntAttribute(note, linecolori, -1);
            long linethick = LookupIntAttribute(note, linethicki, 1);
            long fillcolor = -1;
            long fillflag = 0;

            // set default color to be that of channel
            AColor::MIDIChannel(&dc, note->chan+1);
            if (shape != text) {
              if (linecolor != -1)
                dc.SetPen(wxPen(wxColour(RED(linecolor), 
                                         GREEN(linecolor),
                                         BLUE(linecolor)),
                                linethick, wxSOLID));
            }
            if (shape != line) {
              fillcolor = LookupIntAttribute(note, fillcolori, -1);
              fillflag = LookupLogicalAttribute(note, filll, false);

              if (fillcolor != -1) 
                dc.SetBrush(wxBrush(wxColour(RED(fillcolor),
                                             GREEN(fillcolor),
                                             BLUE(fillcolor)),
                                    wxSOLID));
              if (!fillflag) dc.SetBrush(*wxTRANSPARENT_BRUSH);
            }
            int y1 = PITCH_TO_Y(LookupRealAttribute(note, y1r, note->pitch), bottom);
            if (shape == line) {
              // extreme zooms caues problems under windows, so we have to do some
              // clipping before calling display routine
              if (x < h) { // clip line on left
                y = int((y + (y1 - y) * (h - x) / (x1 - x)) + 0.5);
                x = h;
              }
              if (x1 > h1) { // clip line on right
                y1 = int((y + (y1 - y) * (h1 - x) / (x1 - x)) + 0.5);
                x1 = h1;
              }
              dc.DrawLine(TIME_TO_X(x), y, TIME_TO_X(x1), y1);
            } else if (shape == rectangle) {
              if (x < h) { // clip on left, leave 10 pixels to spare
                x = h - (linethick + 10) / pps;
              }
              if (x1 > h1) { // clip on right, leave 10 pixels to spare
                x1 = h1 + (linethick + 10) / pps;
              }
              dc.DrawRectangle(TIME_TO_X(x), y, int((x1 - x) * pps + 0.5), y1 - y + 1);
            } else if (shape == triangle) {
              wxPoint points[3];
              points[0].x = TIME_TO_X(x);
              CLIP(points[0].x);
              points[0].y = y;
              points[1].x = TIME_TO_X(LookupRealAttribute(note, x1r, note->pitch));
              CLIP(points[1].x);
              points[1].y = y1;
              points[2].x = TIME_TO_X(LookupRealAttribute(note, x2r, note->time));
              CLIP(points[2].x);
              points[2].y = PITCH_TO_Y(LookupRealAttribute(note, y2r, note->pitch), bottom);
              dc.DrawPolygon(3, points);
            } else if (shape == polygon) {
              wxPoint points[20]; // upper bound of 20 sides
              points[0].x = TIME_TO_X(x);
              CLIP(points[0].x);
              points[0].y = y;
              points[1].x = TIME_TO_X(LookupRealAttribute(note, x1r, note->time));
              CLIP(points[1].x);
              points[1].y = y1;
              points[2].x = TIME_TO_X(LookupRealAttribute(note, x2r, note->time));
              CLIP(points[2].x);
              points[2].y = PITCH_TO_Y(LookupRealAttribute(note, y2r, note->pitch), bottom);
              int n = 3;
              while (n < 20) {
                char name[8];
                sprintf(name, "x%dr", n);
                char *attr = symbol_table.insert_string(name);
                double xn = LookupRealAttribute(note, attr, -1000000.0);
                if (xn == -1000000.0) break;
                points[n].x = TIME_TO_X(xn);
                CLIP(points[n].x);
                sprintf(name, "y%dr", n - 1);
                attr = symbol_table.insert_string(name);
                double yn = LookupRealAttribute(note, attr, -1000000.0);
                if (yn == -1000000.0) break;
                points[n].y = PITCH_TO_Y(yn, bottom);
                n++;
              }
              dc.DrawPolygon(n, points);
            } else if (shape == oval) {
              int ix = TIME_TO_X(x);
              CLIP(ix);
              int ix1 = int((x1 - x) * pps + 0.5);
              if (ix1 > CLIP_MAX * 2) ix1 = CLIP_MAX * 2; // CLIP a width
              dc.DrawEllipse(ix, y, ix1, y1 - y + 1);
            } else if (shape == text) {
              if (linecolor != -1)
                dc.SetTextForeground(wxColour(RED(linecolor), 
                                              GREEN(linecolor),
                                              BLUE(linecolor)));
              // if no color specified, copy color from brush
              else dc.SetTextForeground(dc.GetBrush().GetColour());

              // This seems to have no effect, so I commented it out. -RBD
              //if (fillcolor != -1)
              //  dc.SetTextBackground(wxColour(RED(fillcolor), 
              //                                GREEN(fillcolor),
              //                                BLUE(fillcolor)));
              //// if no color specified, copy color from brush
              //else dc.SetTextBackground(dc.GetPen().GetColour());

              char *font = LookupAtomAttribute(note, fonta, NULL);
              char *weight = LookupAtomAttribute(note, weighta, NULL);
              int size = LookupIntAttribute(note, sizei, 8);
              const char *justify = LookupStringAttribute(note, justifys, "ld");
              wxFont wxfont;
              wxfont.SetFamily(font == roman ? wxROMAN : 
                                (font == swiss ? wxSWISS :
                                  (font == modern ? wxMODERN : wxDEFAULT)));
              wxfont.SetStyle(wxNORMAL);
              wxfont.SetWeight(weight == bold ? wxBOLD : wxNORMAL);
              wxfont.SetPointSize(size);
              dc.SetFont(wxfont);

              // now do justification
              const char *s = LookupStringAttribute(note, texts, "");
              #ifdef __WXMAC__
		      long textWidth, textHeight;
              #else
		      int textWidth, textHeight;
              #endif
		      dc.GetTextExtent(s, &textWidth, &textHeight);
              long hoffset = 0;
              long voffset = -textHeight; // default should be baseline of text

              if (strlen(justify) != 2) justify = "ld";

              if (justify[0] == 'c') hoffset = -(textWidth/2);
              else if (justify[0] == 'r') hoffset = -textWidth;

              if (justify[1] == 't') voffset = 0;
              else if (justify[1] == 'c') voffset = -(textHeight/2);
              else if (justify[1] == 'b') voffset = -textHeight;
              if (fillflag) {
                // It should be possible to do this with background color,
                // but maybe because of the transfer mode, no background is
                // drawn. To fix this, just draw a rectangle:
                dc.SetPen(wxPen(wxColour(RED(fillcolor), 
                                         GREEN(fillcolor),
                                         BLUE(fillcolor)),
                                1, wxSOLID));
                dc.DrawRectangle(TIME_TO_X(x) + hoffset, y + voffset,
                                 textWidth, textHeight);
              }
              dc.DrawText(s, TIME_TO_X(x) + hoffset, y + voffset);
            }
          }
        }
      }
    }

  }
  dc.DestroyClippingRegion();
}

void TrackArtist::DrawLabelTrack(LabelTrack *track,
                                 wxDC & dc, wxRect & r,
                                 ViewInfo * viewInfo)
{
   double sel0 = viewInfo->sel0;
   double sel1 = viewInfo->sel1;
   
   if (!track->GetSelected())
      sel0 = sel1 = 0.0;
   
   track->Draw(dc, r, viewInfo->h, viewInfo->zoom, sel0, sel1);
}

void TrackArtist::DrawTimeTrack(TimeTrack *track,
                                wxDC & dc, wxRect & r,
                                ViewInfo * viewInfo)
{
   track->Draw(dc, r, viewInfo->h, viewInfo->zoom);
   wxRect envRect = r;
   envRect.height -= 2;
   track->GetEnvelope()->Draw(dc, envRect, viewInfo->h, viewInfo->zoom, false);
}

