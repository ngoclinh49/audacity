/**********************************************************************

  Audacity: A Digital Audio Editor

  WaveTrack.cpp

  Dominic Mazzoni

*******************************************************************//**

\class WaveTrack
\brief A Track that contains audio waveform data.

*//****************************************************************//**

\class WaveTrack::Location
\brief Used only by WaveTrack, a special way to hold location that
can accommodate merged regions.

*//****************************************************************//**

\class TrackFactory
\brief Used to create a WaveTrack, or a LabelTrack..  Implementation 
of the functions of this class are dispersed through the different 
Track classes.

*//*******************************************************************/


#include <wx/defs.h>
#include <wx/intl.h>
#include <wx/debug.h>

#include <math.h>

#include "float_cast.h"

#include "WaveTrack.h"

#include "Envelope.h"
#include "Sequence.h"
#include "Spectrum.h"

#include "Project.h"
#include "Internat.h"

#include "AudioIO.h"
#include "Prefs.h"

WaveTrack* TrackFactory::DuplicateWaveTrack(WaveTrack &orig)
{
   return (WaveTrack*)(orig.Duplicate());
}


WaveTrack *TrackFactory::NewWaveTrack(sampleFormat format, double rate)
{
   return new WaveTrack(mDirManager, format, rate);
}

WaveTrack::WaveTrack(DirManager *projDirManager, sampleFormat format, double rate):
   Track(projDirManager)
{
   if (format == (sampleFormat)0) 
   {
      format = GetActiveProject()->GetDefaultFormat();
   }
   if (rate == 0) 
   {
      rate = GetActiveProject()->GetRate();
   }

#ifdef EXPERIMENTAL_SAVE_DEFAULT_VIEW
   gPrefs->Read(wxT("/GUI/DefaultViewMode"), &mDisplay, 0L);
#else //!EXPERIMENTAL_SAVE_DEFAULT_VIEW
   mDisplay = 0; // Move to GUIWaveTrack
#endif //EXPERIMENTAL_SAVE_DEFAULT_VIEW

   mLegacyProjectFileOffset = 0;

   mFormat = format;
   mRate = (int) rate;
   mGain = 1.0;
   mPan = 0.0;
   SetDefaultName(_("Audio Track"));
   SetName(GetDefaultName());
   mDisplayMin = -1.0;
   mDisplayMax = 1.0;
   mDisplayNumLocations = 0;
   mDisplayLocations = NULL;
   mDisplayNumLocationsAllocated = 0;
}

WaveTrack::WaveTrack(WaveTrack &orig):
   Track(orig)
{
#ifdef EXPERIMENTAL_SAVE_DEFAULT_VIEW
   gPrefs->Read(wxT("/GUI/DefaultViewMode"), &mDisplay, 0L);
#else //!EXPERIMENTAL_SAVE_DEFAULT_VIEW
   mDisplay = 0; // Move to GUIWaveTrack
#endif //EXPERIMENTAL_SAVE_DEFAULT_VIEW

   mLegacyProjectFileOffset = 0;

   Init(orig);

   for (WaveClipList::Node *node = orig.mClips.GetFirst(); node; node = node->GetNext())
      mClips.Append(new WaveClip(*node->GetData(), mDirManager));
}

// Copy the track metadata but not the contents.
void WaveTrack::Init(const WaveTrack &orig)
{
   Track::Init(orig);
   mFormat = orig.mFormat;
   mRate = orig.mRate;
   mGain = orig.mGain;
   mPan = orig.mPan;
   SetDefaultName(orig.GetDefaultName());
   SetName(orig.GetName());
   mDisplay = orig.mDisplay;
   mDisplayMin = orig.mDisplayMin;
   mDisplayMax = orig.mDisplayMax;
   mDisplayNumLocations = 0;
   mDisplayLocations = NULL;
   mDisplayNumLocationsAllocated = 0;
}

void WaveTrack::Merge(const Track &orig)
{
   if (orig.GetKind() == Wave)
      mDisplay = ((WaveTrack &)orig).mDisplay;
   Track::Merge(orig);
}

WaveTrack::~WaveTrack()
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
      delete it->GetData();
   mClips.Clear();
   if (mDisplayLocations)
      delete mDisplayLocations;
}

double WaveTrack::GetOffset()
{
   return GetStartTime();
}

void WaveTrack::SetOffset(double o)
{
   double delta = o - GetOffset();

   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip* clip = it->GetData();
      clip->SetOffset(clip->GetOffset() + delta);
   }

   mOffset = o;
}

void WaveTrack::GetDisplayBounds(float *min, float *max)
{
   *min = mDisplayMin;
   *max = mDisplayMax;
}

void WaveTrack::SetDisplayBounds(float min, float max)
{
   mDisplayMin = min;
   mDisplayMax = max;
}

Track *WaveTrack::Duplicate()
{
   return new WaveTrack(*this);
}

double WaveTrack::GetRate() const
{
   return mRate;
}

void WaveTrack::SetRate(double newRate)
{
   mRate = (int) newRate;
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
      it->GetData()->SetRate((int) newRate);
}

float WaveTrack::GetGain() const
{
   return mGain;
}

void WaveTrack::SetGain(float newGain)
{
   mGain = newGain;
}

float WaveTrack::GetPan() const
{
   return mPan;
}

void WaveTrack::SetPan(float newPan)
{
   if (newPan > 1.0)
      mPan = 1.0;
   else if (newPan < -1.0)
      mPan = -1.0;
   else
      mPan = newPan;
}

float WaveTrack::GetChannelGain(int channel)
{
   float left = 1.0;
   float right = 1.0;

   if (mPan < 0)
      right = (mPan + 1.0);
   else if (mPan > 0)
      left = 1.0 - mPan;

   if ((channel%2) == 0)
      return left*mGain;
   else
      return right*mGain;
}

bool WaveTrack::ConvertToSampleFormat(sampleFormat format)
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
      it->GetData()->ConvertToSampleFormat(format);
   mFormat = format;

   return true;
}

