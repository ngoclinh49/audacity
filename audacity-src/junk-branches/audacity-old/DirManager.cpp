/**********************************************************************

  Audacity: A Digital Audio Editor

  DirManager.cpp

  Dominic Mazzoni

**********************************************************************/

#include <wx/defs.h>
#include <wx/app.h>
#include <wx/log.h>
#include <wx/filefn.h>
#include <wx/hash.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/timer.h>
#include <wx/textfile.h>

#include "DirManager.h"
#include "DiskFunctions.h"
#include "Prefs.h"

#include "prefs/PrefsDialog.h"

// Static class variables

int DirManager::numDirManagers = 0;
int DirManager::fileIndex = 0;
bool DirManager::firstCtor = true;
wxString DirManager::tempDirName = ".audacity_temp";

unsigned int DirManager::defaultHashTableSize = 10000;
bool DirManager::hashWarning = false;

// The character which separates directories differs from platform
// to platform.  On Unix, it's a "/", on Windows it's "/" or "\"
// (with "\" preferred), and on the Mac it's a ":".

wxString DirManager::pathChar = wxFILE_SEP_PATH;

#ifdef __WXGTK__
wxString DirManager::home = wxGetHomeDir();
#else
wxString DirManager::home = wxGetCwd();
#endif
wxString DirManager::temp = (DirManager::home +
                             DirManager::pathChar +
                             DirManager::tempDirName);

// Methods

DirManager::DirManager()
{
   if (firstCtor) {
      // The first time a DirManager is created, we need to find
      // a temp directory location.

      firstCtor = false;

      wxString tempFromPrefs = gPrefs->Read("/Directories/TempDir", "");
      wxString tempDefaultLoc = temp;

      temp = "";

      #ifdef __WXGTK__         
      if (tempFromPrefs.GetChar(0) != '/')
         tempFromPrefs = "";
      #endif

      // Stop wxWindows from printing its own error messages

      wxLogNull logNo;

      // Try temp dir that was stored in prefs first

      if (tempFromPrefs != "") {
         if (wxPathExists(tempFromPrefs))
            temp = tempFromPrefs;
         else if (wxMkdir(tempFromPrefs))
            temp = tempFromPrefs;
      }

      // If that didn't work, try the default location

      if (temp=="" && tempDefaultLoc != "") {
         if (wxPathExists(tempDefaultLoc))
            temp = tempDefaultLoc;
         else if (wxMkdir(tempDefaultLoc))
            temp = tempDefaultLoc;
      }

      if (temp != "") {
         // Success
         gPrefs->Write("/Directories/TempDir", temp);
      }
      else {
         wxMessageBox("Audacity could not find a place to store temporary files.\n"
                      "Please enter an appropriate directory in the "
                      "preferences dialog.");

         PrefsDialog dialog(NULL);
         dialog.ShowTempDirPage();
         dialog.ShowModal();

         wxMessageBox("Audacity is now going to exit.  Please launch Audacity "
                      "again to use the new temporary directory.");
         wxExit();
      }

   }
   numDirManagers++;
   if (numDirManagers == 1) {
      CleanTempDir();
   }

   projPath = "";
   projName = "";

   blockFileHash = new wxHashTable(wxKEY_STRING, defaultHashTableSize);

   // Make sure there is plenty of space for temp files

   wxLongLong freeSpace = GetFreeDiskSpace((char *) (const char *) temp);
   if (freeSpace >= 0) {
      if (freeSpace < 1048576) {
         // TODO: allow user to select different temporary volume.
         wxMessageBox
             ("Warning: there is very little free disk space left on this volume. Please select another temporary directory in your preferences.");
      }
   }
}

DirManager::~DirManager()
{
   if (blockFileHash)
      delete blockFileHash;

   numDirManagers--;
   if (numDirManagers == 0) {
      CleanTempDir();
      //::wxRmdir(temp);
   }
}

void DirManager::CleanTempDir()
{
   wxString fname;
   wxStringList fnameList;
   int count = 0;

   fname = wxFindFirstFile((const char *) (temp + pathChar + "*.auf"));
   while (fname != "") {
      if (fname.Length() >= 5 && fname.Right(3) == "auf") {
         count++;
         fnameList.Add(fname);
      }
      fname = wxFindNextFile();
   }

   wxChar **array = fnameList.ListToArray();

   wxProgressDialog *progress = NULL;

   wxYield();
   wxStartTimer();

   for (int i = 0; i < count; i++) {
      wxString fileName = array[i];
      wxRemoveFile(fileName);

      if (!progress && wxGetElapsedTime(false) > 500)
         progress =
             new wxProgressDialog("Progress",
                                  "Cleaning up temporary files...",
                                  1000,
                                  NULL,
                                  wxPD_REMAINING_TIME | wxPD_AUTO_HIDE);

      if (progress)
         progress->Update(int ((i * 1000.0) / count));
   }

   if (progress)
      delete progress;
}

bool DirManager::SetProject(wxString & projPath, wxString & projName,
                            bool create)
{
   if (projPath == "")
      projPath =::wxGetCwd();

   this->projPath = projPath;
   this->projName = projName;
   this->projFull = projPath + pathChar + projName;

   if (create) {
      if (!wxPathExists(projFull))
         return wxMkdir(projFull);
   } else {
#ifndef __WXMAC__
      if (!wxPathExists(projFull))
         return false;
#endif
   }

   return true;
}

wxString DirManager::GetProjectName()
{
   return projName;
}

