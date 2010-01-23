/**********************************************************************

  Audacity: A Digital Audio Editor

  FileFormatPrefs.cpp

  Joshua Haberman
  Dominic Mazzoni

**********************************************************************/

#include <wx/window.h>
#include <wx/statbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>
#include <wx/button.h>
#include <wx/intl.h>

#include "sndfile.h"

#include "../export/ExportMP3.h"
#include "../FileFormats.h"
#include "../Prefs.h"
#include "../Track.h"
#include "FileFormatPrefs.h"

#define ID_MP3_FIND_BUTTON         7001
#define ID_EXPORT_OPTIONS_BUTTON   7002
#define ID_FORMAT_CHOICE           7003

BEGIN_EVENT_TABLE(FileFormatPrefs, wxPanel)
   EVT_BUTTON(ID_MP3_FIND_BUTTON, FileFormatPrefs::OnMP3FindButton)
   EVT_CHOICE(ID_FORMAT_CHOICE,   FileFormatPrefs::OnFormatChoice)
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

   long mp3Bitrate = gPrefs->Read("/FileFormats/MP3Bitrate", 128);
   wxString mp3BitrateString = wxString::Format("%d", mp3Bitrate);

   mFormat = ReadExportFormatPref();
   mFormatBits = ReadExportFormatBitsPref();

   /* Begin layout code... */

   topSizer = new wxStaticBoxSizer(
      new wxStaticBox(this, -1, _("File Format Settings")),
      wxVERTICAL );

   {
      wxStaticBoxSizer *copyOrEditSizer = new wxStaticBoxSizer(
         new wxStaticBox(this, -1,
            _("When importing uncompressed audio files")),
         wxVERTICAL );

      mCopyOrEdit[0] = new wxRadioButton(
         this, -1, _("Make a copy of the file to edit"),
         wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
          
      copyOrEditSizer->Add( mCopyOrEdit[0], 0,
         wxGROW|wxLEFT | wxRIGHT, RADIO_BUTTON_BORDER );

      mCopyOrEdit[1] = new wxRadioButton(
         this, -1, _("Edit the original in place"),
         wxDefaultPosition, wxDefaultSize, 0 );

	  mCopyOrEdit[0]->SetValue(false);
	  mCopyOrEdit[1]->SetValue(false);
   
      copyOrEditSizer->Add( mCopyOrEdit[1], 0,
         wxGROW|wxLEFT | wxRIGHT, RADIO_BUTTON_BORDER );

      topSizer->Add( copyOrEditSizer, 0, wxGROW|wxALL, TOP_LEVEL_BORDER );
   }

   {
      wxStaticBoxSizer *defFormatSizer = new wxStaticBoxSizer(
         new wxStaticBox(this, -1, _("Uncompressed Export Format")),
         wxVERTICAL);

      int numSimpleFormats = sf_num_simple_formats();
      int sel = numSimpleFormats;

      wxString *formatStrings = new wxString[numSimpleFormats+1];
      for(int i=0; i<numSimpleFormats; i++) {
         formatStrings[i] = sf_simple_format(i)->name;
         if (mFormat == sf_simple_format(i)->format &&
             mFormatBits == sf_simple_format(i)->pcmbitwidth)
            sel = i;
      }
      formatStrings[numSimpleFormats] = _("Other...");

      #ifdef __WXMAC__
        // This is just to work around a wxChoice auto-sizing bug
        mDefaultExportFormat = new wxChoice(
           this, ID_FORMAT_CHOICE, wxDefaultPosition, wxSize(200,-1),
           numSimpleFormats+1, formatStrings);
      #else
        mDefaultExportFormat = new wxChoice(
           this, ID_FORMAT_CHOICE, wxDefaultPosition, wxDefaultSize,
           numSimpleFormats+1, formatStrings);
      #endif

      mDefaultExportFormat->SetSelection(sel);
        
      delete[] formatStrings;

      mFormatText = new wxStaticText(this, -1, "CAPITAL LETTERS");
      SetFormatText();
      
      defFormatSizer->Add(mDefaultExportFormat, 0,
                          wxALL, GENERIC_CONTROL_BORDER);

      defFormatSizer->Add(mFormatText, 0,
                          wxALL, GENERIC_CONTROL_BORDER);

      topSizer->Add(
         defFormatSizer, 0, 
         wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, TOP_LEVEL_BORDER );
   }

   {
      wxStaticBoxSizer *mp3SetupSizer = new wxStaticBoxSizer(
         new wxStaticBox(this, -1, _("MP3 Export Setup")),
         wxVERTICAL);

      {
         wxFlexGridSizer *mp3InfoSizer = new wxFlexGridSizer(0, 3, 0, 0);
         mp3InfoSizer->AddGrowableCol(1);

         mp3InfoSizer->Add(
            new wxStaticText(this, -1, _("MP3 Library Version:")), 0, 
            wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, GENERIC_CONTROL_BORDER);

         mMP3Version = new wxStaticText(this, -1, "CAPITAL LETTERS");
         SetMP3VersionText();

         mp3InfoSizer->Add(mMP3Version, 0,
            wxALIGN_CENTER_VERTICAL|wxALL, GENERIC_CONTROL_BORDER);
         
         mMP3FindButton = new wxButton(this, ID_MP3_FIND_BUTTON,
               _("Find Library"));
         
         mp3InfoSizer->Add(mMP3FindButton, 0,
                           wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, GENERIC_CONTROL_BORDER);

         if(GetMP3Exporter()->GetConfigurationCaps() & MP3CONFIG_BITRATE) {
            mp3InfoSizer->Add(
               new wxStaticText(this, -1, _("Bit Rate:")), 0,
               wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, GENERIC_CONTROL_BORDER);

            wxString bitrates[] = { "56", "96", "128", "192", "256", "320" };
            int numBitrates = 6;

            #ifdef __WXMAC__
            // This is just to work around a wxChoice auto-sizing bug
            mMP3Bitrate = new wxChoice(
               this, -1, wxDefaultPosition, wxSize(120,-1), numBitrates, bitrates);
            #else
            mMP3Bitrate = new wxChoice(
               this, -1, wxDefaultPosition, wxDefaultSize, numBitrates, bitrates);
            #endif

            mp3InfoSizer->Add(mMP3Bitrate, 0, 
               wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, GENERIC_CONTROL_BORDER);

            mMP3Bitrate->SetStringSelection(mp3BitrateString);
            if(mMP3Bitrate->GetSelection() == -1)
               mMP3Bitrate->SetStringSelection("128");

            if(!GetMP3Exporter()->ValidLibraryLoaded())
               mMP3Bitrate->Enable(false);
         }

         mp3SetupSizer->Add(
            mp3InfoSizer, 0, wxGROW|wxALL, 0);
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
      versionString = _("MP3 exporting plugin not found");
   
   mMP3Version->SetLabel(versionString);
}

void FileFormatPrefs::SetFormatText()
{
   wxString formatString;

   formatString = sf_header_name(mFormat & SF_FORMAT_TYPEMASK);

   formatString += wxString::Format(_(", %d-bit data"), mFormatBits);   

   formatString += ", " + sf_encoding_name(mFormat & SF_FORMAT_SUBMASK);

   mFormatText->SetLabel(formatString);
}

void FileFormatPrefs::OnFormatChoice(wxCommandEvent& evt)
{
   int sel = mDefaultExportFormat->GetSelection();
   int numSimpleFormats = sf_num_simple_formats();

   if (sel == numSimpleFormats) {
      Other();
   }
   else {
      mFormat = sf_simple_format(sel)->format;
      mFormatBits = sf_simple_format(sel)->pcmbitwidth;
   }

   SetFormatText();
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
   int originalExportFormat = ReadExportFormatPref();
   int originalExportBits = ReadExportFormatBitsPref();

   WriteExportFormatPref(mFormat);
   WriteExportFormatBitsPref(mFormatBits);
   
   gPrefs->SetPath("/FileFormats");

   wxString copyEditString[] = { "copy", "edit" };
   int pos = mCopyOrEdit[0]->GetValue() ? 0 : 1;
   wxString copyOrEdit = copyEditString[pos];
   gPrefs->Write("CopyOrEditUncompressedData", copyOrEdit);

   if(GetMP3Exporter()->GetConfigurationCaps() & MP3CONFIG_BITRATE) {
      long bitrate;
      mMP3Bitrate->GetStringSelection().ToLong(&bitrate);
      gPrefs->Write("MP3Bitrate", bitrate);
   }
   gPrefs->SetPath("/");
      
   if (originalExportFormat != mFormat ||
       originalExportBits != mFormatBits)
      gMenusDirty++;

   return true;
}

FileFormatPrefs::~FileFormatPrefs()
{
}

#define ID_HEADER_CHOICE           7101
#define ID_BITS_CHOICE             7102
#define ID_ENCODING_CHOICE         7103

class OtherFormatDialog:public wxDialog {
 public:
   // constructors and destructors
   OtherFormatDialog(wxWindow * parent, wxWindowID id,
                     unsigned int format,
                     unsigned int formatBits);

public:
   void OnChoice(wxCommandEvent & event);
   void OnOk(wxCommandEvent & event);
   void OnCancel(wxCommandEvent & event);

   void ValidateChoices();

   unsigned int  mFormat;
   unsigned int  mFormatBits;

   wxButton   *mOK;
   wxChoice   *mHeaderChoice;
   wxChoice   *mBitsChoice;
   wxChoice   *mEncodingChoice;

   DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(OtherFormatDialog, wxDialog)
   EVT_CHOICE(ID_HEADER_CHOICE,   OtherFormatDialog::OnChoice)
   EVT_CHOICE(ID_BITS_CHOICE,     OtherFormatDialog::OnChoice)
   EVT_CHOICE(ID_ENCODING_CHOICE, OtherFormatDialog::OnChoice)
   EVT_BUTTON(wxID_OK,            OtherFormatDialog::OnOk)
   EVT_BUTTON(wxID_CANCEL,        OtherFormatDialog::OnCancel)
END_EVENT_TABLE()

OtherFormatDialog::OtherFormatDialog(wxWindow * parent, wxWindowID id,
                                     unsigned int format,
                                     unsigned int formatBits):
   wxDialog(parent, id,
            _("File Format"),
            wxDefaultPosition, wxDefaultSize)
{
   mFormat = format;
   mFormatBits = formatBits;

   wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

   wxControl *item;

   item = new wxStaticText(this, -1,
                     _("(Not all combinations of headers, data sizes,\n"
                       "and encodings are possible.)"),
                     wxDefaultPosition, wxDefaultSize, 0);

   mainSizer->Add(item, 0, wxALIGN_LEFT | wxALL, 5);

   /***/
   
   wxFlexGridSizer *gridSizer = new wxFlexGridSizer(2, 0, 0);

   item = new wxStaticText(this, -1, _("Header: "),
                        wxDefaultPosition, wxDefaultSize, 0);
   gridSizer->Add(item, 0,
                  wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 5);

   wxString *headerStrings = new wxString[sf_num_headers()];
   int i;
   for(i=0; i<sf_num_headers(); i++)
      headerStrings[i] = sf_header_name(i);
   mHeaderChoice = 
      new wxChoice(this, ID_HEADER_CHOICE,
                   wxDefaultPosition, wxDefaultSize,
                   sf_num_headers(), headerStrings);
   mHeaderChoice->SetSelection(((mFormat & SF_FORMAT_TYPEMASK) / 0x10000)-1);
   gridSizer->Add(mHeaderChoice, 1, wxEXPAND | wxALL, 5);

   /***/

   item = new wxStaticText(this, -1, _("Data Size: "),
                        wxDefaultPosition, wxDefaultSize, 0);
   gridSizer->Add(item, 0,
                  wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 5);
   wxString bitStrings[4] = {
      "8-bit",
      "16-bit",
      "24-bit",
      "32-bit"};
   mBitsChoice = 
      new wxChoice(this, ID_BITS_CHOICE,
                   wxDefaultPosition, wxDefaultSize,
                   4, bitStrings);
   mBitsChoice->SetSelection(mFormatBits/8 - 1);
   gridSizer->Add(mBitsChoice, 1, wxEXPAND | wxALL, 5);

   /***/

   item = new wxStaticText(this, -1, _("Encoding: "),
                           wxDefaultPosition, wxDefaultSize, 0);
   gridSizer->Add(item, 0,
                  wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 5);
   wxString *encodingStrings = new wxString[sf_num_encodings()];
   for(i=0; i<sf_num_encodings(); i++)
      encodingStrings[i] = sf_encoding_name(i+1);
   mEncodingChoice = 
      new wxChoice(this, ID_ENCODING_CHOICE,
                   wxDefaultPosition, wxDefaultSize,
                   sf_num_encodings(), encodingStrings);
   mEncodingChoice->SetSelection((mFormat & SF_FORMAT_SUBMASK) - 1);
                                        
   gridSizer->Add(mEncodingChoice, 1, wxEXPAND | wxALL, 5);

   /***/

   mainSizer->Add(gridSizer, 0, wxALIGN_CENTRE | wxALL, 5);
   
   /***/
   
   wxBoxSizer *okSizer = new wxBoxSizer(wxHORIZONTAL);

   mOK = 
       new wxButton(this, wxID_OK, _("OK"), wxDefaultPosition,
                    wxDefaultSize, 0);
   mOK->SetDefault();
   mOK->SetFocus();
   okSizer->Add(mOK, 0, wxALIGN_CENTRE | wxALL, 5);

   wxButton *cancel =
       new wxButton(this, wxID_CANCEL, _("Cancel"), wxDefaultPosition,
                    wxDefaultSize, 0);
   okSizer->Add(cancel, 0, wxALIGN_CENTRE | wxALL, 5);

   mainSizer->Add(okSizer, 0, wxALIGN_CENTRE | wxALL, 5);

   SetAutoLayout(TRUE);
   SetSizer(mainSizer);
   mainSizer->Fit(this);
   mainSizer->SetSizeHints(this);
}

void OtherFormatDialog::ValidateChoices()
{
   SF_INFO info = 
   {44100, 1, 2, mFormatBits, mFormat, 1, 1};
   
   if (sf_format_check(&info))
      mOK->Enable(true);
   else
      mOK->Enable(false);
}

void OtherFormatDialog::OnChoice(wxCommandEvent & event)
{
   int h = mHeaderChoice->GetSelection();
   int b = mBitsChoice->GetSelection();
   int e = mEncodingChoice->GetSelection();

   mFormat = (h+1)*0x10000 + (e+1);
   mFormatBits = (b+1)*8;

   ValidateChoices();
}

void OtherFormatDialog::OnOk(wxCommandEvent & event)
{
   EndModal(true);
}

void OtherFormatDialog::OnCancel(wxCommandEvent & event)
{
   EndModal(false);
}

void FileFormatPrefs::Other()
{
   OtherFormatDialog dlog(this, -1,
                          mFormat, mFormatBits);
   dlog.CentreOnParent();
   dlog.ShowModal();

   if (dlog.GetReturnCode()) {
      mFormat = dlog.mFormat;
      mFormatBits = dlog.mFormatBits;
   }
}