bool WaveTrack::IsEmpty(double t0, double t1)
{
   WaveClipList::Node* it;

   //printf("Searching for overlap in %.6f...%.6f\n", t0, t1);
   for (it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip *clip = it->GetData();

      if (clip->GetStartTime() < t1-(1.0/mRate) &&
          clip->GetEndTime()-(1.0/mRate) > t0) {
         //printf("Overlapping clip: %.6f...%.6f\n",
         //       clip->GetStartTime(),
         //       clip->GetEndTime());
         // We found a clip that overlaps this region
         return false;
      }
   }
   //printf("No overlap found\n");

   // Otherwise, no clips overlap this region
   return true;
}

bool WaveTrack::Cut(double t0, double t1, Track **dest)
{
   if (t1 < t0)
      return false;

   // Cut is the same as 'Copy', then 'Delete'
   if (!Copy(t0, t1, dest))
      return false;
   return Clear(t0, t1);
}

bool WaveTrack::SplitCut(double t0, double t1, Track **dest)
{
   if (t1 < t0)
      return false;

   // SplitCut is the same as 'Copy', then 'SplitDelete'
   if (!Copy(t0, t1, dest))
      return false;
   return SplitDelete(t0, t1);
}

bool WaveTrack::CutAndAddCutLine(double t0, double t1, Track **dest)
{
   if (t1 < t0)
      return false;

   // Cut is the same as 'Copy', then 'Delete'
   if (!Copy(t0, t1, dest))
      return false;
   return ClearAndAddCutLine(t0, t1);
}



//Trim trims within a clip, rather than trimming everything.
//If a bound is outside a clip, it trims everything.
bool WaveTrack::Trim (double t0, double t1)
{
   bool inside0 = false;
   bool inside1 = false;
   //Keeps track of the offset of the first clip greater than
   // the left selection t0.
   double firstGreaterOffset = -1;

   WaveClipList::Node * it;
   for(it = GetClipIterator(); it; it = it->GetNext())
      {
            
         WaveClip * clip = it->GetData();

         //Find the first clip greater than the offset.
         //If we end up clipping the entire track, this is useful.
         if(firstGreaterOffset < 0 && 
            clip->GetStartTime() >= t0)
            firstGreaterOffset = clip->GetStartTime();

         if(t1 > clip->GetStartTime() && t1 < clip->GetEndTime())
            {
               if (!clip->Clear(t1,clip->GetEndTime()))
                  return false;
               inside1 = true;
            }

         if(t0 > clip->GetStartTime() && t0 < clip->GetEndTime())
            {
               if (!clip->Clear(clip->GetStartTime(),t0))
                  return false;
               clip->SetOffset(t0);
               inside0 = true;
            }
      }

   //if inside0 is false, then the left selector was between
   //clips, so delete everything to its left.
   if(false == inside1)
      {
         if (!Clear(t1,GetEndTime()))
            return false;
      }

   if(false == inside0)
      {
         if (!Clear(0,t0))
            return false;
         //Reset the track offset to be at the point of the first remaining clip. 
         SetOffset(firstGreaterOffset );
      }
   
   return true;
}




bool WaveTrack::Copy(double t0, double t1, Track **dest)
{
   *dest = NULL;

   if (t1 <= t0)
      return false;

   WaveTrack *newTrack = new WaveTrack(mDirManager);

   newTrack->Init(*this);

   WaveClipList::Node* it;
   
   for (it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip *clip = it->GetData();

      if (t0 <= clip->GetStartTime() && t1 >= clip->GetEndTime())
      {
         // Whole clip is in copy region
         //printf("copy: clip %i is in copy region\n", (int)clip);
         
         WaveClip *newClip = new WaveClip(*clip, mDirManager);
         newClip->RemoveAllCutLines();
         newClip->Offset(-t0);
         newTrack->mClips.Append(newClip);
      } else
      if (t1 > clip->GetStartTime() && t0 < clip->GetEndTime())
      {
         // Clip is affected by command
         //printf("copy: clip %i is affected by command\n", (int)clip);
         
         WaveClip *newClip = new WaveClip(*clip, mDirManager);
         newClip->RemoveAllCutLines();
         double clip_t0 = t0;
         double clip_t1 = t1;
         if (clip_t0 < clip->GetStartTime())
            clip_t0 = clip->GetStartTime();
         if (clip_t1 > clip->GetEndTime())
            clip_t1 = clip->GetEndTime();

         //printf("copy: clip_t0=%f, clip_t1=%f\n", clip_t0, clip_t1);

         newClip->Offset(-t0);
         if (newClip->GetOffset() < 0)
            newClip->SetOffset(0);

         //printf("copy: clip offset is now %f\n", newClip->GetOffset());
            
         if (!newClip->CreateFromCopy(clip_t0, clip_t1, clip))
         {
            //printf("paste: CreateFromCopy(%f, %f, %i) returns false, quitting\n",
            //   clip_t0, clip_t1, (int)clip);
            // JKC: July 2007, previously we did 'return false' here which
            // could leave *dest undefined.
            // I think this is dealing with clips that don't have any sequence content
            // i.e. we don't copy cut lines and such - anyone like to explain more?
            delete newClip;
         }
         else
         {
            newTrack->mClips.Append(newClip);
         }
      }
   }

   *dest = newTrack;

   return true;
}

