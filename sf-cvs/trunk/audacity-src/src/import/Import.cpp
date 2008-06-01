/**********************************************************************

  Audacity: A Digital Audio Editor

  Import.cpp

  Dominic Mazzoni

*******************************************************************//**

\file Import.cpp

  This file contains a general function which will import almost
  any type of sampled audio file (i.e. anything except MIDI)
  and return the tracks that were imported.  This function just
  figures out which one to call; the actual importers are in
  ImportPCM, ImportMP3, ImportOGG, ImportRawData, ImportLOF, 
  ImportQT and ImportFLAC.

*//***************************************************************//**

\class Format
\brief Abstract base class used in importing a file.

It's defined in Import.h

*//***************************************************************//**

\class Importer
\brief Class which actulaly imports the auido, using functions defined 
in ImportPCM.cpp, ImportMP3.cpp, ImportOGG.cpp, ImportRawData.cpp, 
and ImportLOF.cpp.
 
*//******************************************************************/



#include <wx/textctrl.h>
#include <wx/msgdlg.h>
#include <wx/string.h>
#include <wx/intl.h>
#include <wx/listimpl.cpp>
#include <wx/log.h>
#include "../Audacity.h"

#include "Import.h"
#include "ImportPlugin.h"
#include "ImportPCM.h"
#include "ImportMP3.h"
#include "ImportOGG.h"
#include "ImportQT.h"
#include "ImportRaw.h"
#include "ImportLOF.h"
#include "ImportFLAC.h"
#include "../Track.h"

WX_DEFINE_LIST(ImportPluginList);
WX_DEFINE_LIST(UnusableImportPluginList);
WX_DEFINE_LIST(FormatList);

Importer::Importer()
{
   mImportPluginList = new ImportPluginList;
   mUnusableImportPluginList = new UnusableImportPluginList;

   // build the list of import plugin and/or unusableImporters.
   // order is significant.  If none match, they will all be tried
   // in the order defined here.
   GetPCMImportPlugin(mImportPluginList, mUnusableImportPluginList);
   GetOGGImportPlugin(mImportPluginList, mUnusableImportPluginList);
   GetFLACImportPlugin(mImportPluginList, mUnusableImportPluginList);
   GetMP3ImportPlugin(mImportPluginList, mUnusableImportPluginList);
   GetLOFImportPlugin(mImportPluginList, mUnusableImportPluginList);

   #ifdef USE_QUICKTIME
   GetQTImportPlugin(mImportPluginList, mUnusableImportPluginList);
   #endif
}

Importer::~Importer()
{
   mImportPluginList->DeleteContents(true);
   delete mImportPluginList;
   mUnusableImportPluginList->DeleteContents(true);//JKC
   delete mUnusableImportPluginList;
}

void Importer::GetSupportedImportFormats(FormatList *formatList)
{
   ImportPluginList::Node *importPluginNode = mImportPluginList->GetFirst();
   while(importPluginNode)
   {
      ImportPlugin *importPlugin = importPluginNode->GetData();
      formatList->Append(new Format(importPlugin->GetPluginFormatDescription(),
                                    importPlugin->GetSupportedExtensions()));
      importPluginNode = importPluginNode->GetNext();
   }
}

