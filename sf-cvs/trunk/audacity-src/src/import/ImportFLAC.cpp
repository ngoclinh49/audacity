/**********************************************************************

  Audacity: A Digital Audio Editor

  ImportFLAC.cpp

  Copyright 2004  Sami Liedes
  Leland Lucius

  Based on ImportPCM.cpp by Dominic Mazzoni
  Licensed under the GNU General Public License v2 or later

*//****************************************************************//**

\class FLACImportFileHandle
\brief An ImportFileHandle for FLAC data

*//****************************************************************//**

\class FLACImportPlugin
\brief An ImportPlugin for FLAC data

*//*******************************************************************/

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/window.h>
#endif

#include <wx/defs.h>
#include <wx/intl.h>		// needed for _("translated stings") even if we
							// don't have libflac available
#include "../Audacity.h"

#include "Import.h"
#include "ImportPlugin.h"

#include "../Tags.h"

#define FLAC_HEADER "fLaC"

#define DESC _("FLAC files")

static const wxChar *exts[] =
{
   wxT("flac"),
   wxT("flc")
};

#ifndef USE_LIBFLAC

void GetFLACImportPlugin(ImportPluginList *importPluginList,
                        UnusableImportPluginList *unusableImportPluginList)
{
   UnusableImportPlugin* flacIsUnsupported =
      new UnusableImportPlugin(DESC, wxArrayString(WXSIZEOF(exts), exts));

   unusableImportPluginList->Append(flacIsUnsupported);
}

#else /* USE_LIBFLAC */

#include "../Internat.h"
#include "ImportFLAC.h"

#include <wx/string.h>
#include <wx/utils.h>
#include <wx/file.h>
#include <wx/ffile.h>

#include "FLAC++/decoder.h"

#include "../FileFormats.h"
#include "../Prefs.h"
#include "../WaveTrack.h"
#include "ImportPlugin.h"

/* FLACPP_API_VERSION_CURRENT is 6 for libFLAC++ from flac-1.1.3 (see <FLAC++/export.h>) */
#if !defined FLACPP_API_VERSION_CURRENT || FLACPP_API_VERSION_CURRENT < 6
#define LEGACY_FLAC
#else
#undef LEGACY_FLAC
#endif

class FLACImportFileHandle;

class MyFLACFile : public FLAC::Decoder::File
{
 public:
   MyFLACFile(FLACImportFileHandle *handle) : mFile(handle)
   {
      mWasError = false;
   }
   
   bool get_was_error() const
   {
      return mWasError;
   }
 private:
   friend class FLACImportFileHandle;
   FLACImportFileHandle *mFile;
   bool                  mWasError;
   wxArrayString         mComments;
 protected:
   virtual FLAC__StreamDecoderWriteStatus write_callback(const FLAC__Frame *frame,
							 const FLAC__int32 * const buffer[]);
   virtual void metadata_callback(const FLAC__StreamMetadata *metadata);
   virtual void error_callback(FLAC__StreamDecoderErrorStatus status);
};


class FLACImportPlugin : public ImportPlugin
{
 public:
   FLACImportPlugin():
      ImportPlugin(wxArrayString(WXSIZEOF(exts), exts))
   {
   }

   ~FLACImportPlugin() { }

   wxString GetPluginFormatDescription();
   ImportFileHandle *Open(wxString Filename);
};


class FLACImportFileHandle : public ImportFileHandle
{
   friend class MyFLACFile;
public:
   FLACImportFileHandle(const wxString & name);
   ~FLACImportFileHandle();

   bool Init();

   wxString GetFileDescription();
   int GetFileUncompressedBytes();
   int Import(TrackFactory *trackFactory, Track ***outTracks,
              int *outNumTracks, Tags *tags);
private:
   sampleFormat          mFormat;
   MyFLACFile           *mFile;
   unsigned long         mSampleRate;
   unsigned long         mNumChannels;
   unsigned long         mBitsPerSample;
   FLAC__uint64          mNumSamples;
   FLAC__uint64          mSamplesDone;
   bool                  mStreamInfoDone;
   bool                  mCancelled;
   WaveTrack           **mChannels;
};