bool WaveTrack::Paste(double t0, Track *src)
{
   bool editClipCanMove = true;
   gPrefs->Read(wxT("/GUI/EditClipCanMove"), &editClipCanMove);
   
   //printf("paste: entering WaveTrack::Paste\n");
   
   // JKC Added...
   if( src == NULL )
      return false;

   if (src->GetKind() != Track::Wave)
      return false;
      
   //printf("paste: we have a wave track\n");

   WaveTrack* other = (WaveTrack*)src;
   
   //
   // Pasting is a bit complicated, because with the existence of multiclip mode,
   // we must guess the behaviour the user wants.
   //
   // Currently, two modes are implemented:
   //
   // - If a single clip should be pasted, and it should be pasted inside another
   //   clip, no new clips are generated. The audio is simply inserted.
   //   This resembles the old (pre-multiclip support) behaviour. However, if
   //   the clip is pasted outside of any clip, a new clip is generated. This is
   //   the only behaviour which is different to what was done before, but it
   //   shouldn't confuse users too much.
   //
   // - If multiple clips should be pasted, these are always pasted as single
   //   clips, and the current clip is splitted, when necessary. This may seem
   //   strange at first, but it probably is better than trying to auto-merge
   //   anything. The user can still merge the clips by hand (which should be
   //   a simple command reachable by a hotkey or single mouse click).
   //

   if (other->GetNumClips() == 0)
      return false;

   //printf("paste: we have at least one clip\n");

   double insertDuration = other->GetEndTime();
   WaveClipList::Node* it;

   //printf("Check if we need to make room for the pasted data\n");
   
   // Make room for the pasted data, unless the space being pasted in is empty of
   // any clips
   if (!IsEmpty(t0, t0+insertDuration-1.0/mRate) && editClipCanMove) {
      if (other->GetNumClips() > 1) {
         // We need to insert multiple clips, so split the current clip and
         // move everything to the right, then try to paste again
         Track *tmp = NULL;
         Cut(t0, GetEndTime()+1.0/mRate, &tmp);
         Paste(t0 + insertDuration, tmp);
         delete tmp;
      } else
      {
         // We only need to insert one single clip, so just move all clips
         // to the right of the paste point out of the way
         for (it=GetClipIterator(); it; it=it->GetNext())
         {
            WaveClip* clip = it->GetData();
            if (clip->GetStartTime() > t0-(1.0/mRate))
               clip->Offset(insertDuration);
         }
      }
   }

   if (other->GetNumClips() == 1)
   {
      // Single clip mode
      // printf("paste: checking for single clip mode!\n");
      
      WaveClip *insideClip = NULL;

      for (it=GetClipIterator(); it; it=it->GetNext())
      {
         WaveClip *clip = it->GetData();

         // The 1.0/mRate is the time for one sample - kind of a fudge factor,
         // because an overlap of less than a sample should not trigger
         // traditional behaviour.

         if (editClipCanMove)
         {
            if (t0+src->GetEndTime()-1.0/mRate > clip->GetStartTime() &&
                t0 < clip->GetEndTime() - 1.0/mRate)
            {
               //printf("t0=%.6f: inside clip is %.6f ... %.6f\n",
               //       t0, clip->GetStartTime(), clip->GetEndTime());
               insideClip = clip;
               break;
            }
         } else
         {
            if (t0 >= clip->GetStartTime() && t0 < clip->GetEndTime())
            {
               insideClip = clip;
               break;
            }
         }
      }

      if (insideClip)
      {
         // Exhibit traditional behaviour
         //printf("paste: traditional behaviour\n");
         if (!editClipCanMove)
         {
            // We did not move other clips out of the way already, so
            // check if we can paste without having to move other clips
            for (it=GetClipIterator(); it; it=it->GetNext())
            {
               WaveClip *clip = it->GetData();
               
               if (clip->GetStartTime() > insideClip->GetStartTime() &&
                   insideClip->GetEndTime() + insertDuration >
                                                      clip->GetStartTime())
               {
                  wxMessageBox(
                     _("There is not enough room available to paste the selection"),
                     _("Error"), wxICON_STOP);
                  return false;
               }
            }
         }
         
         return insideClip->Paste(t0, other->GetClipByIndex(0));
      }

      // Just fall through and exhibit new behaviour
   }

   // Insert new clips
   //printf("paste: multi clip mode!\n");

   if (!editClipCanMove && !IsEmpty(t0, t0+insertDuration-1.0/mRate))
   {
      wxMessageBox(
         _("There is not enough room available to paste the selection"),
         _("Error"), wxICON_STOP);
      return false;
   }

   for (it=other->GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip* clip = it->GetData();

      WaveClip* newClip = new WaveClip(*clip, mDirManager);
      newClip->Resample(mRate);
      newClip->Offset(t0);
      newClip->MarkChanged();
      mClips.Append(newClip);
   }
   return true;
}

bool WaveTrack::Clear(double t0, double t1)
{
   bool addCutLines = false;
   bool split = false;
   return HandleClear(t0, t1, addCutLines, split);
}

bool WaveTrack::ClearAndAddCutLine(double t0, double t1)
{
   bool addCutLines = true;
   bool split = false;
   return HandleClear(t0, t1, addCutLines, split);
}

bool WaveTrack::SplitDelete(double t0, double t1)
{
   bool addCutLines = false;
   bool split = true;
   return HandleClear(t0, t1, addCutLines, split);
}

bool WaveTrack::HandleClear(double t0, double t1,
                            bool addCutLines, bool split)
{
   if (t1 < t0)
      return false;

   bool editClipCanMove = true;
   gPrefs->Read(wxT("/GUI/EditClipCanMove"), &editClipCanMove);

   WaveClipList::Node* it;
   WaveClipList clipsToDelete;
   WaveClipList clipsToAdd;
   
   // We only add cut lines when deleting in the middle of a single clip
   // The cut line code is not really prepared to handle other situations
   if (addCutLines)
   {
      for (it=GetClipIterator(); it; it=it->GetNext())
      {
         WaveClip *clip = it->GetData();
         
         if (t1 > clip->GetStartTime() && t0 < clip->GetEndTime() &&
             (t0 + 1.0/mRate < clip->GetStartTime() ||
              t1 - 1.0/mRate > clip->GetEndTime()))
         {
            addCutLines = false;
            break;
         }
      }
   }   

   for (it=GetClipIterator(); it; it=it->GetNext())   // main loop through clips
   {
      WaveClip *clip = it->GetData();

      if (t0 <= clip->GetStartTime() && t1 >= clip->GetEndTime())
      {
         // Whole clip must be deleted - remember this
         clipsToDelete.Append(clip);
      } else
      if (t1 > clip->GetStartTime() && t0 < clip->GetEndTime())
      {
         // Clip data is affected by command
         if (addCutLines)
         {
            if (!clip->ClearAndAddCutLine(t0,t1))
               return false;
         } else
         {
            if (split) {
               // Three cases:

               if (t0 <= clip->GetStartTime()) {
                  // Delete from the left edge
                  clip->Clear(clip->GetStartTime(), t1);
                  clip->Offset(t1-clip->GetStartTime());
               } else
               if (t1 >= clip->GetEndTime()) {
                  // Delete to right edge
                  clip->Clear(t0, clip->GetEndTime());
               } else
               {
                  // Delete in the middle of the clip...we actually create two
                  // new clips out of the left and right halves...

                  WaveClip *left = new WaveClip(*clip, mDirManager);
                  left->Clear(t0, clip->GetEndTime());
                  clipsToAdd.Append(left);

                  WaveClip *right = new WaveClip(*clip, mDirManager);
                  right->Clear(clip->GetStartTime(), t1);
                  right->Offset(t1-clip->GetStartTime());
                  clipsToAdd.Append(right);

                  clipsToDelete.Append(clip);
               }
            }
            else {
               // frame clip envelope points here
               // clip->Clear keeps points < t0 and >= t1 via Envelope::CollapseRegion
               double val;
               val = clip->GetEnvelope()->GetValue(t0);
               clip->GetEnvelope()->Insert(t0 - clip->GetOffset() - 1.0/clip->GetRate(), val);
               val = clip->GetEnvelope()->GetValue(t1);
               clip->GetEnvelope()->Insert(t1 - clip->GetOffset(), val);

               if (!clip->Clear(t0,t1))
                  return false;
            }
         }
      } else
      if (clip->GetStartTime() >= t1)
      {
         // Clip is "behind" the region -- offset it unless we're splitting
         // or we're using the "don't move other clips" mode
         if (!split && editClipCanMove)
            clip->Offset(-(t1-t0));
      }
   }

   for (it=clipsToDelete.GetFirst(); it; it=it->GetNext())
   {
      mClips.DeleteObject(it->GetData());
      delete it->GetData();
   }

   for (it=clipsToAdd.GetFirst(); it; it=it->GetNext())
   {
      mClips.Append(it->GetData());
   }

   return true;
}