// returns number of tracks imported
int Importer::Import(wxString fName,
                     TrackFactory *trackFactory,
                     Track *** tracks,
                     Tags *tags,
                     wxString &errorMessage)
{
   int numTracks = 0;

   wxString extension = fName.AfterLast(wxT('.'));
   
   // see if any of the plugins expect this extension and if so give
   // that plugin first dibs
   ImportPluginList::Node *importPluginNode = mImportPluginList->GetFirst();
   while(importPluginNode)
   {
      ImportPlugin *plugin = importPluginNode->GetData();
      if (plugin->SupportsExtension(extension))
      {
         mInFile = plugin->Open(fName);
         if (mInFile != NULL)
         {
            int res;
            
            res = mInFile->Import(trackFactory, tracks, &numTracks, tags);

            delete mInFile;

            if (res == eImportSuccess)
            {
               // LOF ("list-of-files") has different semantics
               if (extension.IsSameAs(wxT("lof"), false))
                  return 1;

               if (numTracks > 0) {
                  // success!
                  return numTracks;
               }
            }

            if (res == eImportCancelled)
            {
               return 0;
            }

            // We could exit here since we had a match on the file extension,
            // but there may be another plug-in that can import the file and
            // that may recognize the extension, so we allow the loop to
            // continue.
         }
      }
      importPluginNode = importPluginNode->GetNext();
   }

   // None of our plugins can handle this file.  It might be that
   // Audacity supports this format, but support was not compiled in.
   // If so, notify the user of this fact
   UnusableImportPluginList::Node *unusableImporterNode
      = mUnusableImportPluginList->GetFirst();
   while(unusableImporterNode)
   {
      UnusableImportPlugin *unusableImportPlugin = unusableImporterNode->GetData();
      if( unusableImportPlugin->SupportsExtension(extension) )
      {
         errorMessage.Printf(_("This version of Audacity was not compiled with %s support."),
                             unusableImportPlugin->
                             GetPluginFormatDescription().c_str());
         return 0;
      }
      unusableImporterNode = unusableImporterNode->GetNext();
   }

   /* warnings for unsupported data types */

   // if someone has sent us a .cda file, send them away
   if (extension.IsSameAs(wxT("cda"), false)) {
      /* i18n-hint: %s will be the filename */
      errorMessage.Printf(_("\"%s\" is an audio CD track. \nAudacity cannot open audio CDs directly. \nExtract (rip) the CD tracks to an audio format that \nAudacity can import, such as WAV or AIFF."), fName.c_str());
      return 0;
   }

   // playlist type files
   if ((extension.IsSameAs(wxT("m3u"), false))||(extension.IsSameAs(wxT("ram"), false))||(extension.IsSameAs(wxT("pls"), false))) {
      errorMessage.Printf(_("\"%s\" is a playlist file. \nAudacity cannot open this file because it only contains links to other files. \nYou may be able to open it in a text editor and download the actual audio files."), fName.c_str());
      return 0;
   }
   //WMA files of various forms
   if ((extension.IsSameAs(wxT("wmv"), false))||(extension.IsSameAs(wxT("wma"), false))||(extension.IsSameAs(wxT("asf"), false))) {
      errorMessage.Printf(_("\"%s\" is a Windows Media Audio file. \nAudacity cannot open this type of file due to patent restrictions. \nYou need to convert it to a supported audio format, such as WAV or AIFF."), fName.c_str());
      return 0;
   }
   //AAC files of various forms (probably not encrypted)
   if ((extension.IsSameAs(wxT("aac"), false))||(extension.IsSameAs(wxT("m4a"), false))||(extension.IsSameAs(wxT("mp4"), false))) {
      errorMessage.Printf(_("\"%s\" is an Advanced Audio Coding file. \nAudacity cannot open this type of file. \nYou need to convert it to a supported audio format, such as WAV or AIFF."), fName.c_str());
      return 0;
   }
   // encrypted itunes files
   if ((extension.IsSameAs(wxT("m4p"), false))) {
      errorMessage.Printf(_("\"%s\" is an encrypted audio file. These typically are from an online music store. \nAudacity cannot open this type of file due to the encryption. \nTry recording the file into Audacity, or burn it to audio CD then \nextract the CD track to a supported audio format such as WAV or AIFF."), fName.c_str());
      return 0;
   }
   // Real Inc. files of various sorts
   if ((extension.IsSameAs(wxT("ra"), false))||(extension.IsSameAs(wxT("rm"), false))||(extension.IsSameAs(wxT("rpm"), false))||(extension.IsSameAs(wxT("rv"), false))) {
      errorMessage.Printf(_("\"%s\" is a RealPlayer media file. \nAudacity cannot open this proprietary format. \nYou need to convert it to a supported audio format, such as WAV or AIFF."), fName.c_str());
      return 0;
   }

   // MIDI files must be imported, not opened
   if ((extension.IsSameAs(wxT("midi"), false))||(extension.IsSameAs(wxT("mid"), false))) {
      errorMessage.Printf(_("\"%s\" is a MIDI file, not an audio file. \nAudacity cannot open this type of file for playing or editing, but you can \ndisplay it by clicking File > Import > MIDI."), fName.c_str());
      return 0;
   }

   // Other notes-based formats
   if ((extension.IsSameAs(wxT("kar"), false))||(extension.IsSameAs(wxT("mod"), false))||(extension.IsSameAs(wxT("rmi"), false))) {
      errorMessage.Printf(_("\"%s\" is a notes-based file, not an audio file. \nAudacity cannot open this type of file. \nTry converting it to an audio file such as WAV or AIFF and \nthen import it, or record it into Audacity."), fName.c_str());
      return 0;
   }

   // MusePack files
   if ((extension.IsSameAs(wxT("mp+"), false))||(extension.IsSameAs(wxT("mpc"), false))||(extension.IsSameAs(wxT("mpp"), false))) {
      errorMessage.Printf(_("\"%s\" is a Musepack audio file. \nAudacity cannot open this type of file. \nIf you think it might be an mp3 file, rename it to end with \".mp3\" \nand try importing it again. Otherwise you need to convert it to a supported audio \nformat, such as WAV or AIFF."), fName.c_str());
      return 0;
   }

   // WavPack files
   if ((extension.IsSameAs(wxT("wv"), false))||(extension.IsSameAs(wxT("wvc"), false))) {
      errorMessage.Printf(_("\"%s\" is a Wavpack audio file. \nAudacity cannot open this type of file. \nYou need to convert it to a supported audio format, such as WAV or AIFF."), fName.c_str());
      return 0;
   }

   // AC3 files
   if ((extension.IsSameAs(wxT("ac3"), false))) {
      errorMessage.Printf(_("\"%s\" is a Dolby Digital audio file. \nAudacity cannot currently open this type of file. \nYou need to convert it to a supported audio format, such as WAV or AIFF."), fName.c_str());
      return 0;
   }

   // Speex files
   if ((extension.IsSameAs(wxT("spx"), false))) {
      errorMessage.Printf(_("\"%s\" is an Ogg Speex audio file. \nAudacity cannot currently open this type of file. \nYou need to convert it to a supported audio format, such as WAV or AIFF."), fName.c_str());
      return 0;
   }

   // no importPlugin that recognized the extension succeeded.  However, the
   // file might be misnamed.  So this time we try all the importPlugins
   //in order and see if any of them can handle the file
   importPluginNode = mImportPluginList->GetFirst();
   while(importPluginNode)
   {
      ImportPlugin *plugin = importPluginNode->GetData();

      // Skip MP3 import plugin. Opens some non-mp3 audio files (ac3 for example) as garbage.
      while ( importPluginNode && ( plugin->GetPluginFormatDescription().CompareTo( _("MP3 files") ) == 0 ) )
      {
         importPluginNode = importPluginNode->GetNext();
         plugin = importPluginNode->GetData();
      }

      mInFile = plugin->Open(fName);
      if (mInFile != NULL)
      {
         int res;

         numTracks = 0;
         res = mInFile->Import(trackFactory, tracks, &numTracks, tags);
         delete mInFile;

         if (res == eImportSuccess)
         {
            if (numTracks > 0)
            {
               // success!
               errorMessage.Printf(_( "Audacity had to guess at the type of file.\n\nIt was identified as: \"%s\".\n\nIf the audio does not sound right, try renaming the file\nso that it has the correct extension before opening it."),
                                   plugin->GetPluginFormatDescription());
               return numTracks;
            }
         }

         if (res == eImportCancelled)
         {
            return 0;
         }

         // The import failed so we allow the loop to continue since
         // there may be another plugin that can successfully load
         // the file.
      }
      importPluginNode = importPluginNode->GetNext();
   }

   // we were not able to recognize the file type
   errorMessage = _("Audacity did not recognize the type of this file.\nIf it is uncompressed, try importing it using \"Import Raw\"" );
   return 0;
}

wxString Importer::GetFileDescription()
{
   return mInFile->GetFileDescription();
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
// arch-tag: 702e6bd3-b26c-424f-9d6a-c88b565ea143

