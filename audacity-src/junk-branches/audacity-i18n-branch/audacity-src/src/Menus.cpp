/**********************************************************************

  Audacity: A Digital Audio Editor

  Menus.cpp

  Dominic Mazzoni

  This file implements all of the methods that get called when you
  select an item from a menu, along with routines to create the
  menu bar and enable/disable items.  All of the methods here are
  methods of AudacityProject, so for the header file, see Project.h.

**********************************************************************/

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
   #include <wx/app.h>
   #include <wx/menu.h>
   #include <wx/string.h>
#endif

#include "sndfile.h"

#include <wx/textfile.h>
#include <wx/intl.h>

#include "Audacity.h"
#include "AboutDialog.h"
#include "APalette.h"
#include "Benchmark.h"
#include "export/Export.h"
#include "FileFormats.h"
#include "FreqWindow.h"
#include "Help.h"
#include "import/Import.h"
#include "import/ImportRaw.h"
#include "import/ImportMIDI.h"
#include "LabelTrack.h"
#include "Mix.h"
#include "NoteTrack.h"
#include "Prefs.h"
#include "Project.h"
#include "Tags.h"
#include "Track.h"
#include "TrackPanel.h"
#include "WaveTrack.h"
#include "HistoryWindow.h"
#include "effects/Effect.h"
#include "prefs/PrefsDialog.h"

#define AUDACITY_MENUS_ENUM
#include "Menus.h"
#undef AUDACITY_MENUS_ENUM