bool WaveTrack::Silence(double t0, double t1)
{
   if (t1 < t0)
      return false;

   longSampleCount start = (longSampleCount)floor(t0 * mRate + 0.5);
   longSampleCount len = (longSampleCount)floor(t1 * mRate + 0.5) - start;
   bool result = true;

   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip *clip = it->GetData();

      longSampleCount clipStart = clip->GetStartSample();
      longSampleCount clipEnd = clip->GetEndSample();

      if (clipEnd > start && clipStart < start+len)
      {
         // Clip sample region and Get/Put sample region overlap
         sampleCount samplesToCopy = start+len - clipStart;
         if (samplesToCopy > clip->GetNumSamples())
            samplesToCopy = clip->GetNumSamples();
         longSampleCount inclipDelta = 0;
         longSampleCount startDelta = clipStart - start;
         if (startDelta < 0)
         {
            inclipDelta = -startDelta; // make positive value
            samplesToCopy -= inclipDelta;
            startDelta = 0;
         }

         if (!clip->GetSequence()->SetSilence(inclipDelta, samplesToCopy))
         {
            wxASSERT(false); // should always work
            return false;
         }
         clip->MarkChanged();
      }
   }

   return result;
}

bool WaveTrack::InsertSilence(double t, double len)
{
   if (len <= 0)
      return false;

   if (mClips.IsEmpty())
   {
      // Special case if there is no clip yet
      WaveClip* clip = CreateClip();
      return clip->InsertSilence(0, len);
   }

   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip *clip = it->GetData();
      if (clip->GetStartTime() > t)
         clip->Offset(len);
      else if (clip->GetEndTime() > t)
      {
         return clip->InsertSilence(t, len);
      }
   }

   return true;
}

//Performs the opposite of Join
//Analyses selected region for possible Joined clips and disjoins them
bool WaveTrack::Disjoin(double t0, double t1)
{
   sampleCount minSamples = TimeToLongSamples( WAVETRACK_MERGE_POINT_TOLERANCE );
   sampleCount maxAtOnce = 1048576;
   float *buffer = new float[ maxAtOnce ];
   Regions regions;
   
   wxBusyCursor busy;
   
   for( WaveClipList::Node *it = GetClipIterator(); it; it = it->GetNext() )
   {
      WaveClip *clip = it->GetData();

      double startTime = clip->GetStartTime();
      double endTime = clip->GetEndTime();

      if( endTime < t0 || startTime > t1 )
         continue;

      if( t0 > startTime )
         startTime = t0;
      if( t1 < endTime )
         endTime = t1;
      
      //simply look for a sequence of zeroes and if the sequence
      //is greater than minimum number, split-delete the region
      
      longSampleCount seqStart = -1;
      longSampleCount start, end;
      clip->TimeToSamplesClip( startTime, &start );
      clip->TimeToSamplesClip( endTime, &end );
       
      longSampleCount len = ( end - start );
      for( longSampleCount done = 0; done < len; done += maxAtOnce )
      {
         sampleCount numSamples = maxAtOnce;
         if( done + maxAtOnce > len )
            numSamples = len - done;
           
         clip->GetSamples( ( samplePtr )buffer, floatSample, start + done, 
               numSamples );
         for( sampleCount i = 0; i < numSamples; i++ )
         {
            longSampleCount curSamplePos = start + done + i;

            //start a new sequence
            if( buffer[ i ] == 0.0 && seqStart == -1 )
               seqStart = curSamplePos;
            else if( buffer[ i ] != 0.0 || curSamplePos == end - 1 )
            {
               if( seqStart != -1 )
               {
                  longSampleCount seqEnd;
                  
                  //consider the end case, where selection ends in zeroes
                  if( curSamplePos == end - 1 && buffer[ i ] == 0.0 )
                     seqEnd = end - 1;
                  else
                     seqEnd = curSamplePos - 1;
                  if( seqEnd - seqStart + 1 > minSamples )
                  {
                     Region *region = new Region;
                     region->start = seqStart / GetRate() + clip->GetStartTime();
                     region->end = seqEnd / GetRate() + clip->GetStartTime();
                     regions.Add( region );
                  }
                  seqStart = -1;
               }
            }
         }
      }
   }

   for( unsigned int i = 0; i < regions.GetCount(); i++ )
   {
      SplitDelete( regions.Item( i )->start, regions.Item( i )->end );
      delete regions.Item( i );
   }
   
   delete[] buffer;
   return true;
}

