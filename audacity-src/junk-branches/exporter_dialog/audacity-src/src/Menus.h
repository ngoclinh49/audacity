/**********************************************************************

  Audacity: A Digital Audio Editor

  Menus.h

  Dominic Mazzoni

  All of the menu bar handling is part of the class AudacityProject,
  but the event handlers for all of the menu items have been moved
  to Menus.h and Menus.cpp for clarity.

**********************************************************************/

void CommandManagerCallback(void *fptr);
void CommandManagerListCallback(void *fptr, int index);

void CreateMenusAndCommands();

void ModifyExportMenus();
void ModifyUndoMenus();

double NearestZeroCrossing(double t0);

// used in routines OnSelectionSave
// and OnSelectionRestore
double mSel0save;
double mSel1save;
 
        // Audio I/O Commands

void OnStop();
void OnPause();
void OnRecord();
void OnSkipStart();
void OnSkipEnd();

        // Different posibilities for playing sound

bool MakeReadyToPlay(); // Helper function that sets button states etc.
void OnPlayStop();
void OnPlayOneSecond();
void OnPlayToSelection();
void OnPlayLooped();

        // Selection-Editing Commands

void OnCursorLeft();
void OnCursorRight();
void OnSelExtendLeft();
void OnSelExtendRight();
void OnSelContractLeft();
void OnSelContractRight();
void OnSelToStart();
void OnSelToEnd();

void OnZeroCrossing();

        // File Menu

void OnNew();
void OnOpen();
void OnClose();
void OnSave();
void OnSaveAs();

void OnExportMix();
void OnExportSelection();
void OnExportLossyMix();
void OnExportLossySelection();

void OnExportLabels();

void OnPreferences();

void OnExit();

        // Edit Menu

void OnUndo();
void OnRedo();
void OnHistory();

void OnCut();
void OnCopy();
void OnPaste();
void OnPasteOver();
void OnTrim();

void OnDelete();
void OnSilence();

void OnSplit();
void OnSplitLabels();
void OnDuplicate();

void OnSelectAll();
void OnSelectCursorEnd();
void OnSelectStartCursor();

        // View Menu

void OnZoomIn();
void OnZoomOut();
void OnZoomNormal();
void OnZoomFit();
void OnZoomSel();

void OnSelectionFormat(int index);
void OnSnapOn();
void OnSnapOff();

void OnPlotSpectrum();

void OnFloatControlToolBar();
void OnLoadEditToolBar();
void OnFloatEditToolBar();
void OnLoadMixerToolBar();
void OnFloatMixerToolBar();


        // Project Menu

void OnImport();
void OnImportLabels();
void OnImportMIDI();
void OnImportRaw();

void OnEditID3();

void OnQuickMix();

void OnSelectionSave();
void OnSelectionRestore();

void OnCursorTrackStart();
void OnCursorTrackEnd();
void OnCursorSelStart();
void OnCursorSelEnd();

void OnAlign(int index);
void OnAlignMoveSel(int index);
void HandleAlign(int index, bool moveSel);

void OnNewWaveTrack();
void OnNewStereoTrack();
void OnNewLabelTrack();
void OnNewTimeTrack();
void OnRemoveTracks();
void OnAddLabel();

        // Effect Menu

void OnEffect(int type, int index);
void OnGenerateEffect(int index);
void OnGeneratePlugin(int index);
void OnProcessEffect(int index);
void OnProcessPlugin(int index);
void OnAnalyzeEffect(int index);
void OnAnalyzePlugin(int index);

        // Help Menu

void OnAbout();
void OnHelp();
void OnHelpIndex();
void OnHelpSearch();
void OnBenchmark();

       //

void OnSeparator();

