/**********************************************************************

  Audacity: A Digital Audio Editor

  Sequence.cpp

  Dominic Mazzoni

**********************************************************************/

#include "Audacity.h"

#include <float.h>
#include <math.h>

#include <wx/dynarray.h>
#include <wx/intl.h>

#include "Sequence.h"

#include "BlockFile.h"
#include "DirManager.h"

class SeqBlock {
 public:
   BlockFile * f;

   sampleCount start;
   sampleCount len;
   float       min;
   float       max;
   float       rms;
};

WX_DEFINE_ARRAY(SeqBlock *, BlockArray);

class SummaryInfo {
 public:
   int            bytesPerFrame;
   sampleCount    frames64K;
   int            offset64K;
   sampleCount    frames256;
   int            offset256;
   int            totalSummaryBytes;
};

const int headerTagLen = 20;
char headerTag[headerTagLen + 1] = "AudacityBlockFile110";

int Sequence::sMaxDiskBlockSize = 1048576;

// Sequence methods
Sequence::Sequence(DirManager * projDirManager, sampleFormat format)
{
   mDirManager = projDirManager;
   mNumSamples = 0;
   mSampleFormat = format;
   mBlock = new BlockArray();

   mSummary = new SummaryInfo;
   mMinSamples = sMaxDiskBlockSize / SAMPLE_SIZE(mSampleFormat) / 2;
   mMaxSamples = mMinSamples * 2;
   CalcSummaryInfo();
}

Sequence::Sequence(const Sequence &orig)
{
   mDirManager = orig.mDirManager;
   mNumSamples = 0;
   mSampleFormat = orig.mSampleFormat;
   mSummary = new SummaryInfo;
   *mSummary = *orig.mSummary;
   mMaxSamples = orig.mMaxSamples;
   mMinSamples = orig.mMinSamples;

   mBlock = new BlockArray();

   Paste(0, &orig);
}

Sequence::~Sequence()
{
   for (unsigned int i = 0; i < mBlock->Count(); i++) {
      if (mBlock->Item(i)->f)
         mDirManager->Deref(mBlock->Item(i)->f);
      delete mBlock->Item(i);
   }

   delete mBlock;

   delete mSummary;
}

int Sequence::GetSummaryBytes() const
{
   return mSummary->totalSummaryBytes;
}

sampleCount Sequence::GetMaxBlockSize() const
{
   return mMaxSamples;
}

sampleCount Sequence::GetIdealBlockSize() const
{
   return mMaxSamples;
}

bool Sequence::Lock()
{
   for (unsigned int i = 0; i < mBlock->Count(); i++)
      mBlock->Item(i)->f->Lock();

   return true;
}

bool Sequence::Unlock()
{
   for (unsigned int i = 0; i < mBlock->Count(); i++)
      mBlock->Item(i)->f->Unlock();

   return true;
}

sampleFormat Sequence::GetSampleFormat() const
{
   return mSampleFormat;
}

bool Sequence::SetSampleFormat(sampleFormat format)
{
   if (mBlock->Count() > 0 || mNumSamples > 0)
      return false;

   mSampleFormat = format;
   return true;
}

bool Sequence::ConvertToSampleFormat(sampleFormat format)
{
   if (format == mSampleFormat)
      return true;

   if (mBlock->Count() == 0) {
      mSampleFormat = format;
      return true;
   }

   sampleFormat oldFormat = mSampleFormat;
   mSampleFormat = format;

   for (unsigned int i = 0; i < mBlock->Count(); i++) {
      BlockFile *oldBlock = mBlock->Item(i)->f;
      sampleCount len = mBlock->Item(i)->len;

      if (!oldBlock->IsAlias()) {
         BlockFile *newBlock =
            mDirManager->NewBlockFile(mSummary->totalSummaryBytes);

         samplePtr buffer1 = NewSamples(len, oldFormat);
         samplePtr buffer2 = NewSamples(len, mSampleFormat);

         oldBlock->ReadData(buffer1, oldFormat, 0, len);
         CopySamples(buffer1, oldFormat,
                     buffer2, mSampleFormat, len);
         newBlock->WriteData(buffer2, mSampleFormat, len);

         mBlock->Item(i)->f = newBlock;
         mDirManager->Deref(oldBlock);

         UpdateSummaries(buffer2, mBlock->Item(i), len);

         DeleteSamples(buffer2);
         DeleteSamples(buffer1);
      }
   }

   return true;
}

bool Sequence::GetMinMax(sampleCount start, sampleCount len,
                         float * outMin, float * outMax) const
{
   if (len == 0 || mBlock->Count() == 0) {
      *outMin = float(0.0);
      *outMax = float(0.0);
      return true;
   }

   float min = FLT_MAX;
   float max = -FLT_MAX;

   unsigned int block0 = FindBlock(start);
   unsigned int block1 = FindBlock(start + len);
   
   sampleCount s0, l0, maxl0;

   // First calculate the min/max of the blocks in the middle of this region;
   // this is very fast because we have the min/max of every entire block
   // already in memory.
   unsigned int b;
   int i;

   for (b = block0 + 1; b < block1; b++) {
      if (mBlock->Item(b)->min < min)
         min = mBlock->Item(b)->min;
      if (mBlock->Item(b)->max > max)
         max = mBlock->Item(b)->max;
   }

   // Now we take the first and last blocks into account, noting that the
   // selection may only partly overlap these blocks.  If the overall min/max
   // of either of these blocks is within min...max, then we can ignore them.
   // If not, we need read some samples and summaries from disk.
   if (mBlock->Item(block0)->min < min || mBlock->Item(block0)->max > max) {
      s0 = start - mBlock->Item(block0)->start;
      l0 = len;
      maxl0 = mBlock->Item(block0)->start + mBlock->Item(block0)->len - start;
      if (l0 > maxl0)
         l0 = maxl0;

      float *buffer = new float[l0];

      // TODO: optimize this to use Read256 and Read64K
      Read((samplePtr)buffer, floatSample, mBlock->Item(block0), s0, l0);
      for (i = 0; i < l0; i++) {
         if (buffer[i] < min)
            min = buffer[i];
         if (buffer[i] > max)
            max = buffer[i];
      }

      delete[]buffer;
   }

   if (block1 > block0 &&
       (mBlock->Item(block1)->min < min
        || mBlock->Item(block1)->max > max)) {

      s0 = 0;
      l0 = (start + len) - mBlock->Item(block1)->start;
      float *buffer = new float[l0];

      // TODO: optimize this to use Read256 and Read64K
      Read((samplePtr)buffer, floatSample, mBlock->Item(block1), s0, l0);
      for (i = 0; i < l0; i++) {
         if (buffer[i] < min)
            min = buffer[i];
         if (buffer[i] > max)
            max = buffer[i];
      }

      delete[]buffer;

   }

   *outMin = min;
   *outMax = max;

   return true;
}