void MyFLACFile::metadata_callback(const FLAC__StreamMetadata *metadata)
{
   switch (metadata->type)
   {
      case FLAC__METADATA_TYPE_VORBIS_COMMENT:
         for (FLAC__uint32 i = 0; i < metadata->data.vorbis_comment.num_comments; i++) {
            mComments.Add(UTF8CTOWX((char *)metadata->data.vorbis_comment.comments[i].entry));
         }
      break;

      case FLAC__METADATA_TYPE_STREAMINFO:
         mFile->mSampleRate=metadata->data.stream_info.sample_rate;
         mFile->mNumChannels=metadata->data.stream_info.channels;
         mFile->mBitsPerSample=metadata->data.stream_info.bits_per_sample;
         mFile->mNumSamples=metadata->data.stream_info.total_samples;

         if (mFile->mBitsPerSample<=16) {
            if (mFile->mFormat<int16Sample) {
               mFile->mFormat=int16Sample;
            }
         } else if (mFile->mBitsPerSample<=24) {
            if (mFile->mFormat<int24Sample) {
               mFile->mFormat=int24Sample;
            }
         } else {
            mFile->mFormat=floatSample;
         }
         mFile->mStreamInfoDone=true;
      break;
	  // handle the other types we do nothing with to avoid a warning
	  case FLAC__METADATA_TYPE_PADDING:	// do nothing with padding
	  case FLAC__METADATA_TYPE_APPLICATION:	// no idea what to do with this
	  case FLAC__METADATA_TYPE_SEEKTABLE:	// don't need a seektable here
	  case FLAC__METADATA_TYPE_CUESHEET:	// convert this to labels?
	  case FLAC__METADATA_TYPE_PICTURE:		// ignore pictures
	  case FLAC__METADATA_TYPE_UNDEFINED:	// do nothing with this either
	  break;
   }
}

void MyFLACFile::error_callback(FLAC__StreamDecoderErrorStatus status)
{
   mWasError = true;
   
   /*
   switch (status)
   {
   case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
      wxPrintf(wxT("Flac Error: Lost sync\n"));
      break;
   case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
      wxPrintf(wxT("Flac Error: Crc mismatch\n"));
      break;
   case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
      wxPrintf(wxT("Flac Error: Bad Header\n"));
      break;
   default:
      wxPrintf(wxT("Flac Error: Unknown error code\n"));
      break;
   }*/
}

