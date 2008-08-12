/**********************************************************************

  Audacity: A Digital Audio Editor

  ODDecodeBlockFile.h
  
  Created by Michael Chinen (mchinen)
  Audacity(R) is copyright (c) 1999-2008 Audacity Team.
  License: GPL v2.  See License.txt.

******************************************************************//**

\class ODDecodeBlockFile
\brief ODDecodeBlockFile is a special type of SimpleBlockFile that does not necessarily have summary OR audio data available
The summary and audio is eventually computed and written to a file in a background thread. 

Load On-Demand implementation of the SimpleBlockFIle for audio files that need to be decoded (mp3,flac,etc..).
   
Also, see ODPCMAliasBlockFile for a similar file.
*//*******************************************************************/






#ifndef __AUDACITY_ODDecodeBlockFile__
#define __AUDACITY_ODDecodeBlockFile__

#include "SimpleBlockFile.h"
#include "../BlockFile.h"
#include "../ondemand/ODTaskThread.h"
#include "../DirManager.h"
#include "../ondemand/ODDecodeTask.h"
#include <wx/thread.h>

/// An AliasBlockFile that references uncompressed data in an existing file 
class ODDecodeBlockFile : public SimpleBlockFile
{
 public:

   // Constructor / Destructor

   /// Create a disk file and write summary and sample data to it
   ODDecodeBlockFile(wxFileName baseFileName, wxFileName audioFileName,
                   samplePtr sampleData, sampleCount sampleLen,
                   sampleFormat format,
                   bool allowDeferredWrite = false);
   /// Create the memory structure to refer to the given block file
   ODDecodeBlockFile(wxFileName existingFile, wxFileName audioFileName, sampleCount len,
                   float min, float max, float rms);

   virtual ~ODDecodeBlockFile();   
   //checks to see if summary data has been computed and written to disk yet.  Thread safe.  Blocks if we are writing summary data.
   virtual bool IsSummaryAvailable();
   
   /// Returns TRUE if this block's complete data is ready to be accessed by Read()
   virtual bool IsDataAvailable();
   
   /// Returns TRUE if the summary has not yet been written, but is actively being computed and written to disk 
   virtual bool IsSummaryBeingComputed(){return false;}
   
   //Calls that rely on summary files need to be overidden
   virtual wxLongLong GetSpaceUsage();
   /// Gets extreme values for the specified region
   virtual void GetMinMax(sampleCount start, sampleCount len,
                          float *outMin, float *outMax, float *outRMS);
   /// Gets extreme values for the entire block
   virtual void GetMinMax(float *outMin, float *outMax, float *outRMS);
   /// Returns the 256 byte summary data block
   virtual bool Read256(float *buffer, sampleCount start, sampleCount len);
   /// Returns the 64K summary data block
   virtual bool Read64K(float *buffer, sampleCount start, sampleCount len);

   ///Makes new ODPCMAliasBlockFile or PCMAliasBlockFile depending on summary availability
   virtual BlockFile *Copy(wxFileName fileName);
   
   ///Saves as xml ODPCMAliasBlockFile or PCMAliasBlockFile depending on summary availability
   virtual void SaveXML(XMLWriter &xmlFile);

   ///Reconstructs from XML a ODPCMAliasBlockFile and reschedules it for OD loading
   static BlockFile *BuildFromXML(DirManager &dm, const wxChar **attrs);

   ///Writes the summary file if summary data is available
   virtual void Recover(void);
   
   ///A public interface to WriteSummary
   void DoWriteSummary(){WriteSummary();}
   
   ///Sets the value that indicates where the first sample in this block corresponds to the global sequence/clip.  Only for display use.
   void SetStart(sampleCount startSample){mStart = startSample;}
   
   ///Gets the value that indicates where the first sample in this block corresponds to the global sequence/clip.  Only for display use.
   sampleCount GetStart(){return mStart;}
   
   //Below calls are overrided just so we can take wxlog calls out, which are not threadsafe.
   
   /// Reads the specified data from the aliased file using libsndfile
   virtual int ReadData(samplePtr data, sampleFormat format,
                        sampleCount start, sampleCount len);
                 
   /// Read the summary into a buffer
   virtual bool ReadSummary(void *data);
   
   ///Returns the type of audiofile this blockfile is loaded from.
   virtual int GetDecodeType(){return mType;}
   virtual void SetDecodeType(int type){mType=type;}
   
   ///sets the amount of samples the clip associated with this blockfile is offset in the wavetrack (non effecting)
   void SetClipOffset(sampleCount numSamples){mClipOffset= numSamples;}
   
   ///Gets the number of samples the clip associated with this blockfile is offset by.  
   sampleCount GetClipOffset(){return mClipOffset;}
   
   //OD TODO:set ISAlias to true while we have no data?
   
                        

  protected:
   
   virtual void WriteSummary();
   virtual void *CalcSummary(samplePtr buffer, sampleCount len,
                             sampleFormat format);
   
   int mType;
   
                                                                                 
   ///The original file the audio came from.
   wxFileName mAudioFileName;

   ODLock    mDataAvailableMutex;
   bool mDataAvailable;
   bool mDataBeingComputed;
   
   
   ///for reporting after task is complete.  Only for display use.
   sampleCount mStart;

   ///the ODTask needs to know where this blockfile lies in the track, so for convenience, we have this here.
   sampleCount mClipOffset;

};

#endif