bool Sequence::Copy(sampleCount s0, sampleCount s1, Sequence **dest)
{
   *dest = 0;

   if (s0 >= s1 || s0 >= mNumSamples || s1 < 0)
      return false;

   int numBlocks = mBlock->Count();
   int b0 = FindBlock(s0);
   int b1 = FindBlock(s1);

   if (s1 == mNumSamples)
      b1 = numBlocks;

   *dest = new Sequence(mDirManager, mSampleFormat);

   samplePtr buffer = NewSamples(mMaxSamples, mSampleFormat);

   int blocklen;

   // Do the first block

   if (b0 >= 0 && b0 < numBlocks && s0 != mBlock->Item(b0)->start) {

      blocklen = (mBlock->Item(b0)->start + mBlock->Item(b0)->len - s0);
      if (blocklen > (s1 - s0))
         blocklen = s1 - s0;
      Get(buffer, mSampleFormat, s0, blocklen);

      (*dest)->Append(buffer, mSampleFormat, blocklen);
   }

   if (b0 >= 0 && b0 < numBlocks && s0 == mBlock->Item(b0)->start) {
      b0--;
   }
   // If there are blocks in the middle, copy the blockfiles directly
   for (int b = b0 + 1; b < b1; b++)
      ((Sequence *)*dest)->AppendBlock(mBlock->Item(b));

   // Do the last block
   if (b1 > b0 && b1 < numBlocks) {
      blocklen = (s1 - mBlock->Item(b1)->start);
      Get(buffer, mSampleFormat, mBlock->Item(b1)->start, blocklen);
      (*dest)->Append(buffer, mSampleFormat, blocklen);
   }
   
   DeleteSamples(buffer);
   
   return true;
}

bool Sequence::Paste(sampleCount s, const Sequence *src)
{
   if (s < 0)
      s = 0;
   if (s >= mNumSamples)
      s = mNumSamples;

   BlockArray *srcBlock = src->mBlock;
   int addedLen = src->mNumSamples;
   unsigned int srcNumBlocks = srcBlock->Count();
   int sampleSize = SAMPLE_SIZE(mSampleFormat);

   if (addedLen == 0 || srcNumBlocks == 0)
      return true;

   unsigned int b = FindBlock(s);
   unsigned int numBlocks = mBlock->Count();

   if (numBlocks == 0) {
      // Special case: this track is currently empty.

      for (unsigned int i = 0; i < srcNumBlocks; i++)
         AppendBlock(srcBlock->Item(i));

      return ConsistencyCheck("Paste branch one");
   }

   if (b >= 0 && b < numBlocks
       && mBlock->Item(b)->len + addedLen < mMaxSamples) {
      // Special case: we can fit all of the new samples inside of
      // one block!

      samplePtr buffer = NewSamples(mMaxSamples, mSampleFormat);

      int splitPoint = s - mBlock->Item(b)->start;
      Read(buffer, mSampleFormat, mBlock->Item(b), 0, splitPoint);
      src->Get(buffer + splitPoint*sampleSize,
               mSampleFormat, 0, addedLen);
      Read(buffer + (splitPoint + addedLen)*sampleSize,
           mSampleFormat, mBlock->Item(b),
           splitPoint, mBlock->Item(b)->len - splitPoint);

      SeqBlock *largerBlock = new SeqBlock();
      largerBlock->start = mBlock->Item(b)->start;
      largerBlock->len = mBlock->Item(b)->len + addedLen;
      largerBlock->f =
         mDirManager->NewBlockFile(mSummary->totalSummaryBytes);

      FirstWrite(buffer, largerBlock, largerBlock->len);

      mDirManager->Deref(mBlock->Item(b)->f);
      delete mBlock->Item(b);
      mBlock->Item(b) = largerBlock;

      for (unsigned int i = b + 1; i < numBlocks; i++)
         mBlock->Item(i)->start += addedLen;

      mNumSamples += addedLen;

      DeleteSamples(buffer);

      return ConsistencyCheck("Paste branch two");
   }

   // Case two: if we are inserting four or fewer blocks,
   // it's simplest to just lump all the data together
   // into one big block along with the split block,
   // then resplit it all
   unsigned int i;

   BlockArray *newBlock = new BlockArray();
   newBlock->Alloc(numBlocks + srcNumBlocks + 2);
   int newNumBlocks = 0;

   for (i = 0; i < b; i++) {
      newBlock->Add(mBlock->Item(i));
      newNumBlocks++;
   }

   SeqBlock *splitBlock = mBlock->Item(b);
   sampleCount splitLen = mBlock->Item(b)->len;
   int splitPoint = s - splitBlock->start;

   if (srcNumBlocks <= 4) {

      sampleCount sum = splitLen + addedLen;

      samplePtr sumBuffer = NewSamples(sum, mSampleFormat);
      Read(sumBuffer, mSampleFormat, splitBlock, 0, splitPoint);
      src->Get(sumBuffer + splitPoint * sampleSize,
               mSampleFormat,
               0, addedLen);
      Read(sumBuffer + (splitPoint + addedLen) * sampleSize, mSampleFormat,
           splitBlock, splitPoint,
           splitBlock->len - splitPoint);

      BlockArray *split = Blockify(sumBuffer, sum);
      for (i = 0; i < split->Count(); i++) {
         split->Item(i)->start += splitBlock->start;
         newBlock->Add(split->Item(i));
         newNumBlocks++;
      }
      delete split;
      DeleteSamples(sumBuffer);
   } else {

      // The final case is that we're inserting at least five blocks.
      // We divide these into three groups: the first two get merged
      // with the first half of the split block, the middle ones get
      // copied in as is, and the last two get merged with the last
      // half of the split block.

      sampleCount srcFirstTwoLen =
          srcBlock->Item(0)->len + srcBlock->Item(1)->len;
      sampleCount leftLen = splitPoint + srcFirstTwoLen;

      samplePtr leftBuffer = NewSamples(leftLen, mSampleFormat);
      Read(leftBuffer, mSampleFormat, splitBlock, 0, splitPoint);
      src->Get(leftBuffer + splitPoint*sampleSize,
               mSampleFormat, 0, srcFirstTwoLen);

      BlockArray *split = Blockify(leftBuffer, leftLen);
      for (i = 0; i < split->Count(); i++) {
         split->Item(i)->start += splitBlock->start;
         newBlock->Add(split->Item(i));
         newNumBlocks++;
      }
      delete split;
      DeleteSamples(leftBuffer);

      for (i = 2; i < srcNumBlocks - 2; i++) {
         SeqBlock *insertBlock = new SeqBlock();
         insertBlock->start = srcBlock->Item(i)->start + s;
         insertBlock->len = srcBlock->Item(i)->len;
         insertBlock->min = srcBlock->Item(i)->min;
         insertBlock->max = srcBlock->Item(i)->max;
         insertBlock->rms = srcBlock->Item(i)->rms;

         insertBlock->f = mDirManager->CopyBlockFile(srcBlock->Item(i)->f);
         if (!insertBlock->f) {
            // TODO error: Could not paste!  (Out of disk space?)
            return false;
         }

         newBlock->Add(insertBlock);
         newNumBlocks++;
      }

      sampleCount srcLastTwoLen =
         srcBlock->Item(srcNumBlocks - 2)->len +
         srcBlock->Item(srcNumBlocks - 1)->len;
      sampleCount rightSplit = splitBlock->len - splitPoint;
      sampleCount rightLen = rightSplit + srcLastTwoLen;

      samplePtr rightBuffer = NewSamples(rightLen, mSampleFormat);
      sampleCount lastStart = srcBlock->Item(srcNumBlocks - 2)->start;
      src->Get(rightBuffer, mSampleFormat,
               lastStart, srcLastTwoLen);
      Read(rightBuffer + srcLastTwoLen * sampleSize, mSampleFormat,
           splitBlock, splitPoint, rightSplit);

      sampleCount pos = s + lastStart;

      split = Blockify(rightBuffer, rightLen);
      for (i = 0; i < split->Count(); i++) {
         split->Item(i)->start += pos;
         newBlock->Add(split->Item(i));
         newNumBlocks++;
      }
      delete split;
      DeleteSamples(rightBuffer);
   }

   mDirManager->Deref(splitBlock->f);
   delete splitBlock;

   // Copy remaining blocks to new block array and
   // swap the new block array in for the old
   for (i = b + 1; i < numBlocks; i++) {
      mBlock->Item(i)->start += addedLen;
      newBlock->Add(mBlock->Item(i));
      newNumBlocks++;
   }

   delete mBlock;
   mBlock = newBlock;

   mNumSamples += addedLen;

   return ConsistencyCheck("Paste branch three");
}

