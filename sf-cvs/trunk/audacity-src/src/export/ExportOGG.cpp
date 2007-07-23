/**********************************************************************

  Audacity: A Digital Audio Editor

  ExportOGG.cpp

  Joshua Haberman

  This program is distributed under the GNU General Public License, version 2.
  A copy of this license is included with this source.

  Portions from vorbis-tools, copyright 2000-2002 Michael Smith
  <msmith@labyrinth.net.au>; Vorbize, Kenneth Arnold <kcarnold@yahoo.com>;
  and libvorbis examples, Monty <monty@xiph.org>

**********************************************************************/

#include "../Audacity.h"

#ifdef USE_LIBVORBIS

#include "Export.h"

#include <wx/log.h>
#include <wx/msgdlg.h>

#include <vorbis/vorbisenc.h>

#include "../FileIO.h"
#include "../Project.h"
#include "../Mix.h"
#include "../Prefs.h"

#include "../Internat.h"
#include "../Tags.h"

//----------------------------------------------------------------------------
// ExportOGGOptions
//----------------------------------------------------------------------------

class ExportOGGOptions : public wxDialog
{
public:

   ExportOGGOptions(wxWindow *parent);
   void PopulateOrExchange(ShuttleGui & S);
   void OnOK(wxCommandEvent& event);

private:

   int mOggQualityUnscaled;

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(ExportOGGOptions, wxDialog)
   EVT_BUTTON(wxID_OK, ExportOGGOptions::OnOK)
END_EVENT_TABLE()

/// 
/// 
ExportOGGOptions::ExportOGGOptions(wxWindow *parent)
:  wxDialog(NULL, wxID_ANY,
            wxString(_("Specify Ogg Vorbis Options")),
            wxDefaultPosition, wxDefaultSize,
            wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP)
{
   ShuttleGui S(this, eIsCreatingFromPrefs);

   mOggQualityUnscaled = gPrefs->Read(wxT("/FileFormats/OggExportQuality"),50)/10;

   PopulateOrExchange(S);
}

/// 
/// 
void ExportOGGOptions::PopulateOrExchange(ShuttleGui & S)
{
   S.StartHorizontalLay(wxEXPAND, 0);
   {
      S.StartStatic(_("Ogg Vorbis Export Setup"), 1);
      {
         S.StartMultiColumn(2, wxEXPAND);
         {
            S.SetStretchyCol(1);
            S.TieSlider(_("Quality:"), mOggQualityUnscaled, 10);
         }
         S.EndMultiColumn();
      }
      S.EndStatic();
   }
   S.EndHorizontalLay();
   S.StartHorizontalLay(wxALIGN_CENTER, false);
   {
#if defined(__WXGTK20__) || defined(__WXMAC__)
      S.Id(wxID_CANCEL).AddButton(_("&Cancel"));
      S.Id(wxID_OK).AddButton(_("&OK"))->SetDefault();
#else
      S.Id(wxID_OK).AddButton(_("&OK"))->SetDefault();
      S.Id(wxID_CANCEL).AddButton(_("&Cancel"));
#endif
   }
   GetSizer()->AddSpacer(5);
   Layout();
   Fit();
   SetMinSize(GetSize());
   Center();

   return;
}

/// 
/// 
void ExportOGGOptions::OnOK(wxCommandEvent& event)
{
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   gPrefs->Write(wxT("/FileFormats/OggExportQuality"),mOggQualityUnscaled * 10);

   EndModal(wxID_OK);

   return;
}

//----------------------------------------------------------------------------
// ExportOGG
//----------------------------------------------------------------------------

#define SAMPLES_PER_RUN 8192

class ExportOGG : public ExportPlugin
{
public:

   ExportOGG();
   void Destroy();

   // Required

