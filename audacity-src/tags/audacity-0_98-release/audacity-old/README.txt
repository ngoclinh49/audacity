Audacity: A Free, Cross-Platform Digital Audio Editor

Version 0.98 (January 21, 2002)

http://audacity.sourceforge.net/

Primary author:

  Dominic Mazzoni <dominic@minorninth.com>

Authors:

  Dominic Mazzoni <dominic@minorninth.com>
  Roger Dannenberg <rbd+@cs.cmu.edu>
  Jason Cohen <cohen3+@andrew.cmu.edu>
  Robert Leidle <rfl+@andrew.cmu.edu>
  Mark Tomlinson <marktoml@hotmail.com>
  Joshua Haberman <joshua@haberman.com>
  Nasca Octavian Paul <paulnasca@email.ro> or <paulnasca@yahoo.com>
  Logan Lewis <proxima@proxc.com>
  Matt Brubeck <mbrubeck@hmc.edu>
  Mark Phillips <mitb@totaldeath.com>
  Tony Oetzmann <airon@epost.de>

Icons and logo:

  Harvey Lubin <agrapha@agrapha.com>
  http://www.agrapha.com/

Aqua/MacOS graphics:

  Tom Woodhams <tom@imaginemedia.co.uk>

For changelog, see the bottom of this document

-------------------------------------------------------------

With the exception of the Xaudio library for importing MP3 files,
this program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Xaudio library is commercial software and has been licensed
for use in this program.  For more information, see their
website at http://www.xaudio.com/

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program (in a file called LICENSE.txt); if not, go to
http://www.gnu.org/copyleft/gpl.html or write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

-------------------------------------------------------------

Source code to this program is always available; for more
information visit our website at:

  http://audacity.sourceforge.net/

This program uses wxWindows, a cross-platform GUI toolkit.  To
compile this program, you will need to download wxWindows from:

  http://www.wxwindows.org/

To compile on Linux and other Unix systems, simply run:

  ./configure
  make
  make install

There are a few options to the configure script.  You can
see them by running "./configure --help".
  
If you want to do any development, you might want to generate
dependencies:

  make dep

Project files for VC++ on Windows and CodeWarrior for Mac
are included with the source code.  If you are having
trouble compiling on a non-Unix platform, please email
audacity-devel@lists.sourceforge.net

-------------------------------------------------------------

Known issues/problems:

* All platforms: There is no warning asking you if you want
  to save changes when you quit or close a window.

* Windows: The floating tool palette does not minimize when
  minimizing a project window.  As a workaround, you can
  just keep the tool palette docked with the project window.

* Linux: Full duplex (play one track while recording another)
  does not seem to work, even if your sound card supports it.
  This is likely a problem with the way we are using OSS.

* Linux: Sound cards that only deal with more than two
  channels are not yet supported.

-------------------------------------------------------------

Changes in 0.98:

* Effects

  - New Invert effect

  - New Reverse effect

  - Improvements to Noise Removal effect

* Bug fixes

  - You can now safely copy and paste between projects.

  - Fixed bugs loading, saving, and undoing envelopes.

  - You're not allowed to open the same project in two different
    windows anymore (it just would have caused data loss).

  - Projects now save the channel of each track, it's no longer
    forgotten

  - Better handling of case when temp directory is invalid at
    start of program

  - Doesn't overwrite a file the project was depending on anymore
    (it renames the old file).  You can now export to a file
	 with the same as the file you imported without any problems.

  - Save As... for a project doesn't destroy the old project anymore.

  - Undo information is thrown away when you close a project,
    saving disk space that had been wasted in previous versions.

  - Save As... recovers gracefully when you try to save to a
    bad location

  - Duplicate of a track with an offset now works

  - Fixed another crash in Amplify... (if no data is selected in
    one of the tracks)

  - Windows: recording/playback devices are no longer reversed
    in the preferences!

  - Windows: VST plug-ins are found no matter how Audacity is
    launched.

  - Windows: You can now import Ogg Vorbis files.

  - If recording fails, a phantom track is no longer created.

  - Changing the export format now changes the menu bar.

  - More minor bug fixes...

* Unix:

  - New OSS code, should provide better playback and record on
    almost all OSS Unix systems.

  - Preliminary support for the KDE/aRts soundserver (a compile-time
    option; must be configured to use this in place of OSS code)

  - Bug fixes to "make install" script

Changes in 0.97:

* Installation/configuration

  - New Windows installer

  - More options in the configure script on Unix

* User interface:

  - Fixed Effects menu bug that would freeze if some but
    not all tracks were selected.

  - Added Noise Removal effect (Dominic)

  - Improved click-drag zoom.

  - Support drag-and-drop to import audio files (Windows only)

  - Improved Export file dialog (asks about strange extensions)

  - Other bug fixes