void AudacityProject::CreateMenuBar()
{
   mMenusDirtyCheck = gMenusDirty;
   mFirstTimeUpdateMenus = true;

   mMenuBar = new wxMenuBar();

   // Note: if you change the titles of any of the menus here,
   // you must also change the title in OnUpdateMenus, below.
   
   int format = ReadExportFormatPref();                         
   wxString pcmFormat = sf_header_name(format & SF_FORMAT_TYPEMASK);

   mExportString.Printf(_("&Export as %s..."), pcmFormat.c_str());
   mExportSelectionString.Printf(_("Export &Selection as %s..."),
                                 pcmFormat.c_str());
   wxString lossyFormat = "MP3";
   mExportLossyString.Printf(_("Export as %s..."), lossyFormat.c_str());
   mExportSelectionLossyString.Printf(_("Export Selection as %s..."),
                                      lossyFormat.c_str());

   mFileMenu = new wxMenu();
   mFileMenu->Append(NewID, _("&New...\tCtrl+N"));
   mFileMenu->Append(OpenID, _("&Open...\tCtrl+O"));
   mFileMenu->Append(CloseID, _("&Close\tCtrl+W"));
   mFileMenu->Append(SaveID, _("&Save Project\tCtrl+S"));
   mFileMenu->Append(SaveAsID, _("Save Project &As..."));
   mFileMenu->AppendSeparator();
   mFileMenu->Append(ExportMixID, mExportString);
   mFileMenu->Append(ExportSelectionID, mExportSelectionString);
   mFileMenu->AppendSeparator();
   mFileMenu->Append(ExportLossyMixID, mExportLossyString);
   mFileMenu->Append(ExportLossySelectionID, mExportSelectionLossyString);
   mFileMenu->AppendSeparator();
   mFileMenu->Append(ExportLabelsID, _("Export &Labels..."));
   mFileMenu->AppendSeparator();
   mFileMenu->Append(PreferencesID, _("&Preferences...\tCtrl+P"));
   mFileMenu->AppendSeparator();
   mFileMenu->Append(ExitID, _("E&xit"));

   mEditMenu = new wxMenu();
   mEditMenu->Append(UndoID, _("&Undo\tCtrl+Z"));
   mEditMenu->Append(RedoID, _("&Redo\tCtrl+R"));
   mEditMenu->AppendSeparator();
   mEditMenu->Append(CutID, _("Cut\tCtrl+X"));
   mEditMenu->Append(CopyID, _("Copy\tCtrl+C"));
   mEditMenu->Append(PasteID, _("Paste\tCtrl+V"));
   mEditMenu->AppendSeparator();
   mEditMenu->Append(DeleteID, _("&Delete\tCtrl+K"));
   mEditMenu->Append(SilenceID, _("Silence\tCtrl+L"));
   mEditMenu->AppendSeparator();
   mEditMenu->Append(InsertSilenceID, _("Insert Silence..."));
   mEditMenu->Append(SplitID, _("Split\tCtrl+Y"));
   mEditMenu->Append(DuplicateID, _("Duplicate\tCtrl+D"));
   mEditMenu->AppendSeparator();
   mEditMenu->Append(SelectAllID, _("&Select All\tCtrl+A"));

   mViewMenu = new wxMenu();
   mViewMenu->Append(ZoomInID, _("Zoom &In\tCtrl+1"));
   mViewMenu->Append(ZoomNormalID, _("Zoom &Normal\tCtrl+2"));
   mViewMenu->Append(ZoomOutID, _("Zoom &Out\tCtrl+3"));
   mViewMenu->Append(ZoomFitID, _("Fit in &Window\tCtrl+F"));
   mViewMenu->AppendSeparator();
   mViewMenu->Append(UndoHistoryID, _("History"));
   mViewMenu->Append(PlotSpectrumID, _("&Plot Spectrum\tCtrl+U"));

#ifndef __WXMAC__
   mViewMenu->AppendSeparator();
   mViewMenu->Append(FloatPaletteID, _("Float or Unfloat Palette"));
#endif

   mProjectMenu = new wxMenu();
   mProjectMenu->Append(ImportID, _("&Import Audio...\tCtrl+I"));
   mProjectMenu->Append(ImportLabelsID, _("Import Labels..."));
   mProjectMenu->Append(ImportMIDIID, _("Import &MIDI..."));
   mProjectMenu->Append(ImportRawID, _("Import Raw Data..."));
#ifdef USE_ID3LIB
   mProjectMenu->AppendSeparator();
   mProjectMenu->Append(EditID3ID, _("Edit ID3 Tags..."));   
#endif
   mProjectMenu->AppendSeparator();
   mProjectMenu->Append(QuickMixID, _("&Quick Mix"));
   mProjectMenu->AppendSeparator();
   mProjectMenu->Append(AlignID, _("Align Tracks Together"));
   mProjectMenu->Append(AlignZeroID, _("Align with Zero"));
   mProjectMenu->AppendSeparator();
   mProjectMenu->Append(NewWaveTrackID, _("New &Audio Track"));
   mProjectMenu->Append(NewLabelTrackID, _("New &Label Track"));
   mProjectMenu->AppendSeparator();
   mProjectMenu->Append(RemoveTracksID, _("&Remove Track(s)"));

   /*
      mTrackMenu = new wxMenu();
      mTrackMenu->Append(QuickMixID, _("Quick Mix"));
      mTrackMenu->AppendSeparator();
      mTrackMenu->Append(WaveDisplayID, _("Waveform Display"));
      mTrackMenu->Append(SpectrumDisplayID, _("Spectrum Display"));
      mTrackMenu->AppendSeparator();
      //  mTrackMenu->Append(AutoCorrelateID, _("AutoCorrelate"));
      mTrackMenu->Append(PitchID, _("Pitch Extract"));
    */

   mEffectMenu = new wxMenu();

   int numEffects = Effect::GetNumEffects(false);
   int fi;
   for (fi = 0; fi < numEffects; fi++)
      mEffectMenu->Append(FirstEffectID + fi,
                          (Effect::GetEffect(fi, false))->GetEffectName());

   mPluginMenu = new wxMenu();

   int numPlugins = Effect::GetNumEffects(true);
   for (fi = 0; fi < numPlugins; fi++)
      mPluginMenu->Append(FirstPluginID + fi,
                          (Effect::GetEffect(fi, true))->GetEffectName());

#ifdef __WXMAC__
   wxApp::s_macAboutMenuItemId = AboutID;
#endif

   mHelpMenu = new wxMenu();
   mHelpMenu->Append(AboutID, _("About Audacity..."));

#ifndef __WXMAC__
   mHelpMenu->AppendSeparator();
#endif

   mHelpMenu->Append(HelpID, _("Online Help..."));
   mHelpMenu->Append(HelpIndexID, _("Online Help Index..."));
   mHelpMenu->Append(HelpSearchID, _("Search Online Help..."));

   mHelpMenu->AppendSeparator();

   mHelpMenu->Append(BenchmarkID, _("Run Benchmark..."));

   mMenuBar->Append(mFileMenu, _("&File"));
   mMenuBar->Append(mEditMenu, _("&Edit"));
   mMenuBar->Append(mViewMenu, _("&View"));
   mMenuBar->Append(mProjectMenu, _("&Project"));
   //  mMenuBar->Append(mTrackMenu, _("&Track"));
   mMenuBar->Append(mEffectMenu, _("E&ffect"));
   mMenuBar->Append(mPluginMenu, _("Plugin&s"));
   mMenuBar->Append(mHelpMenu, _("&Help"));

   SetMenuBar(mMenuBar);

   mInsertSilenceAmount = 1.0;
}

