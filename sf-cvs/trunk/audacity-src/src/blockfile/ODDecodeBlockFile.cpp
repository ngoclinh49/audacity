/**********************************************************************

  Audacity: A Digital Audio Editor

  ODDecodeBlockFile.cpp
   
  Created by Michael Chinen (mchinen)
  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2.  See License.txt.

******************************************************************//**

\class ODDecodeBlockFile
\brief ODDecodeBlockFile is a special type of SimpleBlockFile that does not necessarily have summary OR audio data available
The summary is eventually computed and written to a file in a background thread.  See ODPCMAliasBlockFile for a similar class.

*//*******************************************************************/

#include "ODDecodeBlockFile.h"


#ifdef _WIN32
   #include <windows.h>
#endif

#include <wx/utils.h>
#include <wx/wxchar.h>
#include <wx/log.h>
#include <wx/thread.h>
#include <sndfile.h>

#include "../FileFormats.h"
#include "../Internat.h"

const int bheaderTagLen = 20;
char bheaderTag[bheaderTagLen + 1] = "AudacityBlockFile112";



   /// Create a disk file and write summary and sample data to it
ODDecodeBlockFile::ODDecodeBlockFile(wxFileName baseFileName,wxFileName audioFileName,
                   samplePtr sampleData, sampleCount sampleLen,
                   sampleFormat format,
                   bool allowDeferredWrite):
   SimpleBlockFile(baseFileName,sampleData,sampleLen,format,allowDeferredWrite)
{
   mDataAvailable=false;
   mAudioFileName = audioFileName;
}
   
/// Create the memory structure to refer to the given block file
ODDecodeBlockFile::ODDecodeBlockFile(wxFileName existingFile, wxFileName audioFileName, sampleCount len,
                   float min, float max, float rms):
   SimpleBlockFile(existingFile,len,min,max,rms)
{
   mDataAvailable=false;
   mAudioFileName = audioFileName;
}



ODDecodeBlockFile::~ODDecodeBlockFile()
{

}


//Check to see if we have the file for these calls.
wxLongLong ODDecodeBlockFile::GetSpaceUsage()
{ 
   if(IsSummaryAvailable())
   {
      wxFFile summaryFile(mFileName.GetFullPath());
      return summaryFile.Length();
   }
   else
   {
      return 0;
   }
}


/// Gets extreme values for the specified region
void ODDecodeBlockFile::GetMinMax(sampleCount start, sampleCount len,
                          float *outMin, float *outMax, float *outRMS)
{
   if(IsSummaryAvailable())
   {
      SimpleBlockFile::GetMinMax(start,len,outMin,outMax,outRMS);
   }
   else
   {
      //fake values.  These values are used usually for normalization and amplifying, so we want 
      //the max to be maximal and the min to be minimal
      *outMin = -1.0;
      *outMax = 1.0;
      *outRMS = (float)0.707;//sin with amp of 1 rms
   }
}

/// Gets extreme values for the entire block
void ODDecodeBlockFile::GetMinMax(float *outMin, float *outMax, float *outRMS)
{
  if(IsSummaryAvailable())
   {
      SimpleBlockFile::GetMinMax(outMin,outMax,outRMS);
   }
   else
   {
      //fake values.  These values are used usually for normalization and amplifying, so we want 
      //the max to be maximal and the min to be minimal
      *outMin = -1.0;
      *outMax = 1.0;
      *outRMS = (float)0.707;//sin with amp of 1 rms
   }
}

/// Returns the 256 byte summary data block
bool ODDecodeBlockFile::Read256(float *buffer, sampleCount start, sampleCount len)
{
   if(IsSummaryAvailable())
   {
      return SimpleBlockFile::Read256(buffer,start,len);
   }
   else
   {
      //TODO: put some dummy value?  should we make a fake one?
      buffer = NULL;
      return true;
   }
}

