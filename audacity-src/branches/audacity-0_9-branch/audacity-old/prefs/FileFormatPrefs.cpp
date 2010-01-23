/**********************************************************************

  Audacity: A Digital Audio Editor

  FileFormatPrefs.cpp

  Joshua Haberman

**********************************************************************/

#include <wx/window.h>
#include <wx/statbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/button.h>

#include "../Prefs.h"
#include "../Track.h"
#include "../ExportMP3.h"
#include "FileFormatPrefs.h"

wxString gCopyOrEditOptions[] = {
   "Make a copy of the file to edit",
   "Edit the original in place"
};

wxString gDefaultExportFormatOptions[] = {
   "AIFF",
#ifdef __WXMAC__
   "AIFF with track markers",
#endif
   "WAV",
   "IRCAM",
   "AU",
   "***last"
};

#define ID_MP3_FIND_BUTTON   7001

BEGIN_EVENT_TABLE(FileFormatPrefs, wxPanel)
   EVT_BUTTON(ID_MP3_FIND_BUTTON, FileFormatPrefs::OnMP3FindButton)
END_EVENT_TABLE()

FileFormatPrefs::FileFormatPrefs(wxWindow * parent):
PrefsPanel(parent)
{
   /* Read existing config... */

   wxString copyEdit =
       gPrefs->Read("/FileFormats/CopyOrEditUncompressedData", "edit");

   int copyEditPos = 1; // Fall back to edit if it doesn't match anything else
   if (copyEdit.IsSameAs("copy", false))
      copyEditPos = 0;


   wxString defaultFormat =
       gPrefs->Read("/FileFormats/DefaultExportFormat", "WAV");

   mNumFormats = 1;
   while (gDefaultExportFormatOptions[mNumFormats] != "***last")
      mNumFormats++;

   long mp3Bitrate = gPrefs->Read("/FileFormats/MP3Bitrate", 128);
   wxString mp3BitrateString = wxString::Format("%d", mp3Bitrate);

   int formatPos = 1;                     // Fall back to WAV
   for (int i = 0; i < mNumFormats; i++)
      if (defaultFormat.IsSameAs(gDefaultExportFormatOptions[i], false)) {
         formatPos = i;
         break;
      }

   long oggQuality = gPrefs->Read("/FileFormats/OggExportQuality", 50)/10;

   /* Begin layout code... */

   topSizer = new wxStaticBoxSizer(
      new wxStaticBox(this, -1, "File Format Settings"),
      wxVERTICAL );

   {
      wxStaticBoxSizer *copyOrEditSizer = new wxStaticBoxSizer(
         new wxStaticBox(this, -1, "When importing uncompressed audio files"),
         wxVERTICAL );

      mCopyOrEdit[0] = new wxRadioButton(
         this, -1, "Make a copy of the file to edit",
         wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
          
      copyOrEditSizer->Add( mCopyOrEdit[0], 0,
         wxGROW|wxLEFT | wxRIGHT, RADIO_BUTTON_BORDER );

      mCopyOrEdit[1] = new wxRadioButton(
         this, -1, "Edit the original in place",
         wxDefaultPosition, wxDefaultSize, 0 );

	  mCopyOrEdit[0]->SetValue(false);
	  mCopyOrEdit[1]->SetValue(false);
   
      copyOrEditSizer->Add( mCopyOrEdit[1], 0,
         wxGROW|wxLEFT | wxRIGHT, RADIO_BUTTON_BORDER );

      topSizer->Add( copyOrEditSizer, 0, wxGROW|wxALL, TOP_LEVEL_BORDER );
   }

   {
      wxStaticBoxSizer *defFormatSizer = new wxStaticBoxSizer(
         new wxStaticBox(this, -1, "Uncompressed Export Format"),
         wxVERTICAL);

      mDefaultExportFormats[0] = new wxRadioButton(
         this, -1, gDefaultExportFormatOptions[0],
         wxDefaultPosition, wxDefaultSize, wxRB_GROUP);

      /* I have no idea why this is necessary, but it always gets set on
       * Windows if this isn't here! */
      mDefaultExportFormats[0]->SetValue(false);

      defFormatSizer->Add(
         mDefaultExportFormats[0], 0,
         wxGROW|wxLEFT | wxRIGHT, RADIO_BUTTON_BORDER);

      for(int i = 1; i < mNumFormats; i++) {
         mDefaultExportFormats[i] = new wxRadioButton(
            this, -1, gDefaultExportFormatOptions[i]);

         defFormatSizer->Add(
            mDefaultExportFormats[i], 
            0, wxGROW|wxLEFT | wxRIGHT, RADIO_BUTTON_BORDER);
      }

      topSizer->Add(
         defFormatSizer, 0, 
         wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, TOP_LEVEL_BORDER );
   }

   {
      wxStaticBoxSizer *mp3SetupSizer = new wxStaticBoxSizer(
         new wxStaticBox(this, -1, "MP3 Export Setup"),
         wxVERTICAL);

      {
         wxFlexGridSizer *mp3InfoSizer = new wxFlexGridSizer(0, 3, 0, 0);
         mp3InfoSizer->AddGrowableCol(1);

         mp3InfoSizer->Add(
            new wxStaticText(this, -1, "MP3 Library Version:"), 0, 
            wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, GENERIC_CONTROL_BORDER);

         mMP3Version = new wxStaticText(this, -1, "CAPITAL LETTERS");
         SetMP3VersionText();

         mp3InfoSizer->Add(mMP3Version, 0,
            wxALIGN_CENTER_VERTICAL|wxALL, GENERIC_CONTROL_BORDER);
         
         mMP3FindButton = new wxButton(this, ID_MP3_FIND_BUTTON, "Find Library");
         
         mp3InfoSizer->Add(mMP3FindButton, 0,
                           wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, GENERIC_CONTROL_BORDER);

         if(GetMP3Exporter()->GetConfigurationCaps() & MP3CONFIG_BITRATE) {
            mp3InfoSizer->Add(
               new wxStaticText(this, -1, "Bit Rate:"), 0,
               wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, GENERIC_CONTROL_BORDER);
            wxString bitrates[] = { "16", "24", "32", "40", "48", "56", "64",
                                    "80", "96", "112", "128", "160",
                                    "192", "224", "256", "320" };
            int numBitrates = 16;

            mMP3Bitrate = new wxChoice(
               this, -1, wxDefaultPosition, wxDefaultSize, numBitrates, bitrates);

            mp3InfoSizer->Add(mMP3Bitrate, 0, 
               wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, GENERIC_CONTROL_BORDER);

            mMP3Bitrate->SetStringSelection(mp3BitrateString);
            if(mMP3Bitrate->GetSelection() == -1)
               mMP3Bitrate->SetStringSelection("128");

            if(!GetMP3Exporter()->ValidLibraryLoaded())
               mMP3Bitrate->Enable(false);
            
            mp3InfoSizer->Add( 0, 0 );  /* we have nothing to put in the third column */
         }

         /* this code works, but at the moment it's not worth it to work out
          * the remaining details. (what should the default be, does it get
          * stored, etc.)
         
         if(GetMP3Exporter()->GetConfigurationCaps() & MP3CONFIG_QUALITY) {
            mp3InfoSizer->Add(
               new wxStaticText(this, -1, "Quality:"), 0,
               wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, GENERIC_CONTROL_BORDER);

            mMP3Quality = new wxSlider(this, -1, GetMP3Exporter()->GetQuality(), 1,
                  GetMP3Exporter()->GetQualityVariance());
            mp3InfoSizer->Add(mMP3Quality, 0,
                  wxGROW|wxALIGN_CENTER_VERTICAL|wxALL,GENERIC_CONTROL_BORDER);

            if(!GetMP3Exporter()->ValidLibraryLoaded())
               mMP3Quality->Enable(false);
         }*/


         mp3SetupSizer->Add(
            mp3InfoSizer, 0, wxGROW|wxALL, 0);
      }

      {

         wxStaticBoxSizer *OGGFormatSizer = new wxStaticBoxSizer(
            new wxStaticBox(this, -1, "OGG Export Setup"),
            wxHORIZONTAL);
         mOGGQuality = new wxSlider(this, -1, oggQuality, 0, 10,
                               wxDefaultPosition, wxDefaultSize,
                               wxSL_LABELS);
         OGGFormatSizer->Add(new wxStaticText(this, -1, "OGG Quality:"), 0, 
                             wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL,
                             GENERIC_CONTROL_BORDER);
         OGGFormatSizer->Add(mOGGQuality, 1,
                             wxALL|wxGROW, GENERIC_CONTROL_BORDER);

         topSizer->Add(
            OGGFormatSizer, 0, 
            wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, TOP_LEVEL_BORDER );
      }

      topSizer->Add(
         mp3SetupSizer, 0,
         wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, TOP_LEVEL_BORDER);
   }


   SetAutoLayout(true);
   SetSizer(topSizer);

   topSizer->Fit(this);
   topSizer->SetSizeHints(this);

   /* set controls to match existing configuration... */

   mCopyOrEdit[copyEditPos]->SetValue(true);
   mDefaultExportFormats[formatPos]->SetValue(true);
}

void FileFormatPrefs::SetMP3VersionText()
{
   wxString versionString;
   bool doMP3 = true;
   
   doMP3 = GetMP3Exporter()->LoadLibrary();

   if (doMP3)
      doMP3 = GetMP3Exporter()->ValidLibraryLoaded();

   if(doMP3)
      versionString = GetMP3Exporter()->GetLibraryVersion();
   else
      versionString = "MP3 exporting plugin not found";
   
   mMP3Version->SetLabel(versionString);
}
         
void FileFormatPrefs::OnMP3FindButton(wxCommandEvent& evt)
{
   wxString oldPath = gPrefs->Read("/MP3/MP3LibPath", "");
 
   gPrefs->Write("/MP3/MP3LibPath", wxString(""));
   
   if (GetMP3Exporter()->FindLibrary(this))
      SetMP3VersionText();
   else {
      gPrefs->Write("/MP3/MP3LibPath", oldPath);
   }
   
   if(GetMP3Exporter()->GetConfigurationCaps() & MP3CONFIG_BITRATE)
      mMP3Bitrate->Enable(GetMP3Exporter()->ValidLibraryLoaded());
}

bool FileFormatPrefs::Apply()
{
   int pos;
   wxString copyEditString[] = { "copy", "edit" };

   pos = mCopyOrEdit[0]->GetValue() ? 0 : 1;
   wxString copyOrEdit = copyEditString[pos];

   for(int i = 0; i < mNumFormats; i++)
      if(mDefaultExportFormats[i]->GetValue()) {
         pos = i;
         break;
      }
   
   gPrefs->SetPath("/FileFormats");

   wxString originalExportFormat = gPrefs->Read("DefaultExportFormat", "");
   wxString defaultExportFormat = gDefaultExportFormatOptions[pos];

   gPrefs->Write("CopyOrEditUncompressedData", copyOrEdit);
   gPrefs->Write("DefaultExportFormat", defaultExportFormat);

   if(GetMP3Exporter()->GetConfigurationCaps() & MP3CONFIG_BITRATE) {
      long bitrate;
      mMP3Bitrate->GetStringSelection().ToLong(&bitrate);
      gPrefs->Write("MP3Bitrate", bitrate);
   }


   long oggQuality = mOGGQuality->GetValue();
   gPrefs->Write("/FileFormats/OggExportQuality", (long)(oggQuality * 10));
   
   if (originalExportFormat != defaultExportFormat) {
      // Force the menu bar to get recreated
      gMenusDirty++;
   }

   gPrefs->SetPath("/");
   return true;

}


FileFormatPrefs::~FileFormatPrefs()
{
}