bool Sequence::SetSilence(sampleCount s0, sampleCount len)
{
   return Set(NULL, mSampleFormat, s0, len);
}

bool Sequence::InsertSilence(sampleCount s0, sampleCount len)
{
   // Create a new track containing as much silence as we
   // need to insert, and then call Paste to do the insertion
   Sequence *sTrack = new Sequence(mDirManager, mSampleFormat);

   sampleCount idealSamples = GetIdealBlockSize();

   // Allocate a zeroed buffer
   samplePtr buffer = NewSamples(idealSamples, mSampleFormat);
   ClearSamples(buffer, mSampleFormat, 0, idealSamples);

   sampleCount pos = 0;
   BlockFile *firstBlockFile = NULL;

   while (len) {
      sampleCount l = (len > idealSamples ? idealSamples : len);

      SeqBlock *w = new SeqBlock();
      w->start = pos;
      w->len = l;
      w->min = float(0.0);
      w->max = float(0.0);
      w->rms = float(0.0);
      if (pos == 0 || len == l) {
         w->f = mDirManager->NewBlockFile(mSummary->totalSummaryBytes);
         firstBlockFile = w->f;
         FirstWrite(buffer, w, l);
      } else {
         w->f = mDirManager->CopyBlockFile(firstBlockFile);
         if (!w->f) {
            // TODO set error message
            return false;
         }
      }

      sTrack->mBlock->Add(w);

      pos += l;
      len -= l;
   }

   sTrack->mNumSamples = pos;

   Paste(s0, sTrack);

   delete sTrack;
   DeleteSamples(buffer);

   return ConsistencyCheck("InsertSilence");
}

bool Sequence::AppendAlias(wxString fullPath,
                           sampleCount start,
                           sampleCount len, int channel)
{
   SeqBlock *newBlock = new SeqBlock();
   newBlock->start = mNumSamples;
   newBlock->len = len;
   newBlock->f = mDirManager->NewBlockFile(mSummary->totalSummaryBytes);
   newBlock->f->SetAliasedData(fullPath, start, len, channel);

   samplePtr buffer = NewSamples(len, mSampleFormat);
   Read(buffer, mSampleFormat, newBlock, 0, len);

   UpdateSummaries(buffer, newBlock, len);

   DeleteSamples(buffer);

   mBlock->Add(newBlock);
   mNumSamples += newBlock->len;

   return true;
}

bool Sequence::AppendBlock(SeqBlock * b)
{
   SeqBlock *newBlock = new SeqBlock();
   newBlock->start = mNumSamples;
   newBlock->len = b->len;
   newBlock->f = mDirManager->CopyBlockFile(b->f);
   if (!newBlock->f) {
      // TODO error Could not paste!  (Out of disk space?)
      return false;
   }
   newBlock->min = b->min;
   newBlock->max = b->max;
   newBlock->rms = b->rms;
   mDirManager->Ref(newBlock->f);
   mBlock->Add(newBlock);
   mNumSamples += newBlock->len;

   // Don't do a consistency check here because this
   // function gets called in an inner loop

   return true;
}