   bool DisplayOptions(AudacityProject *project = NULL);
   bool Export(AudacityProject *project,
               int channels,
               wxString fName,
               bool selectedOnly,
               double t0,
               double t1,
               MixerSpec *mixerSpec = NULL);
};

ExportOGG::ExportOGG()
:  ExportPlugin()
{
   SetFormat(wxT("OGG"));
   SetExtension(wxT("ogg"));
   SetMaxChannels(255);
   SetCanMetaData(true);
   SetDescription(_("Ogg Vorbis Files"));
}

void ExportOGG::Destroy()
{
   delete this;
}

bool ExportOGG::Export(AudacityProject *project,
                       int numChannels,
                       wxString fName,
                       bool selectionOnly,
                       double t0,
                       double t1,
                       MixerSpec *mixerSpec)
{
   double    rate    = project->GetRate();
   TrackList *tracks = project->GetTracks();
   double    quality = (gPrefs->Read(wxT("/FileFormats/OggExportQuality"), 50)/(float)100.0);

   wxLogNull logNo;            // temporarily disable wxWindows error messages 
   bool      cancelling = false;
   int       eos = 0;

   FileIO outFile(fName, FileIO::Output);

   if (!outFile.IsOpened()) {
      wxMessageBox(_("Unable to open target file for writing"));
      return false;
   }

   // All the Ogg and Vorbis encoding data
   ogg_stream_state stream;
   ogg_page         page;
   ogg_packet       packet;

   vorbis_info      info;
   vorbis_comment   comment;
   vorbis_dsp_state dsp;
   vorbis_block     block;

   // Encoding setup
   vorbis_info_init(&info);
   vorbis_encode_init_vbr(&info, numChannels, int(rate + 0.5), quality);

   // Retrieve tags
   Tags *tags = project->GetTags();
   if (tags->IsEmpty() && project->GetShowId3Dialog()) {
      if (!tags->ShowEditDialog(project,
                                _("Edit the metadata for the OGG file"),
                                true)) {
         return false;  // user selected "cancel"
      }
   }
   tags->ExportOGGTags(&comment);

   // Set up analysis state and auxiliary encoding storage
   vorbis_analysis_init(&dsp, &info);
   vorbis_block_init(&dsp, &block);

   // Set up packet->stream encoder.  According to encoder example,
   // a random serial number makes it more likely that you can make
   // chained streams with concatenation.
   srand(time(NULL));
   ogg_stream_init(&stream, rand());

   // First we need to write the required headers:
   //    1. The Ogg bitstream header, which contains codec setup params
   //    2. The Vorbis comment header
   //    3. The bitstream codebook.
   //
   // After we create those our responsibility is complete, libvorbis will
   // take care of any other ogg bistream constraints (again, according
   // to the example encoder source)
   ogg_packet bitstream_header;
   ogg_packet comment_header;
   ogg_packet codebook_header;

   vorbis_analysis_headerout(&dsp, &comment, &bitstream_header, &comment_header,
         &codebook_header);

   // Place these headers into the stream
   ogg_stream_packetin(&stream, &bitstream_header);
   ogg_stream_packetin(&stream, &comment_header);
   ogg_stream_packetin(&stream, &codebook_header);

   // Flushing these headers now guarentees that audio data will
   // start on a new page, which apparently makes streaming easier
   while (ogg_stream_flush(&stream, &page)) {
      outFile.Write(page.header, page.header_len);
      outFile.Write(page.body, page.body_len);
   }

   int numWaveTracks;
   WaveTrack **waveTracks;
   tracks->GetWaveTracks(selectionOnly, &numWaveTracks, &waveTracks);
   Mixer *mixer = new Mixer(numWaveTracks, waveTracks,
                            tracks->GetTimeTrack(),
                            t0, t1,
                            numChannels, SAMPLES_PER_RUN, false,
                            rate, floatSample, true, mixerSpec);

   GetActiveProject()->ProgressShow(_("Export"),
                                    selectionOnly ?
                                    _("Exporting the selected audio as Ogg Vorbis") :
                                    _("Exporting the entire project as Ogg Vorbis"));

   while (!cancelling && !eos) {
      float **vorbis_buffer = vorbis_analysis_buffer(&dsp, SAMPLES_PER_RUN);
      sampleCount samplesThisRun = mixer->Process(SAMPLES_PER_RUN);

      if (samplesThisRun == 0) {
         // Tell the library that we wrote 0 bytes - signalling the end.
         vorbis_analysis_wrote(&dsp, 0);
      }
      else {
         
         for (int i = 0; i < numChannels; i++) {
            float *temp = (float *)mixer->GetBuffer(i);
            memcpy(vorbis_buffer[i], temp, sizeof(float)*SAMPLES_PER_RUN);
         }

         // tell the encoder how many samples we have
         vorbis_analysis_wrote(&dsp, samplesThisRun);
      }

      // I don't understand what this call does, so here is the comment
      // from the example, verbatim:
      //
      //    vorbis does some data preanalysis, then divvies up blocks
      //    for more involved (potentially parallel) processing. Get
      //    a single block for encoding now
      while (vorbis_analysis_blockout(&dsp, &block) == 1) {

         // analysis, assume we want to use bitrate management
         vorbis_analysis(&block, NULL);
         vorbis_bitrate_addblock(&block);

         while (vorbis_bitrate_flushpacket(&dsp, &packet)) {

            // add the packet to the bitstream
            ogg_stream_packetin(&stream, &packet);

            // From vorbis-tools-1.0/oggenc/encode.c:
            //   If we've gone over a page boundary, we can do actual output,
            //   so do so (for however many pages are available).

            while (!eos) {
					int result = ogg_stream_pageout(&stream, &page);
					if (!result) {
                  break;
               }

               outFile.Write(page.header, page.header_len);
               outFile.Write(page.body, page.body_len);

               if (ogg_page_eos(&page)) {
                  eos = 1;
               }
				}
         }
      }

      int progressvalue = int (1000 * ((mixer->MixGetCurrentTime()-t0) /
                                       (t1-t0)));
      cancelling = !GetActiveProject()->ProgressUpdate(progressvalue);
   }
   GetActiveProject()->ProgressHide();

   delete mixer;

	ogg_stream_clear(&stream);

	vorbis_block_clear(&block);
	vorbis_dsp_clear(&dsp);
	vorbis_info_clear(&info);

   outFile.Close();

   return !cancelling;
}

bool ExportOGG::DisplayOptions(AudacityProject *project)
{
   ExportOGGOptions od(project);

   od.ShowModal();

   return true;
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
ExportPlugin *New_ExportOGG()
{
   return new ExportOGG();
}

#endif // USE_LIBVORBIS


// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: 33184ece-e482-44d9-9ff3-b4a11b41112b