bool WaveTrack::Join(double t0, double t1)
{
   // Merge all WaveClips overlapping selection into one

   WaveClipList::Node* it;
   WaveClipList clipsToDelete;
   WaveClip *newClip;

   for (it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip *clip = it->GetData();   

      if (clip->GetStartTime() < t1-(1.0/mRate) &&
          clip->GetEndTime()-(1.0/mRate) > t0) {

         // Put in sorted order
         int i;
         for(i=0; i<(int)clipsToDelete.GetCount(); i++)
            if (clipsToDelete[i]->GetStartTime() > clip->GetStartTime())
               break;
         //printf("Insert clip %.6f at position %d\n", clip->GetStartTime(), i);
         clipsToDelete.Insert(i, clip);
      }
   }

   //if there are no clips to delete, nothing to do
   if( clipsToDelete.GetCount() == 0 )
      return true;
   
   newClip = CreateClip();
   double t = clipsToDelete[0]->GetOffset();
   newClip->SetOffset(t);
   for(it=clipsToDelete.GetFirst(); it; it=it->GetNext()) 
   {
      WaveClip *clip = it->GetData();

      //printf("t=%.6f adding clip (offset %.6f, %.6f ... %.6f)\n",
      //       t, clip->GetOffset(), clip->GetStartTime(), clip->GetEndTime());

      if (clip->GetOffset() - t > (1.0 / mRate)) {
         double addedSilence = (clip->GetOffset() - t);
         //printf("Adding %.6f seconds of silence\n");
         newClip->InsertSilence(t, addedSilence);
         t += addedSilence;
      }

      //printf("Pasting at %.6f\n", t);
      newClip->Paste(t, clip);
      t = newClip->GetEndTime();      

      mClips.DeleteObject(clip);
      delete clip;
   }

   return true;
}

bool WaveTrack::Append(samplePtr buffer, sampleFormat format,
                       sampleCount len, unsigned int stride /* = 1 */,
                       XMLWriter *blockFileLog /* = NULL */)
{
   return GetLastOrCreateClip()->Append(buffer, format, len, stride,
                                        blockFileLog);
}

bool WaveTrack::AppendAlias(wxString fName, sampleCount start,
                            sampleCount len, int channel)
{
   return GetLastOrCreateClip()->AppendAlias(fName, start, len, channel);
}

sampleCount WaveTrack::GetBestBlockSize(longSampleCount s)
{
   sampleCount bestBlockSize = GetMaxBlockSize();

   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip* clip = it->GetData();
      longSampleCount startSample = (longSampleCount)floor(clip->GetStartTime()*mRate + 0.5);
      longSampleCount endSample = startSample + clip->GetNumSamples();
      if (s >= startSample && s < endSample)
      {
         bestBlockSize = clip->GetSequence()->GetMaxBlockSize();
         break;
      }
   }

   return bestBlockSize;
}

sampleCount WaveTrack::GetMaxBlockSize()
{
   int maxblocksize = 0;
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip* clip = it->GetData();
      if (clip->GetSequence()->GetMaxBlockSize() > maxblocksize)
         maxblocksize = clip->GetSequence()->GetMaxBlockSize();
   }

   if (maxblocksize == 0)
   {
      // We really need the maximum block size, so create a
      // temporary sequence to get it.
      Sequence *tempseq = new Sequence(mDirManager, mFormat);
      maxblocksize = tempseq->GetMaxBlockSize();
      delete tempseq;
   }

   wxASSERT(maxblocksize > 0);

   return maxblocksize;
}

sampleCount WaveTrack::GetIdealBlockSize()
{
   return GetLastOrCreateClip()->GetSequence()->GetIdealBlockSize();
}

bool WaveTrack::Flush()
{
   return GetLastOrCreateClip()->Flush();
}

bool WaveTrack::HandleXMLTag(const wxChar *tag, const wxChar **attrs)
{
   if (!wxStrcmp(tag, wxT("wavetrack"))) {
      double dblValue;
      long nValue;
      while(*attrs) {
         const wxChar *attr = *attrs++;
         const wxChar *value = *attrs++;
         
         if (!value)
            break;
         
         const wxString strValue = value;
         if (!wxStrcmp(attr, wxT("rate")))
         {
            // mRate is an int, but "rate" in the project file is a float.
            if (!XMLValueChecker::IsGoodString(strValue) || 
                  !Internat::CompatibleToDouble(strValue, &dblValue) ||
                  (dblValue < 100.0) || (dblValue > 100000.0)) // same bounds as ImportRawDialog::OnOK
               return false;
            mRate = lrint(dblValue);
         }
         else if (!wxStrcmp(attr, wxT("offset")) && 
                  XMLValueChecker::IsGoodString(strValue) && 
                  Internat::CompatibleToDouble(strValue, &dblValue))
         {
            // Offset is only relevant for legacy project files. The value
            // is cached until the actual WaveClip containing the legacy
            // track is created.
            mLegacyProjectFileOffset = dblValue;
         }
         else if (!wxStrcmp(attr, wxT("gain")) && 
                  XMLValueChecker::IsGoodString(strValue) && 
                  Internat::CompatibleToDouble(strValue, &dblValue))
            mGain = dblValue;
         else if (!wxStrcmp(attr, wxT("pan")) && 
                  XMLValueChecker::IsGoodString(strValue) && 
                  Internat::CompatibleToDouble(strValue, &dblValue) && 
                  (dblValue >= -1.0) && (dblValue <= 1.0))
            mPan = dblValue;
         else if (!wxStrcmp(attr, wxT("name")) && XMLValueChecker::IsGoodString(strValue))
            mName = strValue;
         else if (!wxStrcmp(attr, wxT("channel")))
         {
            if (!XMLValueChecker::IsGoodInt(strValue) || !strValue.ToLong(&nValue) || 
                  !XMLValueChecker::IsValidChannel(nValue))
               return false;
            mChannel = nValue;
         }
         else if (!wxStrcmp(attr, wxT("linked")) && 
                  XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue))
            mLinked = (nValue != 0);
         
      } // while
      return true;
   }

   return false;
}

void WaveTrack::HandleXMLEndTag(const wxChar *tag)
{
   // In case we opened a pre-multiclip project, we need to
   // simulate closing the waveclip tag.
   GetLastOrCreateClip()->HandleXMLEndTag( wxT("waveclip") );
}