sampleCount Sequence::GetBestBlockSize(sampleCount start) const
{
   // This method returns a nice number of samples you should try to grab in
   // one big chunk in order to land on a block boundary, based on the starting
   // sample.  The value returned will always be nonzero and will be no larger
   // than the value of GetMaxBlockSize();
   int b = FindBlock(start);
   int numBlocks = mBlock->Count();
   
   sampleCount result = (mBlock->Item(b)->start + mBlock->Item(b)->len - start);
   
   while(result < mMinSamples && b+1<numBlocks &&
         (mBlock->Item(b)->len+result) <= mMaxSamples) {
      b++;
      result += mBlock->Item(b)->len;
   }
   
   wxASSERT(result > 0 && result <= mMaxSamples);
   
   return result;
}

bool Sequence::HandleXMLTag(const char *tag, const char **attrs)
{
   if (!strcmp(tag, "blockfile")) {
      if (mBlock->Count() == 0)
         return false;

      SeqBlock *lastSeqBlock = mBlock->Item(mBlock->Count()-1);
      lastSeqBlock->f = mDirManager->LoadBlockFile(attrs, mSampleFormat);

      return true;
   }
   if (!strcmp(tag, "waveblock")) {
      SeqBlock *wb = new SeqBlock();
      wb->f = 0;
      wb->start = 0;
      wb->len = 0;

      // loop through attrs, which is a null-terminated list of
      // attribute-value pairs
      while(*attrs) {
         const char *attr = *attrs++;
         const char *value = *attrs++;
         
         if (!value)
            break;
         
         if (!strcmp(attr, "start"))
            wb->start = atoi(value);
 
         if (!strcmp(attr, "len"))
            wb->len = atoi(value);

      } // while

      mBlock->Add(wb);

      return true;
   }
   
   if (!strcmp(tag, "sequence")) {
      while(*attrs) {
         const char *attr = *attrs++;
         const char *value = *attrs++;
         
         if (!value)
            break;
         
         if (!strcmp(attr, "maxsamples"))
            mMaxSamples = atoi(value);
         else if (!strcmp(attr, "sampleformat"))
            mSampleFormat = (sampleFormat)atoi(value);
         else if (!strcmp(attr, "numsamples"))
            mNumSamples = atoi(value);         
      } // while

      CalcSummaryInfo();

      return true;
   }
   
   return false;
}

XMLTagHandler *Sequence::HandleXMLChild(const char *tag)
{
   if (!strcmp(tag, "waveblock") ||
       !strcmp(tag, "blockfile"))
      return this;
   else
      return NULL;
}

void Sequence::WriteXML(int depth, FILE *fp)
{
   int i;
   unsigned int b;
   
   for(i=0; i<depth; i++)
      fprintf(fp, "\t");
   fprintf(fp, "<sequence ");
   fprintf(fp, "maxsamples=\"%d\" ", (int)mMaxSamples);
   fprintf(fp, "sampleformat=\"%d\" ", (int)mSampleFormat);
   fprintf(fp, "numsamples=\"%d\" ", (int)mNumSamples);
   fprintf(fp, ">\n");

   for (b = 0; b < mBlock->Count(); b++) {
      SeqBlock *bb = mBlock->Item(b);
      for(i=0; i<depth+1; i++)
         fprintf(fp, "\t");
      fprintf(fp, "<waveblock ");
      fprintf(fp, "start=\"%d\" ", bb->start);
      fprintf(fp, "len=\"%d\" ", bb->len);
      fprintf(fp, ">\n");

      mDirManager->SaveBlockFile(bb->f, depth+2, fp);

      for(i=0; i<depth+1; i++)
         fprintf(fp, "\t");
      fprintf(fp, "</waveblock>\n");
   }

   for(i=0; i<depth; i++)
      fprintf(fp, "\t");
   fprintf(fp, "</sequence>\n");
}

SeqBlock *Sequence::NewInitedSeqBlock()
{
   SeqBlock *b = new SeqBlock();
   b->f = mDirManager->NewBlockFile(mSummary->totalSummaryBytes);
   return b;
}

int Sequence::FindBlock(sampleCount pos, sampleCount lo,
                        sampleCount guess, sampleCount hi) const
{
   wxASSERT(mBlock->Item(guess)->len > 0);
   wxASSERT(lo <= guess && guess <= hi && lo <= hi);

   if (pos >= mBlock->Item(guess)->start &&
       pos < mBlock->Item(guess)->start + mBlock->Item(guess)->len)
      return guess;

   if (pos < mBlock->Item(guess)->start)
      return FindBlock(pos, lo, (lo + guess) / 2, guess);
   else
      return FindBlock(pos, guess + 1, (guess + 1 + hi) / 2, hi);
}

int Sequence::FindBlock(sampleCount pos) const
{
   wxASSERT(pos >= 0 && pos <= mNumSamples);

   int numBlocks = mBlock->Count();

   if (pos == 0)
      return 0;

   if (pos == mNumSamples)
      return (numBlocks - 1);

   int rval = FindBlock(pos, 0, numBlocks / 2, numBlocks);

   wxASSERT(rval >= 0 && rval < numBlocks &&
            pos >= mBlock->Item(rval)->start &&
            pos < mBlock->Item(rval)->start + mBlock->Item(rval)->len);

   return rval;
}

bool Sequence::Read(samplePtr buffer, sampleFormat format,
                    SeqBlock * b, sampleCount start, sampleCount len) const
{
   wxASSERT(b);
   wxASSERT(start >= 0);
   wxASSERT(start + len <= b->len);

   BlockFile *f = b->f;

   int result = f->ReadData(buffer, format, start, len);

   if (result != len) {
      // TODO err
      printf(_("Expected to read %d samples, got %d samples.\n"),
             len, result);
   }

   return true;
}

bool Sequence::Read256(float *buffer, SeqBlock * b,
                       sampleCount start, sampleCount len) const
{
   wxASSERT(b);
   wxASSERT(start >= 0);
   wxASSERT(start + len <= ((b->len + 255) / 256));

   char *summary = (char *)malloc(GetSummaryBytes());
   if (!b->f->ReadSummary(summary)) {
      // TODO handle missing file
      return false;
   }

   memcpy(buffer,
          summary + mSummary->offset256 + (start * mSummary->bytesPerFrame),
          len * mSummary->bytesPerFrame);

   free(summary);

   return true;
}