FLAC__StreamDecoderWriteStatus MyFLACFile::write_callback(const FLAC__Frame *frame,
							  const FLAC__int32 * const buffer[])
{
   short *tmp=new short[frame->header.blocksize];

   for (unsigned int chn=0; chn<mFile->mNumChannels; chn++) {
      if (frame->header.bits_per_sample == 16) {
         for (unsigned int s=0; s<frame->header.blocksize; s++) {
            tmp[s]=buffer[chn][s];
         }

         mFile->mChannels[chn]->Append((samplePtr)tmp,
                  int16Sample,
                  frame->header.blocksize);
      }
      else {
         mFile->mChannels[chn]->Append((samplePtr)buffer[chn],
                  int24Sample,
                  frame->header.blocksize);
      }
   }

   delete [] tmp;

   mFile->mSamplesDone += frame->header.blocksize;

   if (!mFile->mProgress->Update(mFile->mSamplesDone, mFile->mNumSamples)) {
      mFile->mCancelled = true;
      return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
   }

   return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


void GetFLACImportPlugin(ImportPluginList *importPluginList,
			 UnusableImportPluginList *unusableImportPluginList)
{
   importPluginList->Append(new FLACImportPlugin);
}


wxString FLACImportPlugin::GetPluginFormatDescription()
{
    return DESC;
}


ImportFileHandle *FLACImportPlugin::Open(wxString filename)
{
   // First check if it really is a FLAC file
   
   wxFile binaryFile;
   if (!binaryFile.Open(filename))
      return false; // File not found

   char buf[5];
   int num_bytes = binaryFile.Read(buf, 4);
   buf[num_bytes] = 0;
   if (strcmp(buf, FLAC_HEADER) != 0)
   {
      // File is not a FLAC file
      binaryFile.Close();
      return false; 
   }

   binaryFile.Close();
   
   // Open the file for import
   FLACImportFileHandle *handle = new FLACImportFileHandle(filename);

   bool success = handle->Init();
   if (!success) {
      delete handle;
      return NULL;
   }

   return handle;
}


FLACImportFileHandle::FLACImportFileHandle(const wxString & name)
:  ImportFileHandle(name),
   mSamplesDone(0),
   mStreamInfoDone(false),
   mCancelled(false)
{
   mFormat = (sampleFormat)
      gPrefs->Read(wxT("/SamplingRate/DefaultProjectSampleFormat"), floatSample);
   mFile = new MyFLACFile(this);
}

bool FLACImportFileHandle::Init()
{
#ifdef LEGACY_FLAC
   bool success = mFile->set_filename(OSFILENAME(mFilename));
   if (!success) {
      return false;
   }
   mFile->set_metadata_respond(FLAC__METADATA_TYPE_STREAMINFO);
   mFile->set_metadata_respond(FLAC__METADATA_TYPE_VORBIS_COMMENT);
   FLAC::Decoder::File::State state = mFile->init();
   if (state != FLAC__FILE_DECODER_OK) {
      return false;
   }
#else
   if (mFile->init(OSFILENAME(mFilename)) != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
      return false;
   }
#endif
   mFile->process_until_end_of_metadata();

#ifdef LEGACY_FLAC
   state = mFile->get_state();
   if (state != FLAC__FILE_DECODER_OK) {
      return false;
   }
#else
   // not necessary to check state, error callback will catch errors, but here's how:
   if (mFile->get_state() > FLAC__STREAM_DECODER_READ_FRAME) {
      return false;
   }
#endif

   if (!mFile->is_valid() || mFile->get_was_error()) {
      // This probably is not a FLAC file at all
      return false;
   }
   return true;
}

wxString FLACImportFileHandle::GetFileDescription()
{
   return DESC;
}


int FLACImportFileHandle::GetFileUncompressedBytes()
{
   // TODO: Get Uncompressed byte count.
   return 0;
}


int FLACImportFileHandle::Import(TrackFactory *trackFactory,
				  Track ***outTracks,
				  int *outNumTracks,
              Tags *tags)
{
   wxASSERT(mStreamInfoDone);

   CreateProgress();

   mChannels = new WaveTrack *[mNumChannels];

   unsigned long c;
   for (c = 0; c < mNumChannels; c++) {
      mChannels[c] = trackFactory->NewWaveTrack(mFormat, mSampleRate);
      
      if (mNumChannels == 2) {
         switch (c) {
         case 0:
            mChannels[c]->SetChannel(Track::LeftChannel);
            mChannels[c]->SetLinked(true);
            break;
         case 1:
            mChannels[c]->SetChannel(Track::RightChannel);
            mChannels[c]->SetTeamed(true);
            break;
         }
      }
      else {
         mChannels[c]->SetChannel(Track::MonoChannel);
      }
   }

#ifdef LEGACY_FLAC
   bool res = (mFile->process_until_end_of_file() != 0);
#else
   bool res = (mFile->process_until_end_of_stream() != 0);
#endif

   if (!res) {
      for(c = 0; c < mNumChannels; c++) {
         delete mChannels[c];
      }
      delete[] mChannels;

      return (mCancelled ? eImportCancelled : eImportFailed);
   }
   
   *outNumTracks = mNumChannels;
   *outTracks = new Track *[mNumChannels];
   for (c = 0; c < mNumChannels; c++) {
      mChannels[c]->Flush();
      (*outTracks)[c] = mChannels[c];
   }
   delete[] mChannels;

   tags->Clear();
   size_t cnt = mFile->mComments.GetCount();
   for (c = 0; c < cnt; c++) {
      tags->SetTag(mFile->mComments[c].BeforeFirst(wxT('=')),
                   mFile->mComments[c].AfterFirst(wxT('=')));
   }

   return eImportSuccess;
}


FLACImportFileHandle::~FLACImportFileHandle()
{
   mFile->finish();
   delete mFile;
}

#endif /* USE_LIBFLAC */