XMLTagHandler *WaveTrack::HandleXMLChild(const wxChar *tag)
{
   //
   // This is legacy code (1.2 and previous) and is not called for new projects!
   //
   if (!wxStrcmp(tag, wxT("sequence")) || !wxStrcmp(tag, wxT("envelope")))
   {
      // This is a legacy project, so set the cached offset
      GetLastOrCreateClip()->SetOffset(mLegacyProjectFileOffset);
 
      // Legacy project file tracks are imported as one single wave clip
      if (!wxStrcmp(tag, wxT("sequence")))
         return GetLastOrCreateClip()->GetSequence();
      else if (!wxStrcmp(tag, wxT("envelope")))
         return GetLastOrCreateClip()->GetEnvelope();
   }
   
   // JKC... for 1.1.0, one step better than what we had, but still badly broken.
   //If we see a waveblock at this level, we'd better generate a sequence.
   if( !wxStrcmp( tag, wxT("waveblock" )))
   {
      // This is a legacy project, so set the cached offset
      GetLastOrCreateClip()->SetOffset(mLegacyProjectFileOffset);
      Sequence *pSeq = GetLastOrCreateClip()->GetSequence();
      return pSeq;
   }

   //
   // This is for the new file format (post-1.2)
   //
   if (!wxStrcmp(tag, wxT("waveclip")))
      return CreateClip();
   else
      return NULL;
}

void WaveTrack::WriteXML(XMLWriter &xmlFile)
{
   xmlFile.StartTag(wxT("wavetrack"));
   xmlFile.WriteAttr(wxT("name"), mName);
   xmlFile.WriteAttr(wxT("channel"), mChannel);
   xmlFile.WriteAttr(wxT("linked"), mLinked);
   xmlFile.WriteAttr(wxT("offset"), mOffset, 8);
   xmlFile.WriteAttr(wxT("rate"), mRate);
   xmlFile.WriteAttr(wxT("gain"), (double)mGain);
   xmlFile.WriteAttr(wxT("pan"), (double)mPan);
   
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      it->GetData()->WriteXML(xmlFile);
   }

   xmlFile.EndTag(wxT("wavetrack"));
}

bool WaveTrack::GetErrorOpening()
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
      if (it->GetData()->GetSequence()->GetErrorOpening())
         return true;

   return false;
}

bool WaveTrack::Lock()
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
      it->GetData()->Lock();

   return true;
}

bool WaveTrack::Unlock()
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
      it->GetData()->Unlock();

   return true;
}

longSampleCount WaveTrack::TimeToLongSamples(double t0)
{
   return (longSampleCount)floor(t0 * mRate + 0.5);
}

double WaveTrack::LongSamplesToTime(longSampleCount pos)
{
   return ((double)pos) / mRate;
}

double WaveTrack::GetStartTime()
{
   bool found = false;
   double best = 0.0;

   if (mClips.IsEmpty())
      return 0;

   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
      if (!found)
      {
         found = true;
         best = it->GetData()->GetStartTime();
      } else if (it->GetData()->GetStartTime() < best)
         best = it->GetData()->GetStartTime();

   return best;
}

double WaveTrack::GetEndTime()
{
   bool found = false;
   double best = 0.0;

   if (mClips.IsEmpty())
      return 0;

   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
      if (!found)
      {
         found = true;
         best = it->GetData()->GetEndTime();
      } else if (it->GetData()->GetEndTime() > best)
         best = it->GetData()->GetEndTime();

   return best;
}

//
// Getting/setting samples.  The sample counts here are
// expressed relative to t=0.0 at the track's sample rate.
//

bool WaveTrack::GetMinMax(float *min, float *max,
                          double t0, double t1)
{
   *min = float(0.0);
   *max = float(0.0);

   if (t0 > t1)
      return false;

   if (t0 == t1)
      return true;

   bool result = true;

   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip* clip = it->GetData();

      if (t1 >= clip->GetStartTime() && t0 <= clip->GetEndTime())
      {
         float clipmin, clipmax;
         if (it->GetData()->GetMinMax(&clipmin, &clipmax, t0, t1))
         {
            if (clipmin < *min)
               *min = clipmin;
            if (clipmax > *max)
               *max = clipmax;
         } else
         {
            result = false;
         }
      }
   }

   return result;
}

bool WaveTrack::Get(samplePtr buffer, sampleFormat format,
                    longSampleCount start, sampleCount len)
{
   // Simple optimization: When this buffer is completely contained within one clip,
   // don't clear anything (because we never won't have to). Otherwise, just clear
   // everything to be on the safe side.
   WaveClipList::Node* it;
   
   bool doClear = true;
   for (it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip *clip = it->GetData();
      if (start >= clip->GetStartSample() && start+len <= clip->GetEndSample())
      {
         doClear = false;
         break;
      }
   }
   if (doClear)
      ClearSamples(buffer, format, 0, len);

   for (it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip *clip = it->GetData();

      longSampleCount clipStart = clip->GetStartSample();
      longSampleCount clipEnd = clip->GetEndSample();

      if (clipEnd > start && clipStart < start+len)
      {
         // Clip sample region and Get/Put sample region overlap
         sampleCount samplesToCopy = start+len - clipStart;
         if (samplesToCopy > clip->GetNumSamples())
            samplesToCopy = clip->GetNumSamples();
         longSampleCount inclipDelta = 0;
         longSampleCount startDelta = clipStart - start;
         if (startDelta < 0)
         {
            inclipDelta = -startDelta; // make positive value
            samplesToCopy -= inclipDelta;
            startDelta = 0;
         }

         if (!clip->GetSamples((samplePtr)(((char*)buffer)+startDelta*SAMPLE_SIZE(format)),
                               format, inclipDelta, samplesToCopy))
         {
            wxASSERT(false); // should always work
            return false;
         }
      }
   }

   return true;
}

bool WaveTrack::Set(samplePtr buffer, sampleFormat format,
                    longSampleCount start, sampleCount len)
{
   bool result = true;

   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip *clip = it->GetData();

      longSampleCount clipStart = clip->GetStartSample();
      longSampleCount clipEnd = clip->GetEndSample();

      if (clipEnd > start && clipStart < start+len)
      {
         // Clip sample region and Get/Put sample region overlap
         sampleCount samplesToCopy = start+len - clipStart;
         if (samplesToCopy > clip->GetNumSamples())
            samplesToCopy = clip->GetNumSamples();
         longSampleCount inclipDelta = 0;
         longSampleCount startDelta = clipStart - start;
         if (startDelta < 0)
         {
            inclipDelta = -startDelta; // make positive value
            samplesToCopy -= inclipDelta;
            startDelta = 0;
         }

         if (!clip->SetSamples((samplePtr)(((char*)buffer)+startDelta*SAMPLE_SIZE(format)),
                               format, inclipDelta, samplesToCopy))
         {
            wxASSERT(false); // should always work
            return false;
         }
         clip->MarkChanged();
      }
   }

   return result;
}

