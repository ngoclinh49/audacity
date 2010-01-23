/**********************************************************************

  Audacity: A Digital Audio Editor

  ExportMultiple.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_EXPORT_MULTIPLE__
#define __AUDACITY_EXPORT_MULTIPLE__

#include <wx/dialog.h>
#include <wx/string.h>
#include <wx/dynarray.h>   // sadly we are using wx dynamic arrays

#include "Export.h"
#include "../Track.h"
#include "../Tags.h"       // we need to know about the Tags class for metadata

class wxButton;
class wxCheckBox;
class wxChoice;
class wxRadioButton;
class wxTextCtrl;

class AudacityProject;
class ShuttleGui;

class ExportMultiple : public wxDialog
{
public:

   ExportMultiple(AudacityProject *parent);
   virtual ~ExportMultiple();

   int ShowModal();

private:

   // Export
   void CanExport();
   void CountTracksAndLabels();
   bool DirOk();
   /** \brief Export multiple labeled regions of the project to separate files
    *
    * Uses a single label track in the project to split up the audio into a 
    * series of sections, each of which is exported to a separate file.
    * @param byName Controls whether files are named after the text in the
    * labels that define them (true), or just numbered (false).
    * @param prefix The string used to prefix the file number if files are being
    * numbered rather than named */
   bool ExportMultipleByLabel(bool byName, wxString prefix);

   /** \brief Export each track in the project to a separate file
    *
    * @param byName Controls whether files are named after the track names 
    * (true), or just numbered (false).
    * @param prefix The string used to prefix the file number if files are being
    * numbered rather than named */
   bool ExportMultipleByTrack(bool byName, wxString prefix);

   /** Export one file of an export multiple set
    *
    * Called once for each file in the list to do a (non-interactive) export
    * @param channels Number of channels to export
    * @param name The file name (and path) to export to
    * @param selectecOnly Should we  export the selected tracks only?
    * @param t0 Start time for export
    * @param t1 End time for export
    * @param tags Metadata to include in the file (if possible).
    */
   bool DoExport(int channels,
                 wxFileName name,
                 bool selectedOnly,
                 double t0,
                 double t1,
                 Tags tags);
   /** \brief Takes an arbitrary text string and converts it to a form that can
    * be used as a file name, if necessary prompting the user to edit the file
    * name produced */
   wxString MakeFileName(wxString input);
   // Dialog
   void PopulateOrExchange(ShuttleGui& S); 
   void EnableControls();

   void OnFormat(wxCommandEvent& event);
   void OnOptions(wxCommandEvent& event);
   void OnCreate(wxCommandEvent& event);
   void OnChoose(wxCommandEvent& event);
   void OnLabel(wxCommandEvent& event);
   void OnFirst(wxCommandEvent& event);
   void OnFirstFileName(wxCommandEvent& event);
   void OnTrack(wxCommandEvent& event);
   void OnByName(wxCommandEvent& event);
   void OnByNumber(wxCommandEvent& event);
   void OnPrefix(wxCommandEvent& event);
   void OnCancel(wxCommandEvent& event);
   void OnExport(wxCommandEvent& event);

private:
   Exporter mExporter;
   ExportPluginArray mPlugins;
   AudacityProject *mProject;
   TrackList *mTracks;           /**< The list of tracks in the project that is
                                   being exported */
   TrackListIterator mIterator;  /**< Iterator used to work through all the
                                   tracks in the project */
   LabelTrack *mLabels;
   int mNumLabels;
   int mNumWaveTracks;
   wxArrayPtrVoid mSelected;
   int mFilterIndex;
   int mFormatIndex;
   int mSubFormatIndex;
   bool mInitialized;

   /** Array of characters not allowed to be in file names on this platform */
   wxArrayString exclude;

   wxChoice      *mFormat;
   wxButton      *mOptions;

   wxTextCtrl    *mDir;    /**< The directory all the exported files will end
                             up in */
   wxButton      *mCreate;
   wxButton      *mChoose;
   
   wxRadioButton *mLabel;
   wxStaticText  *mLabelLabel;

   wxCheckBox    *mFirst;
   wxStaticText  *mFirstFileLabel;
   wxTextCtrl    *mFirstFileName;   /**< Name to use for exporting audio before
                                      the first label in the file */

   wxRadioButton *mTrack;
   wxStaticText  *mTrackLabel;
   
   wxRadioButton *mByName;
   wxStaticText  *mByNameLabel;

   wxRadioButton *mByNumber;
   wxStaticText  *mByNumberLabel;

   wxStaticText  *mPrefixLabel;
   wxTextCtrl    *mPrefix;

   wxCheckBox    *mOverwrite;

   wxButton      *mCancel;
   wxButton      *mExport;

   DECLARE_EVENT_TABLE()
      
};

/** \brief A private class used to store the information needed to do an
    * export.
    *
    * We create a set of these during the interactive phase of the export
    * cycle, then use them when the actual exports are done */
   class ExportKit
   {
   public:
      Tags filetags; /**< The set of metadata to use for the export */
      wxFileName destfile; /**< The file to export to */
      double t0;           /**< Start time for the export */
      double t1;           /**< End time for the export */
   };  // end of ExportKit declaration
   /* we are going to want an set of these kits, and don't know how many until
    * runtime. I would dearly like to use a std::vector, but it seems that
    * this isn't done anywhere else in Audacity, presumably for a reason?, so
    * I'm stuck with wxArrays, which are much harder, as well as non-standard.
    */
   WX_DECLARE_OBJARRAY(ExportKit, ExportKitArray);


#endif

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: d6904b91-a320-4194-8d60-caa9175b6bb4