/// Returns the 64K summary data block
bool ODDecodeBlockFile::Read64K(float *buffer, sampleCount start, sampleCount len)
{
   if(IsSummaryAvailable())
   {
      return SimpleBlockFile::Read64K(buffer,start,len);
   }
   else
   {
      //TODO: put some dummy value?  should we make a fake one?
      return true;
   }
}

/// If the summary has been computed,
/// Construct a new PCMAliasBlockFile based on this one.
/// otherwise construct an ODPCMAliasBlockFile that still needs to be computed.
/// @param newFileName The filename to copy the summary data to.
BlockFile *ODDecodeBlockFile::Copy(wxFileName newFileName)
{
   BlockFile *newBlockFile;
   
   
   if(IsSummaryAvailable())
   {
      //create a simpleblockfile, because once it has the summary it is a simpleblockfile for all intents an purposes
      newBlockFile  = SimpleBlockFile::Copy(newFileName) ; 
   }
   else
   {
      //TODO:do the correct constructor thingthing for this one.
      //Also, this one needs to be scheduled for loading as well ... what to do?
      newBlockFile  = NULL; // new ODPCMAliasBlockFile(newFileName,
                              //                     mAliasedFileName, mAliasStart,
                                //                   mLen, mAliasChannel,
                                  //                 mMin, mMax, mRMS);
      //TODO:add to the ODManager Task list so this one loads itself.  It's wasteful to do this twice, but lets be simple and naieve
      //just this once,.
   }
   
   
   return newBlockFile;
}


/// Writes the xml as a PCMAliasBlockFile if we can (if we have a summary file)
/// Otherwise writes XML as a subset of attributes with 'odpcmaliasblockfile as the start tag.
/// Most notably, the summaryfile attribute refers to a file that does not yet, so when the project file is read back in
/// and this object reconstructed, it needs to avoid trying to open it as well as schedule itself for OD loading
void ODDecodeBlockFile::SaveXML(XMLWriter &xmlFile)
{
/* TODO:
   if(IsSummaryAvailable())
   {
      PCMAliasBlockFile::SaveXML(xmlFile);
   }
   else
   {
      xmlFile.StartTag(wxT("ODDecodeBlockFile"));

      xmlFile.WriteAttr(wxT("summaryfile"), mFileName.GetFullName());
      xmlFile.WriteAttr(wxT("aliasfile"), mAliasedFileName.GetFullPath());
      xmlFile.WriteAttr(wxT("aliasstart"), mAliasStart);
      xmlFile.WriteAttr(wxT("aliaslen"), mLen);
      xmlFile.WriteAttr(wxT("aliaschannel"), mAliasChannel);
      //these have not been computed yet.
      //xmlFile.WriteAttr(wxT("min"), mMin);
      //xmlFile.WriteAttr(wxT("max"), mMax);
     // xmlFile.WriteAttr(wxT("rms"), mRMS);

      xmlFile.EndTag(wxT("odpcmaliasblockfile"));
   }
*/
}