void WaveTrack::GetEnvelopeValues(double *buffer, int bufferLen,
                         double t0, double tstep)
{
   memset(buffer, 0, sizeof(double)*bufferLen);

   double startTime = t0;
   double endTime = t0+tstep*bufferLen;

   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip *clip = it->GetData();
      
      if (clip->GetStartTime() < endTime && clip->GetEndTime() > startTime)
      {
         double* rbuf = buffer;
         int rlen = bufferLen;
         double rt0 = t0;

         if (rt0 < clip->GetStartTime())
         {
            int dx = (int) floor((clip->GetStartTime() - rt0) / tstep + 0.5);
            rbuf += dx;
            rlen -= dx;
            rt0 = clip->GetStartTime();
         }

         if (rt0+rlen*tstep > clip->GetEndTime())
         {
            rlen = (int) ((clip->GetEndTime()-rt0) / tstep);
         }

         clip->GetEnvelope()->GetValues(rbuf, rlen, rt0, tstep);
      }
   }
}

WaveClip* WaveTrack::GetClipAtX(int xcoord)
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      wxRect r;
      it->GetData()->GetDisplayRect(&r);
      if (xcoord >= r.x && xcoord < r.x+r.width)
         return it->GetData();
   }

   return NULL;
}

Envelope* WaveTrack::GetEnvelopeAtX(int xcoord)
{
   WaveClip* clip = GetClipAtX(xcoord);
   if (clip)
      return clip->GetEnvelope();
   else
      return NULL;
}

// Search for any active DragPoint on the current track
Envelope* WaveTrack::GetActiveEnvelope(void)
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip* clip = it->GetData();
      Envelope* env = clip->GetEnvelope() ;
      if (env->GetDragPoint() >= 0)
         return env;
   }
   return NULL;
}

Sequence* WaveTrack::GetSequenceAtX(int xcoord)
{
   WaveClip* clip = GetClipAtX(xcoord);
   if (clip)
      return clip->GetSequence();
   else
      return NULL;
}

WaveClip* WaveTrack::CreateClip()
{
   WaveClip* clip = new WaveClip(mDirManager, mFormat, mRate);
   mClips.Append(clip);
   return clip;
}

WaveClip* WaveTrack::GetLastOrCreateClip()
{
   if (mClips.IsEmpty()) {
      WaveClip *clip = CreateClip();
      clip->SetOffset(mOffset);
      return clip;
   }
   else
      return mClips.GetLast()->GetData();
}

int WaveTrack::GetClipIndex(WaveClip* clip)
{
   return mClips.IndexOf(clip);
}

WaveClip* WaveTrack::GetClipByIndex(int index)
{
   if(index < (int)mClips.GetCount())
      return mClips.Item(index)->GetData();
   else
      return NULL;
}

int WaveTrack::GetNumClips() const
{
   return mClips.GetCount();
}

void WaveTrack::MoveClipToTrack(int clipIndex, WaveTrack* dest)
{
   WaveClipList::Node* node = mClips.Item(clipIndex);
   WaveClip* clip = node->GetData();
   mClips.DeleteNode(node);
   dest->mClips.Append(clip);
}

void WaveTrack::MoveClipToTrack(WaveClip *clip, WaveTrack* dest)
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext()) {
      if (it->GetData() == clip) {
         WaveClip* clip = it->GetData();
         mClips.DeleteNode(it);
         dest->mClips.Append(clip);
         return; // JKC iterator is now 'defunct' so better return straight away.
      }
   }
}

bool WaveTrack::CanOffsetClip(WaveClip* clip, double amount,
                              double *allowedAmount /* = NULL */)
{
   if (allowedAmount)
      *allowedAmount = amount;

   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip* c = it->GetData();
      if (c != clip && c->GetStartTime() < clip->GetEndTime()+amount &&
                       c->GetEndTime() > clip->GetStartTime()+amount)
      {
         if (!allowedAmount)
            return false; // clips overlap

         if (amount > 0)
         {
            if (c->GetStartTime()-clip->GetEndTime() < *allowedAmount)
               *allowedAmount = c->GetStartTime()-clip->GetEndTime();
            if (*allowedAmount < 0)
               *allowedAmount = 0;
         } else
         {
            if (c->GetEndTime()-clip->GetStartTime() > *allowedAmount)
               *allowedAmount = c->GetEndTime()-clip->GetStartTime();
            if (*allowedAmount > 0)
               *allowedAmount = 0;
         }
      }
   }

   if (allowedAmount)
   {
      if (*allowedAmount == amount)
         return true;

      // Check if the new calculated amount would not violate
      // any other constraint
      if (!CanOffsetClip(clip, *allowedAmount, NULL)) {
         *allowedAmount = 0; // play safe and don't allow anything
         return false;
      }
      else
         return true;
   } else
      return true;
}

bool WaveTrack::CanInsertClip(WaveClip* clip)
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip* c = it->GetData();
      if (c->GetStartTime() < clip->GetEndTime() && c->GetEndTime() > clip->GetStartTime())
         return false; // clips overlap
   }

   return true;
}

bool WaveTrack::Split( double t0, double t1 )
{
   bool ret = SplitAt( t0 );
   if( ret && t0 != t1 )
      ret = SplitAt( t1 );
   return ret;
}

bool WaveTrack::SplitAt(double t)
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip* c = it->GetData();
      if (t > c->GetStartTime() && t < c->GetEndTime())
      {
         double val;
         t = LongSamplesToTime(TimeToLongSamples(t)); // put t on a sample
         val = c->GetEnvelope()->GetValue(t);
         c->GetEnvelope()->Insert(t - c->GetOffset() - 1.0/c->GetRate(), val);  // frame end points
         c->GetEnvelope()->Insert(t - c->GetOffset(), val);
         WaveClip* newClip = new WaveClip(*c, mDirManager);
         if (!c->Clear(t, c->GetEndTime()))
         {
            delete newClip;
            return false;
         }
         if (!newClip->Clear(c->GetStartTime(), t))
         {
            delete newClip;
            return false;
         }
         longSampleCount here = llrint(floor(((t - c->GetStartTime() - mOffset) * mRate) + 0.5));
         newClip->Offset((double)here/(double)mRate);
         mClips.Append(newClip);
         return true;
      }
   }

   return true;
}