bool Sequence::Read64K(float *buffer, SeqBlock * b,
                       sampleCount start, sampleCount len) const
{
   wxASSERT(b);
   wxASSERT(start >= 0);
   wxASSERT(start + len <= ((b->len + 65535) / 65536));

   char *summary = (char *)malloc(GetSummaryBytes());
   if (!b->f->ReadSummary(summary)) {
      // TODO handle missing file
      return false;
   }

   memcpy(buffer,
          summary + mSummary->offset64K + (start * mSummary->bytesPerFrame),
          len * mSummary->bytesPerFrame);

   free(summary);

   return true;
}

bool Sequence::FirstWrite(samplePtr buffer, SeqBlock * b,
                          sampleCount len)
{
   wxASSERT(b);
   wxASSERT(b->len <= mMaxSamples);

   UpdateSummaries(buffer, b, len);
   return b->f->WriteData(buffer, mSampleFormat, len);
}

bool Sequence::CopyWrite(samplePtr buffer, SeqBlock *b,
                         sampleCount start, sampleCount len)
{
   // We don't ever write to an existing block; to support Undo,
   // we copy the old block entirely into memory, dereference it,
   // make the change, and then write the new block to disk.

   wxASSERT(b);
   wxASSERT(b->len <= mMaxSamples);
   wxASSERT(start + len <= b->len);

   int sampleSize = SAMPLE_SIZE(mSampleFormat);
   samplePtr newBuffer = NewSamples(mMaxSamples, mSampleFormat);
   wxASSERT(newBuffer);

   Read(newBuffer, mSampleFormat, b, 0, b->len);
   memcpy(newBuffer + start*sampleSize, buffer, len*sampleSize);

   BlockFile *oldBlockFile = b->f;
   b->f = mDirManager->NewBlockFile(mSummary->totalSummaryBytes);

   mDirManager->Deref(oldBlockFile);

   b->f->WriteData(newBuffer, mSampleFormat, b->len);
   UpdateSummaries(newBuffer, b, b->len);

   DeleteSamples(newBuffer);

   return true;
}

bool Sequence::UpdateSummaries(samplePtr buffer,
                               SeqBlock * b, sampleCount len)
{
   char *fullSummary = (char *)malloc(mSummary->totalSummaryBytes);

   memcpy(fullSummary, headerTag, headerTagLen);

   float *summary64K = (float *)(fullSummary + mSummary->offset64K);
   float *summary256 = (float *)(fullSummary + mSummary->offset256);

   float *fbuffer = new float[len];
   CopySamples(buffer, mSampleFormat,
               (samplePtr)fbuffer, floatSample, len);

   sampleCount sumLen;
   sampleCount i, j, jcount;

   float min, max;
   float sumsq;

   // Recalc 256 summaries
   sumLen = (len + 255) / 256;

   for (i = 0; i < sumLen; i++) {
      min = fbuffer[i * 256];
      max = fbuffer[i * 256];
      sumsq = ((float)min) * ((float)min);
      jcount = 256;
      if (i * 256 + jcount > len)
         jcount = len - i * 256;
      for (j = 1; j < jcount; j++) {
         float f1 = fbuffer[i * 256 + j];
         sumsq += ((float)f1) * ((float)f1);
         if (f1 < min)
            min = f1;
         else if (f1 > max)
            max = f1;
      }

      float rms = (float)sqrt(sumsq / jcount);

      summary256[i * 3] = min;
      summary256[i * 3 + 1] = max;
      summary256[i * 3 + 2] = rms;
   }
   for (i = sumLen; i < mSummary->frames256; i++) {
      summary256[i * 3] = 0.0f;
      summary256[i * 3 + 1] = 0.0f;
      summary256[i * 3 + 2] = 0.0f;
   }

   // Recalc 64K summaries
   sumLen = (len + 65535) / 65536;

   for (i = 0; i < sumLen; i++) {
      min = summary256[3 * i * 256];
      max = summary256[3 * i * 256 + 1];
      sumsq = (float)summary256[3 * i * 256 + 2];
      sumsq *= sumsq;

      for (j = 1; j < 256; j++) {
         if (summary256[3 * (i * 256 + j)] < min)
            min = summary256[3 * (i * 256 + j)];
         if (summary256[3 * (i * 256 + j) + 1] > max)
            max = summary256[3 * (i * 256 + j) + 1];
         float r1 = summary256[3 * (i * 256 + j) + 2];
         sumsq += r1*r1;
      }

      float rms = (float)sqrt(sumsq / 256);

      summary64K[i * 3] = min;
      summary64K[i * 3 + 1] = max;
      summary64K[i * 3 + 2] = rms;
   }
   for (i = sumLen; i < mSummary->frames64K; i++) {
      summary64K[i * 3] = 0.0f;
      summary64K[i * 3 + 1] = 0.0f;
      summary64K[i * 3 + 2] = 0.0f;
   }

   // Recalc block-level summary
   min = summary64K[0];
   max = summary64K[1];
   sumsq = (float)summary64K[2];
   sumsq *= sumsq;
   
   for (i = 1; i < sumLen; i++) {
      if (summary64K[3*i] < min)
         min = summary64K[3*i];
      else if (summary64K[3*i+1] > max)
         max = summary64K[3*i+1];
      float r1 = (float)summary64K[3*i+2];
      sumsq += (r1*r1);
   }
   b->min = min;
   b->max = max;
   b->rms = sqrt(sumsq / sumLen);

   b->f->WriteSummary(fullSummary);

   delete[] fbuffer;
   free(fullSummary);

   return true;
}

bool Sequence::Get(samplePtr buffer, sampleFormat format,
                   sampleCount start, sampleCount len) const
{
   if (start < 0 || start > mNumSamples ||
       start+len > mNumSamples)
      return false;
   int b = FindBlock(start);

   while (len) {
      sampleCount blen =
          mBlock->Item(b)->start + mBlock->Item(b)->len - start;
      if (blen > len)
         blen = len;
      sampleCount bstart = (start - (mBlock->Item(b)->start));

      Read(buffer, format, mBlock->Item(b), bstart, blen);

      len -= blen;
      buffer += (blen * SAMPLE_SIZE(format));
      b++;
      start += blen;
   }

   return true;
}