void AudacityProject::OnUpdateMenus(wxUpdateUIEvent & event)
{
   if (mMenusDirtyCheck != gMenusDirty) {
      /*
      TODO!
      */
   }

   // Note that the titles of the menus here are dependent on the
   // titles above.

   mEditMenu->Enable(mEditMenu->FindItem(_("Undo")),
                     mUndoManager.UndoAvailable());
   mEditMenu->Enable(mEditMenu->FindItem(_("Redo")),
                     mUndoManager.RedoAvailable());

   bool nonZeroRegionSelected = (mViewInfo.sel1 > mViewInfo.sel0);

   int numTracks = 0;
   int numTracksSelected = 0;
   int numWaveTracks = 0;
   int numWaveTracksSelected = 0;
   int numLabelTracks = 0;

   TrackListIterator iter(mTracks);
   VTrack *t = iter.First();
   while (t) {
      numTracks++;
      if (t->GetKind() == VTrack::Wave)
         numWaveTracks++;
      if (t->GetKind() == VTrack::Label)
         numLabelTracks++;
      if (t->GetSelected()) {
         numTracksSelected++;
         if (t->GetKind() == VTrack::Wave)
            numWaveTracksSelected++;
      }
      t = iter.Next();
   }

   mEditMenu->Enable(mEditMenu->FindItem("Paste"),
                     numTracksSelected > 0 && msClipLen > 0.0);

   // Return from this function if nothing's changed since
   // the last time we were here.

   if (!mFirstTimeUpdateMenus &&
       mLastNonZeroRegionSelected==nonZeroRegionSelected &&
       mLastNumTracks==numTracks &&
       mLastNumTracksSelected==numTracksSelected &&
       mLastNumWaveTracks==numWaveTracks &&
       mLastNumWaveTracksSelected==numWaveTracksSelected &&
       mLastNumLabelTracks==numLabelTracks)
      return;

   // Otherwise, save state and then update all of the menus

   mFirstTimeUpdateMenus = false;
   mLastNonZeroRegionSelected = nonZeroRegionSelected;
   mLastNumTracks = numTracks;
   mLastNumTracksSelected = numTracksSelected;
   mLastNumWaveTracks = numWaveTracks;
   mLastNumWaveTracksSelected = numWaveTracksSelected;
   mLastNumLabelTracks = numLabelTracks;

   mFileMenu->Enable(mFileMenu->FindItem(mExportString), numTracks > 0);
   mFileMenu->Enable(mFileMenu->FindItem(mExportSelectionString),
                     numTracksSelected > 0 && nonZeroRegionSelected);
   mFileMenu->Enable(mFileMenu->FindItem(mExportLossyString), numTracks > 0);
   mFileMenu->Enable(mFileMenu->FindItem(mExportSelectionLossyString),
                     numTracksSelected > 0 && nonZeroRegionSelected);
   mFileMenu->Enable(mFileMenu->FindItem(_("Export Labels...")),
                     numLabelTracks > 0);

   mEditMenu->Enable(mEditMenu->FindItem(_("Cut")),
                     numTracksSelected > 0 && nonZeroRegionSelected);
   mEditMenu->Enable(mEditMenu->FindItem(_("Copy")),
                     numTracksSelected > 0 && nonZeroRegionSelected);
   mEditMenu->Enable(mEditMenu->FindItem(_("Delete")),
                     numTracksSelected > 0 && nonZeroRegionSelected);
   mEditMenu->Enable(mEditMenu->FindItem(_("Silence")),
                     numTracksSelected > 0 && nonZeroRegionSelected);
   mEditMenu->Enable(mEditMenu->FindItem(_("Insert Silence...")),
                     numTracksSelected > 0);
   mEditMenu->Enable(mEditMenu->FindItem(_("Split")),
                     numTracksSelected > 0 && nonZeroRegionSelected);
   mEditMenu->Enable(mEditMenu->FindItem(_("Duplicate")),
                     numTracksSelected > 0 && nonZeroRegionSelected);
   mEditMenu->Enable(mEditMenu->FindItem(_("Select All")), numTracks > 0);

   mViewMenu->Enable(mViewMenu->FindItem(_("Plot Spectrum")),
                     numWaveTracksSelected > 0 && nonZeroRegionSelected);

   mProjectMenu->Enable(mProjectMenu->FindItem(_("Quick Mix")),
                        numWaveTracksSelected > 1);

   mProjectMenu->Enable(mProjectMenu->FindItem(_("Align Tracks Together")),
                        numTracksSelected > 1);
   mProjectMenu->Enable(mProjectMenu->FindItem(_("Align with Zero")),
                        numTracksSelected > 0);
   mProjectMenu->Enable(mProjectMenu->FindItem(_("Remove Track(s)")),
                        numTracksSelected > 0);

   int e;
   for (e = 0; e < Effect::GetNumEffects(false); e++) {
      Effect *f = Effect::GetEffect(e, false);
      wxString effectName = f->GetEffectName();
      int itemNo = mEffectMenu->FindItem(effectName);
      mEffectMenu->Enable(itemNo,
                          numWaveTracksSelected > 0
                          && nonZeroRegionSelected);
   }

   for (e = 0; e < Effect::GetNumEffects(true); e++) {
      Effect *f = Effect::GetEffect(e, true);
      wxString effectName = f->GetEffectName();
      int itemNo = mPluginMenu->FindItem(effectName);
      mPluginMenu->Enable(itemNo,
                          numWaveTracksSelected > 0
                          && nonZeroRegionSelected);
   }
}