/// Constructs a ODPCMAliasBlockFile from the xml output of WriteXML.
/// Also schedules the ODPCMAliasBlockFile for OD loading.
BlockFile *ODDecodeBlockFile::BuildFromXML(DirManager &dm, const wxChar **attrs)
{//TODO:
//   wxFileName summaryFileName;
//   wxFileName aliasFileName;
//   sampleCount aliasStart=0, aliasLen=0;
//   int aliasChannel=0;
//   float min=0, max=0, rms=0;
//   long nValue;
//
//
//
//   while(*attrs)
//   {
//      const wxChar *attr =  *attrs++;
//      const wxChar *value = *attrs++;
//      if (!value) 
//         break;
//
//      const wxString strValue = value;
//      if( !wxStricmp(attr, wxT("summaryfile")) )
//      {
//      
//         // Can't use XMLValueChecker::IsGoodFileName here, but do part of its test.
//         if (!XMLValueChecker::IsGoodFileString(strValue))
//            return NULL;
//
//         #ifdef _WIN32
//            if (strValue.Length() + 1 + dm.GetProjectDataDir().Length() > MAX_PATH)
//               return NULL;
//         #endif
//
//         dm.AssignFile(summaryFileName,value,FALSE);
//      }
//      else if( !wxStricmp(attr, wxT("aliasfile")) )
//      {
//         if (XMLValueChecker::IsGoodPathName(strValue))
//            aliasFileName.Assign(strValue);
//         else if (XMLValueChecker::IsGoodFileName(strValue, dm.GetProjectDataDir()))
//            // Allow fallback of looking for the file name, located in the data directory.
//            aliasFileName.Assign(dm.GetProjectDataDir(), strValue);
//         else 
//            return NULL;
//      }
//      else if (XMLValueChecker::IsGoodInt(strValue) && strValue.ToLong(&nValue)) 
//      { // integer parameters
//         if( !wxStricmp(attr, wxT("aliasstart")) )
//            aliasStart = nValue;
//         else if( !wxStricmp(attr, wxT("aliaslen")) )
//            aliasLen = nValue;
//         else if( !wxStricmp(attr, wxT("aliaschannel")) )
//            aliasChannel = nValue;
//            
//         //The folowing attributes don't exist yet - not quite sure what to do with them yet.
////         else if( !wxStricmp(attr, wxT("min")) )
////            min = nValue;
////         else if( !wxStricmp(attr, wxT("max")) )
////            max = nValue;
////         else if( !wxStricmp(attr, wxT("rms")) )
////            rms = nValue;
//      }
//   }
//
//   //file doesn't exist, but IsGoodFileName Checks that it does - maybe we should have a different method to check for valid names?
//   if ( /*!XMLValueChecker::IsGoodFileName(summaryFileName.GetFullName(), summaryFileName.GetPath(wxPATH_GET_VOLUME)) || */
//         !XMLValueChecker::IsGoodFileName(aliasFileName.GetFullName(), aliasFileName.GetPath(wxPATH_GET_VOLUME)) || 
//         (aliasLen <= 0) || (aliasLen < 0.0) || !XMLValueChecker::IsValidChannel(aliasChannel) || (rms < 0.0))
//      return NULL;
//
//
//   //TODO: schedule for OD Loading
//   
//   return new ODPCMAliasBlockFile(summaryFileName, aliasFileName,
//                                aliasStart, aliasLen, aliasChannel);
return NULL;
}



void ODDecodeBlockFile::Recover(void)
{
   if(IsSummaryAvailable())
   {
      WriteSummary();
   }
}

bool ODDecodeBlockFile::IsSummaryAvailable()
{
   return IsDataAvailable();
}

bool ODDecodeBlockFile::IsDataAvailable()
{
   bool retval;
   mDataAvailableMutex.Lock();
   retval= mDataAvailable;
   mDataAvailableMutex.Unlock();
   return retval;
}
/// Write the summary to disk, using the derived ReadData() to get the data
/// Here, the decoder ODTask associated with this file must fetch the samples with
/// the ODDecodeTask::Decode() method.
void ODDecodeBlockFile::WriteSummary()
{

/*   
   //Below from BlockFile.cpp's method.  We need to delete the data returned by
   //CalcSummary, because it uses local info.  In the future we might do something
   //smarter and thread-dependant like a static thread context.
   wxFFile summaryFile(mFileName.GetFullPath(), wxT("wb"));

   if( !summaryFile.IsOpened() ){
      // Never silence the Log w.r.t write errors; they always count
      // as new errors
      
      //however, this is going to be called from a non-main thread,
      //and wxLog calls are not thread safe.
      printf("Unable to write summary data to file %s",
                   mFileName.GetFullPath().c_str());
      // If we can't write, there's nothing to do.
      return;
   }

   // To build the summary data, call ReadData (implemented by the
   // derived classes) to get the sample data
   samplePtr sampleData = NewSamples(mLen, floatSample);
   this->ReadData(sampleData, floatSample, 0, mLen);

   void *summaryData = CalcSummary(sampleData, mLen,
                                            floatSample);
   summaryFile.Write(summaryData, mSummaryInfo.totalSummaryBytes);

   DeleteSamples(sampleData);
   delete [] (char *) summaryData;
   
   
   //above from BlockFiles.cpps method
   
*/   

   mDataAvailableMutex.Lock();
   mDataAvailable=true;
   mDataAvailableMutex.Unlock();
}