// Pass NULL to set silence
bool Sequence::Set(samplePtr buffer, sampleFormat format,
                   sampleCount start, sampleCount len)
{
   if (start < 0 || start > mNumSamples ||
       start+len > mNumSamples)
      return false;

   samplePtr temp = NULL;
   if (format != mSampleFormat) {
      temp = NewSamples(mMaxSamples, mSampleFormat);
      wxASSERT(temp);
   }

   samplePtr silence = NULL;
   if (!buffer) {
      silence = NewSamples(mMaxSamples, format);
      wxASSERT(silence);
      ClearSamples(silence, format, 0, mMaxSamples);
   }

   int b = FindBlock(start);

   while (len) {
      int blen = mBlock->Item(b)->start + mBlock->Item(b)->len - start;
      if (blen > len)
         blen = len;

      if (buffer) {
         if (format == mSampleFormat)
            CopyWrite(buffer, mBlock->Item(b), start - mBlock->Item(b)->start,
                      blen);
         else {
            CopySamples(buffer, format, temp, mSampleFormat, blen);
            CopyWrite(temp, mBlock->Item(b), start - mBlock->Item(b)->start,
                      blen);
         }
         buffer += (blen * SAMPLE_SIZE(format));
      } else
         CopyWrite(silence, mBlock->Item(b), start - mBlock->Item(b)->start,
                   blen);

      len -= blen;
      start += blen;
      b++;
   }

   if (!buffer)
      DeleteSamples(silence);

   if (format != mSampleFormat)
      DeleteSamples(temp);

   return ConsistencyCheck("Set");
}

bool Sequence::GetWaveDisplay(float *min, float *max, float *rms,
                              int len, sampleCount *where,
                              double samplesPerPixel)
{
   sampleCount s0 = where[0];
   sampleCount s1 = where[len];

   int divisor;
   if (samplesPerPixel >= 65536)
      divisor = 65536;
   else if (samplesPerPixel >= 256)
      divisor = 256;
   else
      divisor = 1;

   if (s1 > mNumSamples)
      s1 = mNumSamples;

   sampleCount srcX = s0;

   unsigned int block0 = FindBlock(s0);

   float *temp = new float[mMaxSamples];

   int pixel = 0;
   float theMin = 0;
   float theMax = 0;
   float sumsq = float(0.0);
   unsigned int b = block0;
   int jcount = 0;

   while (srcX < s1) {
      // Get more samples
      sampleCount num;

      num = ((mBlock->Item(b)->len -
              (srcX - mBlock->Item(b)->start)) + divisor - 1)
         / divisor;
      
      if (num > (s1 - srcX + divisor - 1) / divisor)
         num = (s1 - srcX + divisor - 1) / divisor;
      
      switch (divisor) {
      default:
      case 1:
         Read((samplePtr)temp, floatSample, mBlock->Item(b),
              srcX - mBlock->Item(b)->start, num);
         break;
      case 256:
         Read256(temp, mBlock->Item(b),
                 (srcX - mBlock->Item(b)->start) / divisor, num);
         break;
      case 65536:
         Read64K(temp, mBlock->Item(b),
                 (srcX - mBlock->Item(b)->start) / divisor, num);
         break;
      }
      
      // Get min/max/rms of samples for each pixel we can
      int x = 0;

      if (b==block0) {
         if (divisor > 1) {
            theMin = temp[0];
            theMax = temp[1];
         }
         else {
            theMin = temp[0];
            theMax = temp[0];
         }
         sumsq = float(0.0);
         jcount = 0;
      }
      
      while (x < num) {
         
         while (pixel < len &&
                where[pixel] / divisor == srcX / divisor + x) {
            if (pixel > 0) {
               min[pixel - 1] = theMin;
               max[pixel - 1] = theMax;
               if (jcount > 0)
                  rms[pixel - 1] = (float)sqrt(sumsq / jcount);
               else
                  rms[pixel - 1] = 0.0f;
            }
            pixel++;
            if (where[pixel] != where[pixel - 1]) {
               theMin = FLT_MAX;
               theMax = -FLT_MAX;
               sumsq = float(0.0);
               jcount = 0;
            }
         }
         
         sampleCount stop = (where[pixel] - srcX) / divisor;
         if (stop == x)
            stop++;
         if (stop > num)
            stop = num;
         
         switch (divisor) {
         default:
         case 1:
            while (x < stop) {
               if (temp[x] < theMin)
                  theMin = temp[x];
               if (temp[x] > theMax)
                  theMax = temp[x];
               sumsq += ((float)temp[x]) * ((float)temp[x]);
               x++;
               jcount++;
            }
            break;
         case 256:
         case 65536:
            while (x < stop) {
               if (temp[3 * x] < theMin)
                  theMin = temp[3 * x];
               if (temp[3 * x + 1] > theMax)
                  theMax = temp[3 * x + 1];
               sumsq += ((float)temp[3*x+2]) * ((float)temp[3*x+2]);
               x++;
               jcount++;
            }
            
            break;
         }
      }

      b++;

      srcX += num * divisor;

      if (b >= mBlock->Count())
         break;

      srcX = mBlock->Item(b)->start;

   }

   while (pixel <= len) {
      min[pixel - 1] = theMin;
      max[pixel - 1] = theMax;
      if (jcount > 0)
         rms[pixel - 1] = (float)sqrt(sumsq / jcount);
      else
         rms[pixel - 1] = 0.0f;
      pixel++;
   }

   delete[] temp;

   return true;
}

sampleCount Sequence::GetIdealAppendLen()
{
   int numBlocks = mBlock->Count();
   sampleCount max = GetMaxBlockSize();
   sampleCount lastBlockLen;

   if (numBlocks == 0)
      return max;

   lastBlockLen = mBlock->Item(numBlocks-1)->len;
   if (lastBlockLen == max)
      return max;
   else
      return max - lastBlockLen;
}