//
// File Menu
//

void AudacityProject::OnNew(wxCommandEvent & event)
{
   CreateNewAudacityProject(gParentWindow);
}

void AudacityProject::OnOpen(wxCommandEvent & event)
{
   wxString path = gPrefs->Read("/DefaultOpenPath",::wxGetCwd());

   wxString fileName = wxFileSelector(_("Select an audio file..."),
                                      path,     // Path
                                      "",       // Name
                                      "",       // Extension
                                      _("All files (*.*)|*.*|"
                                      "Audacity projects (*.aup)|*.aup|"
                                      "WAV files (*.wav)|*.wav|"
                                      "AIFF files (*.aif)|*.aif|"
                                      "AU files (*.au)|*.au|"
                                      "IRCAM files (*.snd)|*.snd|"
                                      "MP3 files (*.mp3)|*.mp3"),
                                      0,        // Flags
                                      this);    // Parent

   if (fileName != "") {

      path =::wxPathOnly(fileName);
      gPrefs->Write("/DefaultOpenPath", path);

      if (mDirty) {
         AudacityProject *project =
             CreateNewAudacityProject(gParentWindow);
         project->OpenFile(fileName);
      } else
         OpenFile(fileName);
   }
}

void AudacityProject::OnSave(wxCommandEvent & event)
{
   Save();
}

void AudacityProject::OnSaveAs(wxCommandEvent & event)
{
   SaveAs();
}

void AudacityProject::OnExit(wxCommandEvent & event)
{
   QuitAudacity();
}

void AudacityProject::OnExportLabels(wxCommandEvent & event)
{
   VTrack *t;
   int numLabelTracks = 0;

   TrackListIterator iter(mTracks);

   t = iter.First();
   while (t) {
      if (t->GetKind() == VTrack::Label)
         numLabelTracks++;
      t = iter.Next();
   }

   if (numLabelTracks == 0) {
      wxMessageBox(_("There are no label tracks to export."));
      return;
   }

   wxString fName = _("labels.txt");

   fName = wxFileSelector(_("Export Labels As:"),
                          NULL,
                          fName,
                          "txt",
                          "*.txt", wxSAVE | wxOVERWRITE_PROMPT, this);

   if (fName == "")
      return;

   wxTextFile f(fName);
#ifdef __WXMAC__
   wxFile *temp = new wxFile();
   temp->Create(fName);
   delete temp;
#else
   f.Create();
#endif
   f.Open();
   if (!f.IsOpened()) {
      wxMessageBox(_("Couldn't write to file: ") + fName);
      return;
   }

   t = iter.First();
   while (t) {
      if (t->GetKind() == VTrack::Label)
         ((LabelTrack *) t)->Export(f);

      t = iter.Next();
   }

#ifdef __WXMAC__
   f.Write(wxTextFileType_Mac);
#else
   f.Write();
#endif
   f.Close();
}

void AudacityProject::OnExportMix(wxCommandEvent & event)
{
   ::Export(this, false, 0.0, mTracks->GetMaxLen());
}

void AudacityProject::OnExportSelection(wxCommandEvent & event)
{
   ::Export(this, true, mViewInfo.sel0, mViewInfo.sel1);
}

void AudacityProject::OnExportLossyMix(wxCommandEvent & event)
{
   ::ExportLossy(this, false, 0.0, mTracks->GetMaxLen());
}

void AudacityProject::OnExportLossySelection(wxCommandEvent & event)
{
   ::ExportLossy(this, true, mViewInfo.sel0, mViewInfo.sel1);
}

void AudacityProject::OnPreferences(wxCommandEvent & event)
{
   PrefsDialog dialog(this /* parent */ );
   dialog.ShowModal();
}

//
// Edit Menu
//

