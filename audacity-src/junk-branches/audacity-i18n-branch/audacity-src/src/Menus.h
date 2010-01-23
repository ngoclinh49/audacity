/**********************************************************************

  Audacity: A Digital Audio Editor

  Menus.h

  Dominic Mazzoni

  All of the menu bar handling is part of the class AudacityProject,
  but the event handlers for all of the menu items have been moved
  to Menus.h and Menus.cpp for clarity.

**********************************************************************/

#ifdef AUDACITY_MENUS_ENUM
enum {
   MenusFirstID = 1100,

   // File Menu

   NewID,
   OpenID,
   CloseID,
   SaveID,
   SaveAsID,
   ExportMixID,
   ExportSelectionID,
   ExportLossyMixID,
   ExportLossySelectionID,
   ExportLabelsID,
   PreferencesID,
   ExitID,

   // Edit Menu

   UndoID,
   RedoID,
   UndoHistoryID,

   CutID,
   CopyID,
   PasteID,

   DeleteID,
   SilenceID,

   InsertSilenceID,
   SplitID,
   DuplicateID,

   SelectAllID,

   // View Menu

   ZoomInID,
   ZoomOutID,
   ZoomNormalID,
   ZoomFitID,

   PlotSpectrumID,

   FloatPaletteID,

   // Project Menu

   ImportID,
   ImportLabelsID,
   ImportMIDIID,
   ImportRawID,
   
   EditID3ID,

   AlignZeroID,
   AlignID,

   NewWaveTrackID,
   NewLabelTrackID,
   RemoveTracksID,

   WaveDisplayID,
   SpectrumDisplayID,
   AutoCorrelateID,
   PitchID,
   QuickMixID,

   // Help Menu

   AboutID,
   HelpID,
   HelpIndexID,
   HelpSearchID,
   BenchmarkID,

   // Effect Menu

   FirstEffectID = 2000,

   // Plugin Menu

   FirstPluginID = 3000,
};
#endif

#ifdef AUDACITY_MENUS_EVENT_TABLE
  // File menu
    EVT_MENU(NewID, AudacityProject::OnNew)
    EVT_MENU(OpenID, AudacityProject::OnOpen)
    EVT_MENU(CloseID, AudacityProject::OnClose)
    EVT_MENU(SaveID, AudacityProject::OnSave)
    EVT_MENU(SaveAsID, AudacityProject::OnSaveAs)
    EVT_MENU(ExportMixID, AudacityProject::OnExportMix)
    EVT_MENU(ExportSelectionID, AudacityProject::OnExportSelection)
    EVT_MENU(ExportLossyMixID, AudacityProject::OnExportLossyMix)
    EVT_MENU(ExportLossySelectionID, AudacityProject::OnExportLossySelection)
    EVT_MENU(ExportLabelsID, AudacityProject::OnExportLabels)
    EVT_MENU(PreferencesID, AudacityProject::OnPreferences)
    EVT_MENU(ExitID, AudacityProject::OnExit)
    // Edit menu
    EVT_MENU(UndoID, AudacityProject::Undo)
    EVT_MENU(RedoID, AudacityProject::Redo)
    EVT_MENU(UndoHistoryID, AudacityProject::UndoHistory)

    EVT_MENU(CutID, AudacityProject::Cut)
    EVT_MENU(CopyID, AudacityProject::Copy)
    EVT_MENU(PasteID, AudacityProject::Paste)
    EVT_MENU(DeleteID, AudacityProject::OnDelete)
    EVT_MENU(SilenceID, AudacityProject::OnSilence)
    EVT_MENU(SplitID, AudacityProject::OnSplit)
    EVT_MENU(DuplicateID, AudacityProject::OnDuplicate)
    EVT_MENU(InsertSilenceID, AudacityProject::OnInsertSilence)
    EVT_MENU(SelectAllID, AudacityProject::OnSelectAll)
    // View menu
    EVT_MENU(ZoomInID, AudacityProject::OnZoomIn)
    EVT_MENU(ZoomOutID, AudacityProject::OnZoomOut)
    EVT_MENU(ZoomNormalID, AudacityProject::OnZoomNormal)
    EVT_MENU(ZoomFitID, AudacityProject::OnZoomFit)
    EVT_MENU(PlotSpectrumID, AudacityProject::OnPlotSpectrum)
    EVT_MENU(FloatPaletteID, AudacityProject::OnFloatPalette)
    // Project menu
    EVT_MENU(ImportID, AudacityProject::OnImport)
    EVT_MENU(ImportLabelsID, AudacityProject::OnImportLabels)
    EVT_MENU(ImportMIDIID, AudacityProject::OnImportMIDI)
    EVT_MENU(ImportRawID, AudacityProject::OnImportRaw)
    EVT_MENU(EditID3ID, AudacityProject::OnEditID3)
    EVT_MENU(AlignID, AudacityProject::OnAlign)
    EVT_MENU(AlignZeroID, AudacityProject::OnAlignZero)
    EVT_MENU(QuickMixID, AudacityProject::OnQuickMix)
    EVT_MENU(NewWaveTrackID, AudacityProject::OnNewWaveTrack)
    EVT_MENU(NewLabelTrackID, AudacityProject::OnNewLabelTrack)
    EVT_MENU(RemoveTracksID, AudacityProject::OnRemoveTracks)
    // Help menu
    EVT_MENU(AboutID, AudacityProject::OnAbout)
    EVT_MENU(HelpID, AudacityProject::OnHelp)
    EVT_MENU(HelpIndexID, AudacityProject::OnHelpIndex)
    EVT_MENU(HelpSearchID, AudacityProject::OnHelpSearch)
    EVT_MENU(BenchmarkID, AudacityProject::OnBenchmark)
    // Update menu method
    EVT_UPDATE_UI(UndoID, AudacityProject::OnUpdateMenus)