bool Sequence::Append(samplePtr buffer, sampleFormat format,
                      sampleCount len)
{
   samplePtr temp = NULL;
   if (format != mSampleFormat) {
      temp = NewSamples(mMaxSamples, mSampleFormat);
      wxASSERT(temp);
   }

   // If the last block is not full, we need to add samples to it
   int numBlocks = mBlock->Count();
   if (numBlocks > 0 && mBlock->Item(numBlocks - 1)->len < mMinSamples) {
      SeqBlock *lastBlock = mBlock->Item(numBlocks - 1);
      sampleCount addLen;
      if (lastBlock->len + len < mMaxSamples)
         addLen = len;
      else
         addLen = GetIdealBlockSize() - lastBlock->len;

      SeqBlock *newLastBlock = NewInitedSeqBlock();

      samplePtr buffer2 = NewSamples((lastBlock->len + addLen), mSampleFormat);
      Read(buffer2, mSampleFormat, lastBlock, 0, lastBlock->len);

      if (format == mSampleFormat)
         memcpy(buffer2 + lastBlock->len * SAMPLE_SIZE(format),
                buffer,
                addLen * SAMPLE_SIZE(format));
      else {
         CopySamples(buffer, format, temp, mSampleFormat, addLen);
         memcpy(buffer2 + lastBlock->len * SAMPLE_SIZE(format),
                temp,
                addLen * SAMPLE_SIZE(format));
      }

      newLastBlock->start = lastBlock->start;
      newLastBlock->len = lastBlock->len + addLen;

      FirstWrite(buffer2, newLastBlock, lastBlock->len + addLen);

      DeleteSamples(buffer2);

      mDirManager->Deref(lastBlock->f);
      delete lastBlock;
      mBlock->Item(numBlocks - 1) = newLastBlock;

      len -= addLen;
      mNumSamples += addLen;
      buffer += addLen * SAMPLE_SIZE(format);
   }
   // Append the rest as new blocks
   while (len) {
      sampleCount idealSamples = GetIdealBlockSize();
      sampleCount l = (len > idealSamples ? idealSamples : len);
      SeqBlock *w = new SeqBlock();
      w->f = mDirManager->NewBlockFile(mSummary->totalSummaryBytes);
      w->start = mNumSamples;
      w->len = l;

      if (format == mSampleFormat)
         FirstWrite(buffer, w, l);
      else {
         CopySamples(buffer, format, temp, mSampleFormat, l);
         FirstWrite(temp, w, l);
      }

      mBlock->Add(w);

      buffer += l * SAMPLE_SIZE(format);
      mNumSamples += l;
      len -= l;
   }

   if (format != mSampleFormat)
      DeleteSamples(temp);

   ConsistencyCheck("Append");

   return true;
}

BlockArray *Sequence::Blockify(samplePtr buffer, sampleCount len)
{
   BlockArray *list = new BlockArray();
   list->Alloc(10);

   if (len == 0)
      return list;

   int num = (len + (mMaxSamples - 1)) / mMaxSamples;

   for (int i = 0; i < num; i++) {
      SeqBlock *b = NewInitedSeqBlock();

      b->start = i * len / num;
      b->len = ((i + 1) * len / num) - b->start;

      FirstWrite(buffer + (b->start * SAMPLE_SIZE(mSampleFormat)),
                 b, b->len);

      list->Add(b);
   }

   return list;
}