void AudacityProject::Undo(wxCommandEvent & event)
{
   if (!mUndoManager.UndoAvailable()) {
      wxMessageBox(_("Nothing to undo"));
      return;
   }

   TrackList *l = mUndoManager.Undo(&mViewInfo.sel0, &mViewInfo.sel1);
   PopState(l);

   FixScrollbars();
   //UpdateMenus();
   mTrackPanel->Refresh(false);

   if(mHistoryWindow)
      mHistoryWindow->UpdateDisplay();
}

void AudacityProject::Redo(wxCommandEvent & event)
{
   if (!mUndoManager.RedoAvailable()) {
      wxMessageBox(_("Nothing to redo"));
      return;
   }

   TrackList *l = mUndoManager.Redo(&mViewInfo.sel0, &mViewInfo.sel1);
   PopState(l);

   FixScrollbars();
   //UpdateMenus();
   mTrackPanel->Refresh(false);

   if(mHistoryWindow)
      mHistoryWindow->UpdateDisplay();
}

void AudacityProject::UndoHistory(wxCommandEvent & event)
{
   if(mHistoryWindow)
      mHistoryWindow->Show(true);
   else {
      mHistoryWindow = new HistoryWindow(this, &mUndoManager);
      mHistoryWindow->Show(true);
   }
}


void AudacityProject::Cut(wxCommandEvent & event)
{
   ClearClipboard();

   TrackListIterator iter(mTracks);

   VTrack *n = iter.First();
   VTrack *dest = 0;

   while (n) {
      if (n->GetSelected()) {
         n->Cut(mViewInfo.sel0, mViewInfo.sel1, &dest);
         if (dest)
            msClipboard->Add(dest);
      }
      n = iter.Next();
   }

   msClipLen = (mViewInfo.sel1 - mViewInfo.sel0);
   msClipProject = this;

   mViewInfo.sel1 = mViewInfo.sel0;

   PushState(_("Cut to the clipboard"));

   FixScrollbars();
   mTrackPanel->Refresh(false);
   //UpdateMenus();
}

void AudacityProject::Copy(wxCommandEvent & event)
{
   ClearClipboard();

   TrackListIterator iter(mTracks);

   VTrack *n = iter.First();
   VTrack *dest = 0;

   while (n) {
      if (n->GetSelected()) {
         n->Copy(mViewInfo.sel0, mViewInfo.sel1, &dest);
         if (dest)
            msClipboard->Add(dest);
      }
      n = iter.Next();
   }

   msClipLen = (mViewInfo.sel1 - mViewInfo.sel0);
   msClipProject = this;

   //mViewInfo.sel1 = mViewInfo.sel0;
   //UpdateMenus();

   //  PushState();
   //  Not an undoable operation
}

void AudacityProject::Paste(wxCommandEvent & event)
{
   if (mViewInfo.sel0 != mViewInfo.sel1)
      Clear();

   wxASSERT(mViewInfo.sel0 == mViewInfo.sel1);

   double tsel = mViewInfo.sel0;

   TrackListIterator iter(mTracks);
   TrackListIterator clipIter(msClipboard);

   VTrack *n = iter.First();
   VTrack *c = clipIter.First();

   while (n && c) {
      if (n->GetSelected()) {
         if (msClipProject != this && c->GetKind()==VTrack::Wave)
            ((WaveTrack *)c)->Lock();
         
         n->Paste(tsel, c);
         
         if (msClipProject != this && c->GetKind()==VTrack::Wave)
            ((WaveTrack *)c)->Unlock();
         
         c = clipIter.Next();
      }

      n = iter.Next();
   }

   // TODO: What if we clicked past the end of the track?

   mViewInfo.sel0 = tsel;
   mViewInfo.sel1 = tsel + msClipLen;

   PushState(_("Pasted from the clipboard"));

   FixScrollbars();
   mTrackPanel->Refresh(false);
   //UpdateMenus();
}

void AudacityProject::OnDelete(wxCommandEvent & event)
{
   Clear();
}

void AudacityProject::OnSilence(wxCommandEvent & event)
{
   TrackListIterator iter(mTracks);

   VTrack *n = iter.First();

   while (n) {
      if (n->GetSelected())
         n->Silence(mViewInfo.sel0, mViewInfo.sel1);

      n = iter.Next();
   }

   PushState(
      wxString::Format(_("Silenced selected tracks for %.2f seconds at %.2f"),
                       mViewInfo.sel1 - mViewInfo.sel0, mViewInfo.sel0));

   mTrackPanel->Refresh(false);
}

