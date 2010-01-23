/**********************************************************************

  Audacity: A Digital Audio Editor

  ExportPCM.cpp

  Dominic Mazzoni

**********************************************************************/

#include <wx/textctrl.h>
#include <wx/string.h>
#include <wx/window.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/timer.h>
#include <wx/intl.h>

#include "sndfile.h"

#include "../Audacity.h"
#include "../FileFormats.h"
#include "../LabelTrack.h"
#include "../Mix.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../Track.h"
#include "../WaveTrack.h"

bool ExportPCM(AudacityProject *project,
               bool stereo, wxString fName,
               bool selectionOnly, double t0, double t1)
{
   double       rate = project->GetRate();
   wxWindow    *parent = project;
   TrackList   *tracks = project->GetTracks();
   int          format = ReadExportFormatPref();
   int          formatBits = ReadExportFormatBitsPref();
   wxString     formatStr;
   SF_INFO      info;
   SNDFILE     *sf;
   int          err;

   formatStr = sf_header_name(format & SF_FORMAT_TYPEMASK);

   // Use libsndfile to export file

   info.samplerate = (unsigned int)(rate + 0.5);
   info.samples = (unsigned int)((t1 - t0)*rate + 0.5);
   info.channels = stereo? 2: 1;
   info.pcmbitwidth = formatBits;
   info.format = format;
   info.sections = 1;
   info.seekable = 0;

   // If we can't export exactly the format they requested,
   // try the default format for that header type, and try
   // 16-bit samples.
   if (!sf_format_check(&info))
      info.format = (info.format & SF_FORMAT_TYPEMASK);
   if (!sf_format_check(&info))
      info.pcmbitwidth = 16;
   if (!sf_format_check(&info)) {
      wxMessageBox(_("Cannot export audio in this format."));
      return false;
   }

   sf = sf_open_write((const char *)fName, &info);
   if (!sf) {
      wxMessageBox(wxString::Format(_("Cannot export audio to %s"),
                                    (const char *)fName));
      return false;
   }

   double timeStep = 10.0;      // write in blocks of 10 secs

   wxProgressDialog *progress = NULL;
   wxYield();
   wxStartTimer();
   wxBusyCursor busy;
   bool cancelling = false;

   double t = t0;

   while (t < t1 && !cancelling) {

      double deltat = timeStep;
      if (t + deltat > t1)
         deltat = t1 - t;

      sampleCount numSamples = int (deltat * rate + 0.5);

      Mixer *mixer = new Mixer(stereo ? 2 : 1, numSamples, true, rate);
      wxASSERT(mixer);
      mixer->Clear();

      TrackListIterator iter(tracks);
      VTrack *tr = iter.First();
      while (tr) {
         if (tr->GetKind() == VTrack::Wave) {
            if (tr->GetSelected() || !selectionOnly) {
               if (tr->GetChannel() == VTrack::MonoChannel)
                  mixer->MixMono((WaveTrack *) tr, t, t + deltat);
               if (tr->GetChannel() == VTrack::LeftChannel)
                  mixer->MixLeft((WaveTrack *) tr, t, t + deltat);
               if (tr->GetChannel() == VTrack::RightChannel)
                  mixer->MixRight((WaveTrack *) tr, t, t + deltat);
            }
         }
         tr = iter.Next();
      }

      sampleType *mixed = mixer->GetBuffer();

      sf_writef_short(sf, mixed, numSamples);

      t += deltat;

      if (!progress && wxGetElapsedTime(false) > 500) {

         wxString message;

         if (selectionOnly)
            message =
                wxString::
                Format(_("Exporting the selected audio as a %s file"),
                       (const char *) formatStr);
         else
            message =
                wxString::
                Format(_("Exporting the entire project as a %s file"),
                       (const char *) formatStr);

         progress =
             new wxProgressDialog(_("Export"),
                                  message,
                                  1000,
                                  parent,
                                  wxPD_CAN_ABORT |
                                  wxPD_REMAINING_TIME | wxPD_AUTO_HIDE);
      }
      if (progress) {
         cancelling =
             !progress->Update(int (((t - t0) * 1000) / (t1 - t0) + 0.5));
      }

      delete mixer;
   }

   err = sf_close(sf);

   if (err) {
      char buffer[1000];
      sf_error_str(sf, buffer, 1000);
      wxMessageBox(wxString::Format
                   (_("Error (file may not have been written): %s"),
                    buffer));
   }

#ifdef __WXMAC__

   FSSpec spec;

   wxMacFilename2FSSpec(fName, &spec);

   FInfo finfo;
   if (FSpGetFInfo(&spec, &finfo) == noErr) {
      finfo.fdType = sf_header_mactype(format & SF_FORMAT_TYPEMASK);
      finfo.fdCreator = AUDACITY_CREATOR;

      FSpSetFInfo(&spec, &finfo);
   }
#endif

   if (progress)
      delete progress;

   return true;
}