bool Sequence::Delete(sampleCount start, sampleCount len)
{
   if (len == 0)
      return true;
   if (len < 0 || start < 0 || start >= mNumSamples)
      return false;

   unsigned int numBlocks = mBlock->Count();
   unsigned int newNumBlocks = 0;

   unsigned int b0 = FindBlock(start);
   unsigned int b1 = FindBlock(start + len - 1);

   int sampleSize = SAMPLE_SIZE(mSampleFormat);

   // Special case: if the samples to delete are all within a single
   // block and the resulting length is not too small, perform the
   // deletion within this block:
   if (b0 == b1 && mBlock->Item(b0)->len - len >= mMinSamples) {
      SeqBlock *b = mBlock->Item(b0);
      sampleCount pos = start - b->start;
      sampleCount newLen = b->len - len;

      samplePtr buffer = NewSamples(newLen, mSampleFormat);

      Read(buffer, mSampleFormat, b, 0, pos);
      Read(buffer + (pos * sampleSize), mSampleFormat,
           b, pos + len, newLen - pos);

      SeqBlock *newBlock = NewInitedSeqBlock();
      newBlock->start = b->start;
      newBlock->len = newLen;
      FirstWrite(buffer, newBlock, newLen);

      mBlock->Item(b0) = newBlock;

      for (unsigned int j = b0 + 1; j < numBlocks; j++)
         mBlock->Item(j)->start -= len;

      DeleteSamples(buffer);

      mDirManager->Deref(b->f);
      delete b;

      mNumSamples -= len;
      return ConsistencyCheck("Delete - branch one");
   }

   // Create a new array of blocks
   BlockArray *newBlock = new BlockArray();
   newBlock->Alloc(numBlocks - (b1 - b0) + 2);

   // Copy the blocks before the deletion point over to
   // the new array
   unsigned int i;
   for (i = 0; i < b0; i++) {
      newBlock->Add(mBlock->Item(i));
      newNumBlocks++;
   }

   // First grab the samples in block b0 before the deletion point
   // into preBuffer.  If this is enough samples for its own block,
   // or if this would be the first block in the array, write it out.
   // Otherwise combine it with the previous block (splitting them
   // 50/50 if necessary).
   SeqBlock *preBlock = mBlock->Item(b0);
   sampleCount preBufferLen = start - preBlock->start;
   if (preBufferLen) {
      if (preBufferLen >= mMinSamples || b0 == 0) {
         SeqBlock *insBlock = NewInitedSeqBlock();

         insBlock->len = preBufferLen;
         insBlock->start = preBlock->start;

         samplePtr preBuffer = NewSamples(preBufferLen, mSampleFormat);
         Read(preBuffer, mSampleFormat, preBlock, 0, preBufferLen);
         FirstWrite(preBuffer, insBlock, preBufferLen);
         DeleteSamples(preBuffer);

         newBlock->Add(insBlock);
         newNumBlocks++;

         if (b0 != b1) {
            mDirManager->Deref(preBlock->f);
            delete preBlock;
         }
      } else {
         SeqBlock *prepreBlock = mBlock->Item(b0 - 1);
         sampleCount prepreLen = prepreBlock->len;
         sampleCount sum = prepreLen + preBufferLen;

         samplePtr sumBuffer = NewSamples(sum, mSampleFormat);

         Read(sumBuffer, mSampleFormat, prepreBlock, 0, prepreLen);
         Read(sumBuffer + prepreLen*sampleSize, mSampleFormat,
              preBlock, 0, preBufferLen);

         BlockArray *split = Blockify(sumBuffer, sum);
         split->Item(0)->start += prepreBlock->start;
         newBlock->Item(b0 - 1) = split->Item(0);
         for (i = 1; i < split->Count(); i++) {
            split->Item(i)->start += prepreBlock->start;
            newBlock->Add(split->Item(i));
            newNumBlocks++;
         }
         delete split;

         DeleteSamples(sumBuffer);

         mDirManager->Deref(prepreBlock->f);
         delete prepreBlock;

         if (b0 != b1) {
            mDirManager->Deref(preBlock->f);
            delete preBlock;
         }
      }
   } else {
      // The sample where we begin deletion happens to fall
      // right on the beginning of a block.
      if (b0 != b1) {
         mDirManager->Deref(mBlock->Item(b0)->f);
         delete mBlock->Item(b0);
      }
   }

   // Next, delete blocks strictly between b0 and b1
   for (i = b0 + 1; i < b1; i++) {
      mDirManager->Deref(mBlock->Item(i)->f);
      delete mBlock->Item(i);
   }

   // Now, symmetrically, grab the samples in block b1 after the
   // deletion point into postBuffer.  If this is enough samples
   // for its own block, or if this would be the last block in
   // the array, write it out.  Otherwise combine it with the
   // subsequent block (splitting them 50/50 if necessary).
   SeqBlock *postBlock = mBlock->Item(b1);
   sampleCount postBufferLen =
       (postBlock->start + postBlock->len) - (start + len);
   if (postBufferLen) {
      if (postBufferLen >= mMinSamples || b1 == numBlocks - 1) {
         SeqBlock *insBlock = NewInitedSeqBlock();

         insBlock->len = postBufferLen;
         insBlock->start = start;

         samplePtr postBuffer = NewSamples(postBufferLen, mSampleFormat);
         sampleCount pos = (start + len) - postBlock->start;
         Read(postBuffer, mSampleFormat, postBlock, pos, postBufferLen);
         FirstWrite(postBuffer, insBlock, postBufferLen);
         DeleteSamples(postBuffer);

         newBlock->Add(insBlock);
         newNumBlocks++;

         mDirManager->Deref(postBlock->f);
         delete postBlock;
      } else {
         SeqBlock *postpostBlock = mBlock->Item(b1 + 1);
         sampleCount postpostLen = postpostBlock->len;
         sampleCount sum = postpostLen + postBufferLen;

         samplePtr sumBuffer = NewSamples(sum, mSampleFormat);
         sampleCount pos = (start + len) - postBlock->start;
         Read(sumBuffer, mSampleFormat, postBlock, pos, postBufferLen);
         Read(sumBuffer + (postBufferLen * sampleSize), mSampleFormat,
              postpostBlock, 0, postpostLen);

         BlockArray *split = Blockify(sumBuffer, sum);
         for (i = 0; i < split->Count(); i++) {
            split->Item(i)->start += start;
            newBlock->Add(split->Item(i));
            newNumBlocks++;
         }
         delete split;
         b1++;

         DeleteSamples(sumBuffer);

         mDirManager->Deref(postpostBlock->f);
         delete postpostBlock;
         mDirManager->Deref(postBlock->f);
         delete postBlock;
      }
   } else {
      // The sample where we begin deletion happens to fall
      // right on the end of a block.
      if (b0 != b1) {
         mDirManager->Deref(mBlock->Item(b1)->f);
         delete mBlock->Item(b1);
      }
   }

   // Copy the remaining blocks over from the old array
   for (i = b1 + 1; i < numBlocks; i++) {
      mBlock->Item(i)->start -= len;
      newBlock->Add(mBlock->Item(i));
      newNumBlocks++;
   }

   // Substitute our new array for the old one
   delete mBlock;
   mBlock = newBlock;

   // Update total number of samples and do a consistency check.
   mNumSamples -= len;
   return ConsistencyCheck("Delete - branch two");
}

bool Sequence::ConsistencyCheck(const char *whereStr)
{
   unsigned int i;
   int pos = 0;
   unsigned int numBlocks = mBlock->Count();
   bool error = false;

   for (i = 0; i < numBlocks; i++) {
      if (pos != mBlock->Item(i)->start)
         error = true;
      pos += mBlock->Item(i)->len;
   }
   if (pos != mNumSamples)
      error = true;

   if (error) {
#ifdef __WXDEBUG__
      printf("*** Consistency check failed after %s ***\n", whereStr);
#endif
      // TODO error message

      return false;
   }

   return true;
}

void Sequence::DebugPrintf(wxString *dest)
{
   unsigned int i;
   int pos = 0;

   for (i = 0; i < mBlock->Count(); i++) {
      *dest += wxString::Format
         ("Block %3d: start %8d len %8d  %s",
          i,
          mBlock->Item(i)->start,
          mBlock->Item(i)->len,
          (const char *) (mBlock->Item(i)->f->GetName()));
      if (pos != mBlock->Item(i)->start)
         *dest += "  ERROR\n";
      else
         *dest += "\n";
      pos += mBlock->Item(i)->len;
   }
   if (pos != mNumSamples)
      *dest += wxString::Format
         ("ERROR mNumSamples = %d\n", mNumSamples);
}

void Sequence::Debug()
{
   wxString s;
   DebugPrintf(&s);
   printf((const char *)s);
}

// Static
void Sequence::SetMaxDiskBlockSize(int bytes)
{
   sMaxDiskBlockSize = bytes;
}

void Sequence::CalcSummaryInfo()
{
   SummaryInfo *s = mSummary;

   s->bytesPerFrame = sizeof(float) * 3; /* min, max, rms */

   s->frames64K = (mMaxSamples + 65535) / 65536;
   s->frames256 = s->frames64K * 256;

   s->offset64K = headerTagLen;
   s->offset256 = s->offset64K + (s->frames64K * s->bytesPerFrame);
   s->totalSummaryBytes = s->offset256 + (s->frames256 * s->bytesPerFrame);  
}