#endif
#ifdef AUDACITY_MENUS_METHODS
private:
double mInsertSilenceAmount;

wxString mExportString;
wxString mExportSelectionString;
wxString mExportLossyString;
wxString mExportSelectionLossyString;

int      mMenusDirtyCheck;

bool mLastNonZeroRegionSelected;

int mLastNumTracks;
int mLastNumTracksSelected;
int mLastNumWaveTracks;
int mLastNumWaveTracksSelected;
int mLastNumLabelTracks;

bool mFirstTimeUpdateMenus;

public:
void CreateMenuBar();

void OnUpdateMenus(wxUpdateUIEvent & event);

        // File Menu

void OnNew(wxCommandEvent & event);
void OnOpen(wxCommandEvent & event);
void OnClose(wxCommandEvent & event);
void OnSave(wxCommandEvent & event);
void OnSaveAs(wxCommandEvent & event);

void OnExportMix(wxCommandEvent & event);
void OnExportSelection(wxCommandEvent & event);
void OnExportLossyMix(wxCommandEvent & event);
void OnExportLossySelection(wxCommandEvent & event);

void OnExportLabels(wxCommandEvent & event);

void OnPreferences(wxCommandEvent & event);

void OnExit(wxCommandEvent & event);

        // Edit Menu

void Undo(wxCommandEvent & event);
void Redo(wxCommandEvent & event);
void UndoHistory(wxCommandEvent & event);

void Cut(wxCommandEvent & event);
void Copy(wxCommandEvent & event);
void Paste(wxCommandEvent & event);

void OnDelete(wxCommandEvent & event);
void OnSilence(wxCommandEvent & event);

void OnInsertSilence(wxCommandEvent & event);
void OnSplit(wxCommandEvent & event);
void OnDuplicate(wxCommandEvent & event);

void OnSelectAll(wxCommandEvent & event);

        // View Menu

void OnZoomIn(wxCommandEvent & event);
void OnZoomOut(wxCommandEvent & event);
void OnZoomNormal(wxCommandEvent & event);
void OnZoomFit(wxCommandEvent & event);

void ZoomFit();

void OnPlotSpectrum(wxCommandEvent & event);

void OnFloatPalette(wxCommandEvent & event);

        // Project Menu

void OnImport(wxCommandEvent & event);
void OnImportLabels(wxCommandEvent & event);
void OnImportMIDI(wxCommandEvent & event);
void OnImportRaw(wxCommandEvent & event);

void OnEditID3(wxCommandEvent & event);

void OnQuickMix(wxCommandEvent & event);

void OnAlignZero(wxCommandEvent & event);
void OnAlign(wxCommandEvent & event);

void OnNewWaveTrack(wxCommandEvent & event);
void OnNewLabelTrack(wxCommandEvent & event);
void OnRemoveTracks(wxCommandEvent & event);

        // Help Menu

void OnAbout(wxCommandEvent & event);
void OnHelp(wxCommandEvent & event);
void OnHelpIndex(wxCommandEvent & event);
void OnHelpSearch(wxCommandEvent & event);
void OnBenchmark(wxCommandEvent & event);

#endif