BlockFile *DirManager::NewTempBlockFile()
{
   wxString theFileName;
   wxString thePathName;
   do {
      theFileName.Printf("b%05d.auf", fileIndex++);
      thePathName = temp + pathChar + theFileName;
   } while (wxFileExists(thePathName));

   BlockFile *newBlockFile = new BlockFile(theFileName, thePathName);

   blockFileHash->Put(theFileName, (wxObject *) newBlockFile);

   CheckHashTableSize();

   return newBlockFile;
}

BlockFile *DirManager::NewBlockFile()
{
   if (projFull == "")
      return NewTempBlockFile();

   wxString theFileName;
   wxString thePathName;
   do {
      theFileName.Printf("b%05d.auf", fileIndex++);
      thePathName = projFull + pathChar + theFileName;
   } while (wxFileExists(thePathName));

   BlockFile *newBlockFile = new BlockFile(theFileName, thePathName);

   blockFileHash->Put(theFileName, (wxObject *) newBlockFile);

   CheckHashTableSize();

   return newBlockFile;
}

BlockFile *DirManager::NewTempAliasBlockFile(wxString fullPath,
                                             sampleCount start,
                                             sampleCount len, int channel)
{
   wxString theFileName;
   wxString thePathName;
   do {
      theFileName.Printf("b%05d.auf", fileIndex++);
      thePathName = temp + pathChar + theFileName;
   } while (wxFileExists(thePathName));

   BlockFile *newBlockFile = new BlockFile(theFileName, thePathName,
                                           fullPath,
                                           start, len, channel);

   blockFileHash->Put(theFileName, (wxObject *) newBlockFile);

   CheckHashTableSize();

   return newBlockFile;
}

BlockFile *DirManager::NewAliasBlockFile(wxString fullPath,
                                         sampleCount start,
                                         sampleCount len, int channel)
{
   if (projFull == "")
      return NewTempAliasBlockFile(fullPath, start, len,
                                   channel);

   wxString theFileName;
   wxString thePathName;
   do {
      theFileName.Printf("b%05d.auf", fileIndex++);
      thePathName = projFull + pathChar + theFileName;
   } while (wxFileExists(thePathName));

   BlockFile *newBlockFile = new BlockFile(theFileName, thePathName,
                                           fullPath,
                                           start, len, channel);

   blockFileHash->Put(theFileName, (wxObject *) newBlockFile);

   CheckHashTableSize();

   return newBlockFile;
}

void DirManager::SaveBlockFile(BlockFile * f, wxTextFile * out)
{
   if (f->IsAlias()) {
      out->AddLine("Alias");
      out->AddLine(f->mAliasFullPath);
      out->AddLine(wxString::Format("%d", f->mStart));
      out->AddLine(wxString::Format("%d", f->mLen));
      out->AddLine(wxString::Format("%d", f->mChannel));
   }
   out->AddLine(f->mName);
}

BlockFile *DirManager::LoadBlockFile(wxTextFile * in)
{
   wxASSERT(projFull != "");

   wxString blockName = in->GetNextLine();

   bool alias = false;
   wxString aliasFullPath;
   long start, len, channel;

   if (blockName == "Alias") {
      alias = true;
      aliasFullPath = in->GetNextLine();
      if (!(in->GetNextLine().ToLong(&start)))
         return NULL;
      if (!(in->GetNextLine().ToLong(&len)))
         return NULL;
      if (!(in->GetNextLine().ToLong(&channel)))
         return NULL;

      blockName = in->GetNextLine();
   }

   wxString pathName = projFull + pathChar + blockName;
   BlockFile *retrieved = (BlockFile *) blockFileHash->Get(blockName);
   if (retrieved) {
      wxASSERT(retrieved->IsAlias() == alias);
      retrieved->Ref();
      return retrieved;
   } else {
      BlockFile *newBlockFile;

      if (alias)
         newBlockFile = new BlockFile(blockName, pathName,
                                      aliasFullPath, start, len, channel);
      else
         newBlockFile = new BlockFile(blockName, pathName);

      blockFileHash->Put(blockName, (wxObject *) newBlockFile);

      CheckHashTableSize();

      if (!wxFileExists(pathName))
         return 0;
      return newBlockFile;
   }
}

void DirManager::MakePartOfProject(BlockFile * f)
{
   wxString newFullPath = projFull + pathChar + f->mName;
   if (newFullPath != f->mFullPath) {
      bool ok = wxRenameFile(f->mFullPath, newFullPath);
      if (ok)
         f->mFullPath = newFullPath;
      else {
         ok = wxCopyFile(f->mFullPath, newFullPath);
         if (ok) {
            wxRemoveFile(f->mFullPath);
            f->mFullPath = newFullPath;
         } else {
            wxString msg;
            msg.Printf("Could not rename %s to %s",
                       (const char *) f->mFullPath,
                       (const char *) newFullPath);
            wxMessageBox(msg);
         }
      }
   }
}

void DirManager::Ref(BlockFile * f)
{
   f->Ref();
   //  printf("Ref(%d): %s\n",f->refCount, (const char *)(f->fullPath));
}

void DirManager::Deref(BlockFile * f)
{
   wxString theFileName = f->mName;

   //  printf("Deref(%d): %s\n",f->mRefCount-1, (const char *)f->mRullPath);

   if (f->Deref()) {
      // If Deref() returned true, the reference count reached zero
      // and this block is no longer needed.  Remove it from the hash
      // table.

      blockFileHash->Delete(theFileName);
   }
}

void DirManager::CheckHashTableSize()
{
#ifdef __WXDEBUG__
   if (!hashWarning && blockFileHash->GetCount() >= defaultHashTableSize) {
      hashWarning = true;
      wxString msg;
      msg.Printf("Warning: DirManager hash table full (%d entries).",
                 blockFileHash->GetCount());
      wxMessageBox(msg);
   }
#endif
}