void WaveTrack::UpdateLocationsCache()
{
   unsigned int i;
   WaveClipArray clips;
   
   FillSortedClipArray(clips);
   
   mDisplayNumLocations = 0;
   
   // Count number of display locations
   for (i = 0; i < clips.GetCount(); i++)
   {
      WaveClip* clip = clips.Item(i);
      
      mDisplayNumLocations += clip->GetCutLines()->GetCount();

      if (i > 0 && fabs(clips.Item(i - 1)->GetEndTime() -
                  clip->GetStartTime()) < WAVETRACK_MERGE_POINT_TOLERANCE)
         mDisplayNumLocations++;
   }

   if (mDisplayNumLocations == 0)
      return;

   // Alloc necessary number of display locations
   if (mDisplayNumLocations > mDisplayNumLocationsAllocated)
   {
      // Only realloc, if we need more space than before. Otherwise
      // just use block from before.
      if (mDisplayLocations)
         delete[] mDisplayLocations;
      mDisplayLocations = new Location[mDisplayNumLocations];
      mDisplayNumLocationsAllocated = mDisplayNumLocations;
   }

   // Add all display locations to cache
   int curpos = 0;
   
   for (i = 0; i < clips.GetCount(); i++)
   {
      WaveClip* clip = clips.Item(i);

      WaveClipList* cutlines = clip->GetCutLines();
      for (WaveClipList::Node* it = cutlines->GetFirst(); it;
           it = it->GetNext())
      {
         // Add cut line expander point
         mDisplayLocations[curpos].typ = locationCutLine;
         mDisplayLocations[curpos].pos =
            clip->GetOffset() + it->GetData()->GetOffset();
         curpos++;
      }
      
      if (i > 0)
      {
         WaveClip* previousClip = clips.Item(i - 1);

         if (fabs(previousClip->GetEndTime() - clip->GetStartTime())
                                          < WAVETRACK_MERGE_POINT_TOLERANCE)
         {
            // Add merge point
            mDisplayLocations[curpos].typ = locationMergePoint;
            mDisplayLocations[curpos].pos = clips.Item(i-1)->GetEndTime();
            mDisplayLocations[curpos].clipidx1 = mClips.IndexOf(previousClip);
            mDisplayLocations[curpos].clipidx2 = mClips.IndexOf(clip);
            curpos++;
         }
      }
   }
   
   wxASSERT(curpos == mDisplayNumLocations);
}

// Expand cut line (that is, re-insert audio, then delete audio saved in cut line)
bool WaveTrack::ExpandCutLine(double cutLinePosition, double* cutlineStart,
                              double* cutlineEnd)
{
   bool editClipCanMove = true;
   gPrefs->Read(wxT("/GUI/EditClipCanMove"), &editClipCanMove);

   // Find clip which contains this cut line   
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
   {
      WaveClip* clip = it->GetData();
      double start = 0, end = 0;

      if (clip->FindCutLine(cutLinePosition, &start, &end))
      {
         WaveClipList::Node* it2;
         
         if (!editClipCanMove)
         {
            // We are not allowed to move the other clips, so see if there
            // is enough room to expand the cut line
            for (it2=GetClipIterator(); it2; it2=it2->GetNext())
            {
               WaveClip *clip2 = it2->GetData();
               
               if (clip2->GetStartTime() > clip->GetStartTime() &&
                   clip->GetEndTime() + end - start > clip2->GetStartTime())
               {
                  wxMessageBox(
                     _("There is not enough room available to expand the cut line"),
                     _("Error"), wxICON_STOP);
                  return false;
               }
            }
         }
         
         if (!clip->ExpandCutLine(cutLinePosition))
            return false;

         if (cutlineStart)
            *cutlineStart = start;
         if (cutlineEnd)
            *cutlineEnd = end;

         // Move clips which are to the right of the cut line
         if (editClipCanMove)
         {
            for (it2=GetClipIterator(); it2;
                 it2=it2->GetNext())
            {
               WaveClip* clip2 = it2->GetData();
   
               if (clip2->GetStartTime() > clip->GetStartTime())
                  clip2->Offset(end - start);
            }
         }

         return true;
      }
   }

   return false;
}

bool WaveTrack::RemoveCutLine(double cutLinePosition)
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
      if (it->GetData()->RemoveCutLine(cutLinePosition))
         return true;

   return false;
}

bool WaveTrack::MergeClips(int clipidx1, int clipidx2)
{
   WaveClip* clip1 = GetClipByIndex(clipidx1);
   WaveClip* clip2 = GetClipByIndex(clipidx2);

   if(clip2 == NULL) // could happen if one track of a linked pair had a split and the other didn't
      return false;
   
   // Append data from second clip to first clip
   if (!clip1->Paste(clip1->GetEndTime(), clip2))
      return false;

   // Delete second clip
   mClips.DeleteObject(clip2);
   delete clip2;
   
   return true;
}

bool WaveTrack::Resample(int rate, bool progress)
{
   for (WaveClipList::Node* it=GetClipIterator(); it; it=it->GetNext())
      if (!it->GetData()->Resample(rate, progress))
      {
         // FIXME: The track is now in an inconsistent state since some
         //        clips are resampled and some are not
         return false;
      }
      
   mRate = rate;
   
   return true;
}

static int SortClipArrayCmpFunc(WaveClip** clip1, WaveClip** clip2)
{
   if((*clip1)->GetStartTime() < (*clip2)->GetStartTime())
      return -1;
   else
      return 1;
}

void WaveTrack::FillSortedClipArray(WaveClipArray& clips)
{
   clips.Empty();
   
   for (WaveClipList::Node *it=GetClipIterator(); it; it=it->GetNext())
      clips.Add(it->GetData());
   
   clips.Sort(SortClipArrayCmpFunc);
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
// arch-tag: 8cf4eb04-e9b7-4ca5-acd1-aecf564c11d2