void AudacityProject::OnDuplicate(wxCommandEvent & event)
{
   TrackListIterator iter(mTracks);

   VTrack *n = iter.First();
   VTrack *dest = 0;

   TrackList newTracks;

   while (n) {
      if (n->GetSelected()) {
         n->Copy(mViewInfo.sel0, mViewInfo.sel1, &dest);
         if (dest) {
            dest->SetName(n->GetName());
            dest->SetLinked(n->GetLinked());

            // Use max( GetOffset, sel0 ) as the new offset.
            double newOffset = n->GetOffset();
            if (mViewInfo.sel0 > newOffset)
                newOffset = mViewInfo.sel0;
            dest->SetOffset(newOffset);

            newTracks.Add(dest);
         }
      }
      n = iter.Next();
   }

   TrackListIterator nIter(&newTracks);
   n = nIter.First();
   while (n) {
      mTracks->Add(n);
      n = nIter.Next();
   }

   PushState(_("Duplicated"));

   FixScrollbars();
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnSplit(wxCommandEvent & event)
{
   TrackListIterator iter(mTracks);

   VTrack *n = iter.First();
   VTrack *dest = 0;

   TrackList newTracks;

   while (n) {
      if (n->GetSelected()) {
         n->Copy(mViewInfo.sel0, mViewInfo.sel1, &dest);
         if (dest) {
            dest->SetLinked(n->GetLinked());
            dest->SetOffset(mViewInfo.sel0);
            newTracks.Add(dest);

            if (mViewInfo.sel1 >= n->GetMaxLen())
               n->Clear(mViewInfo.sel0, mViewInfo.sel1);
            else
               n->Silence(mViewInfo.sel0, mViewInfo.sel1);
         }
      }
      n = iter.Next();
   }

   TrackListIterator nIter(&newTracks);
   n = nIter.First();
   while (n) {
      mTracks->Add(n);
      n = nIter.Next();
   }

   PushState(_("Split"));

   FixScrollbars();
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnInsertSilence(wxCommandEvent & event)
{
   wxString amountStr =
       wxGetTextFromUser(_("Number of seconds of silence to insert:"),
                         _("Insert Silence"),
                         wxString::Format("%lf", mInsertSilenceAmount),
                         this, -1, -1, TRUE);

   if (amountStr == "")
      return;
   if (!amountStr.ToDouble(&mInsertSilenceAmount)) {
      wxMessageBox(_("Bad number of seconds."));
      return;
   }
   if (mInsertSilenceAmount <= 0.0) {
      wxMessageBox(_("Bad number of seconds."));
      return;
   }

   if (mViewInfo.sel0 != mViewInfo.sel1)
      Clear();

   wxASSERT(mViewInfo.sel0 == mViewInfo.sel1);

   TrackListIterator iter(mTracks);

   VTrack *n = iter.First();

   while (n) {
      if (n->GetSelected())
         n->InsertSilence(mViewInfo.sel0, mInsertSilenceAmount);

      n = iter.Next();
   }

   PushState(
       wxString::Format(_("Inserted %.2f seconds of silence at %.2f"),
                        mInsertSilenceAmount, mViewInfo.sel0));

   FixScrollbars();
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnSelectAll(wxCommandEvent & event)
{
   TrackListIterator iter(mTracks);

   VTrack *t = iter.First();
   while (t) {
      t->SetSelected(true);
      t = iter.Next();
   }
   mViewInfo.sel0 = mTracks->GetMinOffset();
   mViewInfo.sel1 = mTracks->GetMaxLen();

   mTrackPanel->Refresh(false);
   //UpdateMenus();
}

//
// View Menu
//

void AudacityProject::OnZoomIn(wxCommandEvent & event)
{
   mViewInfo.zoom *= 2.0;
   if (mViewInfo.zoom > 6000000)
      mViewInfo.zoom = 6000000;
   FixScrollbars();
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnZoomOut(wxCommandEvent & event)
{
   mViewInfo.zoom /= 2.0;
   FixScrollbars();
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnZoomNormal(wxCommandEvent & event)
{
   mViewInfo.zoom = 44100.0 / 512.0;
   FixScrollbars();
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnZoomFit(wxCommandEvent & event)
{
   ZoomFit();
}

void AudacityProject::OnPlotSpectrum(wxCommandEvent & event)
{
   int selcount = 0;
   WaveTrack *selt = NULL;
   TrackListIterator iter(mTracks);
   VTrack *t = iter.First();
   while (t) {
      if (t->GetSelected())
         selcount++;
      if (t->GetKind() == VTrack::Wave)
         selt = (WaveTrack *) t;
      t = iter.Next();
   }
   if (selcount != 1) {
      wxMessageBox(_("Please select a single track first.\n"));
      return;
   }

   /* This shouldn't be possible, since the menu is grayed out.
    * But we'll check just in case it does happen, to prevent
    * the crash that would result. */

   if(!selt) {
      wxMessageBox(_("Please select a track first.\n"));
      return;
   }


   sampleCount s0 = (sampleCount) ((mViewInfo.sel0 - selt->GetOffset())
                                   * selt->GetRate());
   sampleCount s1 = (sampleCount) ((mViewInfo.sel1 - selt->GetOffset())
                                   * selt->GetRate());
   sampleCount slen = s1 - s0;

   if (slen > 1048576)
      slen = 1048576;

   float *data = new float[slen];
   sampleType *data_sample = new sampleType[slen];

   if (s0 >= selt->GetNumSamples() || s0 + slen > selt->GetNumSamples()) {
      wxMessageBox(_("Not enough samples selected.\n"));
      delete[]data;
      delete[]data_sample;
      return;
   }

   selt->Get(data_sample, s0, slen);

   for (sampleCount i = 0; i < slen; i++)
      data[i] = data_sample[i] / 32767.;

   gFreqWindow->Plot(slen, data, selt->GetRate());
   gFreqWindow->Show(true);
   gFreqWindow->Raise();

   delete[]data;
   delete[]data_sample;
}

void AudacityProject::OnFloatPalette(wxCommandEvent & event)
{
   if (gWindowedPalette)
      HideWindowedPalette();
   else
      ShowWindowedPalette();
}

//
// Project Menu
//

void AudacityProject::OnImport(wxCommandEvent & event)
{
   wxString path = gPrefs->Read("/DefaultOpenPath",::wxGetCwd());

   wxString fileName = wxFileSelector(_("Select an audio file..."),
                                      path,     // Path
                                      "",       // Name
                                      "",       // Extension
                                      _("All files (*.*)|*.*|"
                                        "WAV files (*.wav)|*.wav|"
                                        "AIFF files (*.aif)|*.aif|"
                                        "AU files (*.au)|*.au|"
                                        "IRCAM files (*.snd)|*.snd|"
                                        "MP3 files (*.mp3)|*.mp3|"
                                        "Ogg Vorbis files (*.ogg)|*.ogg"),
                                      0,        // Flags
                                      this);    // Parent

   if (fileName != "") {
      path =::wxPathOnly(fileName);
      gPrefs->Write("/DefaultOpenPath", path);

      Import(fileName);
   }
}

void AudacityProject::OnImportLabels(wxCommandEvent & event)
{
   wxString path = gPrefs->Read("/DefaultOpenPath",::wxGetCwd());

   wxString fileName =
       wxFileSelector(_("Select a text file containing labels..."),
                      path,     // Path
                      "",       // Name
                      ".txt",   // Extension
                      _("Text files (*.txt)|*.txt|" "All files (*.*)|*.*"),
                      0,        // Flags
                      this);    // Parent

   if (fileName != "") {
      path =::wxPathOnly(fileName);
      gPrefs->Write("/DefaultOpenPath", path);

      wxTextFile f;

      f.Open(fileName);
      if (!f.IsOpened()) {
         wxMessageBox(_("Could not open file: ") + fileName);
         return;
      }

      LabelTrack *newTrack = new LabelTrack(&mDirManager);

      newTrack->Import(f);

      SelectNone();
      mTracks->Add(newTrack);
      newTrack->SetSelected(true);

      PushState(
         wxString::Format(_("Imported labels from '%s'"), fileName.c_str()));

      FixScrollbars();
      mTrackPanel->Refresh(false);
   }
}

void AudacityProject::OnImportMIDI(wxCommandEvent & event)
{
   wxString path = gPrefs->Read("/DefaultOpenPath",::wxGetCwd());

   wxString fileName = wxFileSelector(_("Select a MIDI file..."),
                                      path,     // Path
                                      "",       // Name
                                      "",   // Extension
                                      _("All files (*.*)|*.*|"
                                        "MIDI files (*.mid)|*.mid|"
                                        "Allegro files (*.gro)|*.gro"),
                                      0,        // Flags
                                      this);    // Parent

   if (fileName != "") {
      path =::wxPathOnly(fileName);
      gPrefs->Write("/DefaultOpenPath", path);

      NoteTrack *newTrack = new NoteTrack(&mDirManager);

      if (::ImportMIDI(fileName, newTrack)) {

         SelectNone();
         mTracks->Add(newTrack);
         newTrack->SetSelected(true);

         PushState(
             wxString::Format(_("Imported MIDI from '%s'"),
                              fileName.c_str()));

         FixScrollbars();
         mTrackPanel->Refresh(false);
      }
   }
}

void AudacityProject::OnImportRaw(wxCommandEvent & event)
{
   wxString path = gPrefs->Read("/DefaultOpenPath",::wxGetCwd());

   wxString fileName =
       wxFileSelector(_("Select any uncompressed audio file..."),
                      path,     // Path
                      "",       // Name
                      "",       // Extension
                      _("All files (*.*)|*.*"),
                      0,        // Flags
                      this);    // Parent

   if (fileName != "") {
      path =::wxPathOnly(fileName);
      gPrefs->Write("/DefaultOpenPath", path);

      WaveTrack *left = 0;
      WaveTrack *right = 0;

      if (::ImportRaw(this, fileName, &left, &right, &mDirManager)) {

         SelectNone();

         if (left) {
            mTracks->Add(left);
            left->SetSelected(true);
         }

         if (right) {
            mTracks->Add(right);
            right->SetSelected(true);
         }

         PushState(
             wxString::Format(_("Imported raw audio from '%s'"),
                              fileName.c_str()));

         FixScrollbars();
         mTrackPanel->Refresh(false);
      }
   }
}

void AudacityProject::OnEditID3(wxCommandEvent & event)
{
   mTags->ShowEditDialog(this, _("Edit ID3 Tags (for MP3 exporting)"));
}

void AudacityProject::OnQuickMix(wxCommandEvent & event)
{
   if (::QuickMix(mTracks, &mDirManager, mRate)) {

      // After the tracks have been mixed, remove the originals

      TrackListIterator iter(mTracks);
      VTrack *t = iter.First();

      while (t) {
         if (t->GetSelected())
            t = iter.RemoveCurrent();
         else
            t = iter.Next();
      }

      PushState(_("Quick mix"));

      FixScrollbars();
      mTrackPanel->Refresh(false);
   }
}

void AudacityProject::OnAlignZero(wxCommandEvent & event)
{
   TrackListIterator iter(mTracks);
   VTrack *t = iter.First();

   while (t) {
      if (t->GetSelected())
         t->SetOffset(0.0);

      t = iter.Next();
   }

   PushState(_("Aligned with zero"));

   mTrackPanel->Refresh(false);
   //UpdateMenus();
}

void AudacityProject::OnAlign(wxCommandEvent & event)
{
   double avg = 0.0;
   int num = 0;

   TrackListIterator iter(mTracks);
   VTrack *t = iter.First();

   while (t) {
      if (t->GetSelected()) {
         avg += t->GetOffset();
         num++;
      }

      t = iter.Next();
   }

   if (num) {
      avg /= num;

      TrackListIterator iter2(mTracks);
      t = iter2.First();

      while (t) {
         if (t->GetSelected())
            t->SetOffset(avg);

         t = iter2.Next();
      }
   }

   PushState(_("Aligned tracks"));

   mTrackPanel->Refresh(false);
   //UpdateMenus();

}

void AudacityProject::OnNewWaveTrack(wxCommandEvent & event)
{
   WaveTrack *t = new WaveTrack(&mDirManager);

   SelectNone();

   mTracks->Add(t);
   t->SetSelected(true);

   PushState(_("Created new audio track"));

   FixScrollbars();
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnNewLabelTrack(wxCommandEvent & event)
{
   LabelTrack *t = new LabelTrack(&mDirManager);

   SelectNone();

   mTracks->Add(t);
   t->SetSelected(true);

   PushState(_("Created new label track"));

   FixScrollbars();
   mTrackPanel->Refresh(false);
}

void AudacityProject::OnRemoveTracks(wxCommandEvent & event)
{
   TrackListIterator iter(mTracks);
   VTrack *t = iter.First();

   while (t) {
      if (t->GetSelected())
         t = iter.RemoveCurrent();
      else
         t = iter.Next();
   }

   PushState(_("Removed audio track(s)"));

   mTrackPanel->Refresh(false);
   //UpdateMenus();
}

//
// Help Menu
//

void AudacityProject::OnAbout(wxCommandEvent & event)
{
   AboutDialog dlog(this);
   dlog.ShowModal();
}

void AudacityProject::OnHelp(wxCommandEvent & event)
{
   ::ShowHelp(this);
}

void AudacityProject::OnHelpIndex(wxCommandEvent & event)
{
   ::ShowHelpIndex(this);
}

void AudacityProject::OnHelpSearch(wxCommandEvent & event)
{
   ::SearchHelp(this);
}

void AudacityProject::OnBenchmark(wxCommandEvent & event)
{
   ::RunBenchmark(this);
}
