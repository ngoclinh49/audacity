/**********************************************************************

  Audacity: A Digital Audio Editor

  FileFormatPrefs.cpp

  Joshua Haberman

**********************************************************************/

#include <wx/window.h>
#include <wx/statbox.h>

#include "../Prefs.h"
#include "FileFormatPrefs.h"

wxString gCopyOrEditOptions[] = { "Make a copy of the file to edit",
   "Edit the original in place"
};
wxString gDefaultExportFormatOptions[] = { "AIFF",
#ifdef __WXMAC__
   "AIFF with track markers",
#endif
   "WAV",
   "IRCAM",
   "AU",
#ifdef USE_LIBVORBIS
   "Ogg Vorbis",
#endif
   "MP3",
   "***last"
};

FileFormatPrefs::FileFormatPrefs(wxWindow * parent):
PrefsPanel(parent)
{
   wxString copyEdit =
       gPrefs->Read("/FileFormats/CopyOrEditUncompressedData", "edit");

   int pos = 1;                 // Fall back to edit if it doesn't match anything else
   if (copyEdit.IsSameAs("copy", false))
      pos = 0;

   mEnclosingBox = new wxStaticBox(this, -1,
                                   "File Format Settings",
                                   wxPoint(0, 0), GetSize());
   mCopyOrEdit = new wxRadioBox(this, -1,
                                "When importing uncompressed audio files",
                                wxPoint(PREFS_SIDE_MARGINS,
                                        PREFS_TOP_MARGIN),
                                wxSize(GetSize().GetWidth() -
                                       PREFS_SIDE_MARGINS * 2,
                                       65), 2, gCopyOrEditOptions, 1);
   mCopyOrEdit->SetSelection(pos);

   wxString defaultFormat =
       gPrefs->Read("/FileFormats/DefaultExportFormat", "WAV");

   pos = 1;                     // Fall back to WAV
   for (int i = 0; i < 6; i++)
      if (defaultFormat.IsSameAs(gDefaultExportFormatOptions[i], false)) {
         pos = i;
         break;
      }

   int numFormats = 1;
   while (gDefaultExportFormatOptions[numFormats] != "***last")
      numFormats++;

   mDefaultExportFormat = new wxRadioBox(this, -1,
                                         "Default export format",
                                         wxPoint(PREFS_SIDE_MARGINS,
                                                 90),
                                         wxSize(GetSize().GetWidth() -
                                                PREFS_SIDE_MARGINS * 2,
                                                160),
                                         numFormats,
                                         gDefaultExportFormatOptions, 1);
   mDefaultExportFormat->SetSelection(pos);
}

bool FileFormatPrefs::Apply()
{
   wxString copyEditString[] = { "copy", "edit" };
   wxString copyOrEdit = copyEditString[mCopyOrEdit->GetSelection()];
   wxString defaultExportFormat
       = gDefaultExportFormatOptions[mDefaultExportFormat->GetSelection()];

   gPrefs->SetPath("/FileFormats");
   gPrefs->Write("CopyOrEditUncompressedData", copyOrEdit);
   gPrefs->Write("DefaultExportFormat", defaultExportFormat);
   gPrefs->SetPath("/");

   return true;

}


FileFormatPrefs::~FileFormatPrefs()
{
   delete mEnclosingBox;
   delete mCopyOrEdit;
}