/// A thread-safe version of CalcSummary.  BlockFile::CalcSummary
/// uses a static summary array across the class, which we can't use.
/// Get a buffer containing a summary block describing this sample
/// data.  This must be called by derived classes when they
/// are constructed, to allow them to construct their summary data,
/// after which they should write that data to their disk file.
///
/// This method also has the side effect of setting the mMin, mMax,
/// and mRMS members of this class.
///
/// Unlike BlockFile's implementation You SHOULD delete the returned buffer.
/// this is protected so it shouldn't be hard to deal with - just override
/// all BlockFile methods that use this method.
///
/// @param buffer A buffer containing the sample data to be analyzed
/// @param len    The length of the sample data
/// @param format The format of the sample data.
void *ODDecodeBlockFile::CalcSummary(samplePtr buffer, sampleCount len,
                             sampleFormat format)
{
   char* localFullSummary = new char[mSummaryInfo.totalSummaryBytes];

   memcpy(localFullSummary, bheaderTag, bheaderTagLen);

   float *summary64K = (float *)(localFullSummary + mSummaryInfo.offset64K);
   float *summary256 = (float *)(localFullSummary + mSummaryInfo.offset256);

   float *fbuffer;
   
   //mchinen: think we can hack this - don't allocate and copy if we don't need to.,
   if(format==floatSample)
   {
      fbuffer = (float*)buffer;
   }
   else
   {
      fbuffer = new float[len];
      CopySamples(buffer, format,
               (samplePtr)fbuffer, floatSample, len);
   }
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
   
   for (i = sumLen; i < mSummaryInfo.frames256; i++) {
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
   for (i = sumLen; i < mSummaryInfo.frames64K; i++) {
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

   mMin = min;
   mMax = max;
   mRMS = sqrt(sumsq / sumLen);
   

   //if we've used the float sample..
   if(format!=floatSample)
   {
      delete[] fbuffer;
   }
   return localFullSummary;
}




/// Reads the specified data from the aliased file, using libsndfile,
/// and converts it to the given sample format.
///
/// @param data   The buffer to read the sample data into.
/// @param format The format to convert the data into
/// @param start  The offset within the block to begin reading
/// @param len    The number of samples to read
int ODDecodeBlockFile::ReadData(samplePtr data, sampleFormat format,
                                sampleCount start, sampleCount len)
{
   
   if(IsSummaryAvailable())
      return SimpleBlockFile::ReadData(data,format,start,len);
   else
      return 0; //TODO: do we need to zero out the data?  It would be more efficient if the client code checked
               //ISSummaryAvailable first.
}

/// Read the summary of this alias block from disk.  Since the audio data
/// is elsewhere, this consists of reading the entire summary file.
///
/// @param *data The buffer where the summary data will be stored.  It must
///              be at least mSummaryInfo.totalSummaryBytes long.
bool ODDecodeBlockFile::ReadSummary(void *data)
{
//   wxFFile summaryFile(mFileName.GetFullPath(), wxT("rb"));
//   wxLogNull *silence=0;
//   
//   if( !summaryFile.IsOpened() ){
//
//      // new model; we need to return valid data
//      memset(data,0,(size_t)mSummaryInfo.totalSummaryBytes);
//   
//      // we silence the logging for this operation in this object
//      // after first occurrence of error; it's already reported and
//      // spewing at the user will complicate the user's ability to
//      // deal
//      mSilentLog=TRUE;
//      return true;
//
//   }else mSilentLog=FALSE; // worked properly, any future error is new 
//
//   int read = summaryFile.Read(data, (size_t)mSummaryInfo.totalSummaryBytes);
//
//   FixSummary(data);
//
//   return (read == mSummaryInfo.totalSummaryBytes);
return true;
}