* MacOS:

  - Fixed autoscrolling bug (would sometimes appear if you
    release the cursor outside the window while selecting).

* Unix:

  - Fixed Amplify bug (would sometimes freeze trying to
    open the dialog).

Changes in 0.96:

* General User Interface:

  - Added mute/solo buttons

* Importing Audio:

  - Fixed regression bug in 0.95 which caused stereo files to be imported as
    two mono tracks
    
  - Imports MP3 ID3 tags

* Exporting Audio:

  - Exporting MP3 now works, if the appropriate version of the LAME DLL is
    installed (Joshua)
  
  - Allows editing of MP3 ID3 tags with export.

* Preferences:

  - Added Audio I/O selectors on Mac (Dominic) and Windows (Joshua)

* Effects:

  - Added progress dialog support to all effects (which also allows
    effects to be cancelled)
  
  - Added support for stereo effects and effects that add or
    remove tracks, or require multiple passes.
  
  - Improved Amplify effect and fixed all known bugs (Dominic)
  
  - Improved Bass Boost effect
  
  - Added Filter effect (Dominic)
  
  - Added Phaser effect (Paul)
  
  - Added Wahwah effect (Paul)

Changes in 0.95:

* Installation/Compilation:

  - Improved configure script on unix systems (Joshua)

* General User Interface:

  - Menu items are now disabled when unavailable

  - Online help added (Dominic and Logan)

* Importing Audio:

  - Lazy import added, speeding up importing of PCM files by 2x

  - Added support for the Free libmpeg3 library on unix 
    to replace the proprietary xaudio (Joshua)

  - Importing MP3 and Ogg Vorbis files is now handled automatically
    by the Open and Import commands.

  - Fixed the Import Raw Data feature, so now you can
    import files of almost any arbitrary format (as long
    as it's uncompressed).

* Main window:

  - New track labels with a single integrated pop-up menu
    to handle all track options

  - Vertical ruler added, along with preliminary support for
    vertical zooming

  - Stereo tracks can be linked together so changes affect
    both tracks

  - Point-sample display takes over when you zoom very far in

  - Two new wave displays: a dB (logarithmic) wave display and
    a spectral pitch display (using enhanced autocorrelation)

* Preferences:

  - New spectral display preferences

  - Temp directory can be set in preferences

* Frequency display:

  - Many new frequency window enhancements, including support for
    cepstrum, autocorrelation, and enhanced autocorrelation.

* Envelope editor:

  - Envelopes are now interpolated using decibels, making
    cross-fades sound much better

* Effects:

  - Fixed a bug that caused incompatibility with many VST plug-ins.

  - Added Maximize Amplitude effect

  - Added Bass Boost effect (Paul)

* Other:

  - Improved memory management over long Undo histories

  - Many more bug fixes

Changes in 0.94:

* Preferences dialog (Joshua Haberman)

* OGG Vorbis import (Joshua Haberman)

* Silence, Insert Silence commands

* Split and Duplicate commands

* Mac OS X support

* Supports recording on Mac OS 8 and 9

* Many bug fixes

Changes in 0.93:

* Displays playback/recording position indicator

* Keeps track of some preferences

* Supports arbitrary project sample rate

* Mac: opens documents from the Finder

* Floating tool palette is now dockable
  (and docked by default)

* Fixed bugs in handling multiple open projects

* Supports recording (Windows, Linux)

* Frequency Window displays note names (i.e. C4, G#5)

* Many bug fixes for effects, including VST plug-in effects

Changes in 0.92:

* Added Frequency Plot window and improved Spectrum display

* Fixed bug in File:Open when the file to be opened was
  actually a large WAV file

Changes in 0.91:

* Uses xaudio library to import mp3 files

* Zoom menu

Changes in 0.9:

* New floating tool palette with four tools (selection,
  sliding, zooming, and envelope editing) plus play and
  stop buttons

* Playback now mixes tracks, and you can work with the
  document while listening.  The stop button works.

* Rewritten file handling functions.  The main view
  is no longer dependent on the wxWindows DocView
  classes, so we can handle files ourselves.  The
  project file format is now text-based for easy
  debugging.  Eventually it will probably move to XML.

* Improved handling of wave tracks: as before, the data
  is stored in blocks, but now, the blocks are correctly
  limited to betweek n and 2n bytes each (for some n),
  which guarantees editing operations always take the
  same amount of time, while also ensuring that projects
  don't get more fragmented over time.

* Rewritten user interface code.  The shades of gray
  are taken from the OS, and the project window has been
  redesigned to have more consistent layout across all
  platforms.

* Selecting "Open" now does the smart thing, opening a
  project if you give it a project, or importing a WAV
  file if you give it that.

* Flashing cursor indicates the current editing position

* Much improved ruler - besides looking nicer, the ruler
  now displays the selection and the cursor.

* The zoom tool centers on the cursor so you can zoom
  into wherever you are.

