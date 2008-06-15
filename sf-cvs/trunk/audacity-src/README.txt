
Audacity(R): A Free, Cross-Platform Digital Audio Editor
WWW:   http://audacity.sourceforge.net/

Audacity is copyright (c) 1999-2008 by Audacity Team.
This copyright notice applies to all documents in the
Audacity source code archive, except as otherwise noted
(mostly in the lib-src subdirectories).

"Audacity" is a registered trademark of Dominic Mazzoni.

Version 1.3.6a2 (alpha)


Contents of this README:

1.  Licensing
2.  Changes in version 1.3.6a2
3.  Known Issues
4.  Source Code, Libraries and Additional Copyright Information
5.  Compilation Instructions
6.  Previous Changes going back to version 1.1.0

--------------------------------------------------------------------------------

1. Licensing

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at your
option) any later version. The program source code is also freely
available as per Section 4 of this README.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with this program (in a file called LICENSE.txt); if not, go
to http://www.gnu.org/copyleft/gpl.html or write to

  Free Software Foundation, Inc.
  59 Temple Place - Suite 330
  Boston, MA 02111-1307 USA


--------------------------------------------------------------------------------

2.  Changes in version 1.3.6a2

Import / Export:
        * Experimental support for importing a much wider range
           of audio formats via FFmpeg: support has to be enabled
           in *config when building and requires FFmpeg libraries

Effects:
        * Built-in plug-ins now grouped into related categories

Interface:
        * New Debug Log window available in all builds
        * Experimental support for pairing a label track with any
           number of audio tracks so that labels shift with cuts and
           inserts in the audio track
        * Default theme now reverted to that of 1.3.5
        * Recording channels preference now defaults to stereo

Miscellaneous:
        * Bug fixes for shortcut availability/tab order in Selection Bar,
           and for window focus issues when previewing effects
        * Improvements in escaping and navigating fields in dialogs,
           and in stabilty when screen readers are used


--------------------------------------------------------------------------------

3. Known Issues

Please also check:
  http://audacityteam.org/wiki/index.php?title=Known_Issues

for details of any issues that have been identified after release of
this version.

 * Using "Undo" when a label track is paired with any audio
    track causes the pairing to break, and can cause crashes.

 * Preferences window: OK button does not work when a tab
    is selected in the left-hand panel.

 * When "Split New" is used with more than one track selected, and
    the selected region includes white space, the split clip(s) perform
    unwanted alignments instead of remaining at the original time
    position.

 * Export Multiple fails with no export or warning if an empty label is
    encountered.

 * "Audio Cache" on the Directories tab of Preferences caches most audio
    data for the duration of the session, including project data and imported
    files. Enabling it could cause a crash when making long recordings or
    opening large files or projects.

 * Previewing the current curve on opening Equalization, modifying that
    curve and then previewing again plays the audio without the modified
    equalization applied.

 * A few interface elements do not change language without restart.

 * Calculation of "disk space remains for recording (time)" is incorrect when
    recording in 24 bit quality. You may record for 50% longer than the
    indicated time.

 * Pressing Play (but not spacebar) in a second project when another is already
    playing stops playback of the first project.

 * Projects created by Audacity 1.1.x may open incorrectly.

 * Metadata such as ID3 tags is not fully imported and exported for all supported
    file formats. MP3 tags *are* correctly imported and exported to the current ID3
    specification, but some players don't fully support this specification, so may not
    see all the tags.

 * No warning given if File > Save or File > Save Project As... is carried out with
    no tracks open.

 * Not all menu items correctly enabled when the preference: "Select all audio in
    project, if none selected" is checked.

 * Beep on completing long process may not be audible on many systems.

 * Audacity can import and display MIDI files, but they cannot be played
    or edited.

 * Windows only: Welcome Message: On some systems/browsers, links are not
    brought to top, and some screen readers that otherwise work well with Audacity
    cannot read its text.

 * Windows (reported on): There have been reports of clicks during recording on
    some Windows XP systems using Audacity 1.3.4, where 1.2.6 had no problem.
    It is not clear if current releases will have this issue or if it will occur on other
    operating systems. Users can help us by sending any reports of this problem to:
      audacity-devel@lists.sourceforge.net

 * Windows only: Audacity is incompatible with some professional sound cards
    and may crash if one of these cards is the default when you open Audacity.
    As a workaround, make a different sound card your default when using
    Audacity, but please let us know if this affects you so that we can track down
    and solve the problem.

 * Windows Vista only: If no input device (such as a microphone) is enabled
    and connected, clicking Help > Audio Device Info causes a crash.

 * Mac OS X only: Some users find that after running Audacity other media
    players don't produce any sound or crash. Audacity tries to select the best
    quality settings your system is capable of, to give the best recordings
    possible. Some sound drivers also retain these settings as defaults for
    other applications, which can cause these symptoms.

   To get round this, enable the option "Do not modify audio device settings"
   on the Audio I/O tab of the preferences, and make sure that your sound
   device is set up (in the Apple Sound and Midi Setup utility) to work in
   stereo, 16bits, with a sample rate of 44100Hz or 48000Hz.  More help at:
      http://audacityteam.org/wiki/index.php?title=Mac_Bugs#Loss_of_sound_after_running_Audacity

 * Mac OS X only: If using Audacity when the "Hear" audio plug-in is running
   (or has been since boot), there will be excessive memory usage which could
   cause a crash. This appears to be due to buggy memory allocation in "Hear".

 * Mac OS X only: Portable settings aren't picked up, and the default
    settings (in the default location) are always used. See this page for
    a workround:
      http://audacityteam.org/wiki/index.php?title=Portable_Audacity

 * Linux only: Audacity now supports interfacing with Jack, however this has
    not been tested, and is known to have a number of issues with both
    reliability and useability. Patches to improve both will be welcomed.

 * Linux (reported on): The first file imported into a project no longer
    changes the project rate to the file's rate, if that rate is absent from the
    project rate list.

 * Linux (Debian-derived) only: Audacity configure script does not detect
    libsoundtouch on the system and so Change Pitch and Change Tempo
    effects are disabled. This is a debian bug (#476699), which can be
    worked around by symlinking /usr/lib/pkgconfig/soundtouch-1.0.pc
    to /usr/lib/pkgconfig/libSoundTouch.pc.

Also note that the Windows installer will not replace 1.2.x installations, but will
install alongside them.


--------------------------------------------------------------------------------

4.  Source Code, Libraries and Additional Copyright Information

Source code to this program is always available; for more information visit
our website at:

  http://audacity.sourceforge.net/download/

Audacity is built upon other free libraries; some of these libraries may have
come with Audacity in the lib-src directory.  Others you are expected to install
first if you want Audacity to have certain capabilities.  Most of these libraries
are not distributed under the terms of the GPL, but rather some other free,
GPL-compatible license.  Specifically:

  wxWidgets: wxWindows license (based on LGPL)
    Cross-platform GUI library - must be downloaded and
    compiled separately.

  expat: BSD-like license.
    Provides XML parsing.  Included with Audacity

  iAVC: LGPL
    Part of the code to the AVC Compressor effect.
    Included with Audacity.

  libid3tag: GPL
    Reads/writes ID3 tags in MP3 files.  Optional
    separate download as part of libmad.

  libmad: GPL
    Decodes MP3 files.  Optional separate download.

  libnyquist: BSD-like license.
    Functional language for manipulating audio; available
    within Audacity for effects processing.

  libogg: BSD-like license.
    Optional separate download, along with libvorbis.

  libsndfile: LGPL
    Reads and writes uncompressed PCM audio files.
    Included with Audacity.

  libvorbis: BSD-like license.
    Decodes and encodes Ogg Vorbis files.  Optional
    separate download.

  SoundTouch: LGPL
    Changes tempo without changing pitch and vice versa.
    Included in audacity

  Twolame: LGPL
    Encodes MPEG I layer 2 audio (used in DVDs and Radio). Optional separate
    download.

For more information, see the documentation inside each library's
source code directory.

--------------------------------------------------------------------------------
Additional copyright information:
--------------------------------------------------------------------------------

Nyquist

Copyright (c) 2000-2002, by Roger B. Dannenberg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

Redistributions of source code must retain the copyright notice, the
list of conditions, and the disclaimer, all three of which appear below under
"COPYRIGHT AND LICENSE INFORMATION FOR XLISP."

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

Redistributions in binary form must reproduce the copyright notice, the
list of conditions, and the disclaimer, all three of which appear below under
"COPYRIGHT AND LICENSE INFORMATION FOR XLISP," in the documentation
and/or other materials provided with the distribution.

Neither the name of Roger B. Dannenberg, Carnegie Mellon University, nor the
names of any contributors may be used to endorse or promote products derived
from this software without specific prior written permission.

COPYRIGHT AND LICENSE INFORMATION FOR XLISP (part of Nyquist):

Copyright (c) 1984-2002, by David Michael Betz
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

Neither the name of David Michael Betz nor the names of any contributors may be
used to endorse or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


--------------------------------------------------------------------------------

5. Compilation instructions

First you must download wxWidgets. Audacity 1.3.6 and later requires
wxWidgets 2.8.7 from:

  http://www.wxWidgets.org/

If you install the RPM, make sure you install the devel RPM as well, otherwise you
won't be able to compile Audacity from source.

To compile on Linux, Mac OS X, and other Unix systems, simply execute these
commands:

  ./configure
  make
  make install  # as root

To see compile-time options you can set, you can type
"./configure --help".

If you want to do any development, you might want to generate a configure cache
and header dependencies:

  ./configure -C
  make dep

To compile on Windows using MSVC++, please follow the instructions found in
compile.txt in the "win" subdirectory. CodeWarrior for Mac is also supported.

For more information on compilation, please visit:

  http://audacityteam.org/wiki/index.php?title=Developer_Guide#Platform_Specific_Guides

or email us at:

  audacity-devel@lists.sourceforge.net

--------------------------------------------------------------------------------

6.  Previous Changes going back to version 1.1.0

Changes in 1.3.6a1:

Interface:
	* Further improvements to menu navigation and wordings.
	* All file dialogs are now resizable, and support "Places"
	   sidebar on Windows 2000 or later.
	* Preferences:
	        * New "Theme" preference for modifying interface
	           colours and images, with experimental new default
	           colour scheme.
	        * New "Smart Recording" preference automatically pauses
	           recordings that fall below a pre-defined input level.

Compilation:
        * Simplified modular builds for Windows, removing
           static-linked configurations.
        * New shared configurations on Mac to support modular
           builds, and all builds are now Unicode.

Miscellaneous:
        * Default auto save interval reduced to 2 minutes.
        * Bug fixes to correct project rate initialisation on Linux, and
           file importing issues on PPC Macs.


Changes in 1.3.5:

Recording  / Playback:
	* Several bugs fixed so that latency correction should be better, and more
	   devices work correctly. Problems with invalid sample rates under Linux
	   should be much rarer.
	* Newer version of Portaudio library.
	* New feature to record onto the end of an existing track
	   (hold Shift while clicking Record).

Import / Export:
	* Updated versions of Libogg, Libvorbis, Libflac, Libsndfile and Twolame
	   libraries.
	* Handling of unsupported file formats more informative.
	* Handling of file names with slashes on OS X improved. New dialog
	   allows replacement of illegal file name characters on all platforms.

Interface:
	* Improved scaling and layout for rulers and VU meters.
	* Envelope fixes/improvements including full control of undo/redo.
	* New keyboard shortcuts and improved menu navigation.
	* Preferences: More intuitive tab arrangement. New options for
	   mute/solo and Metadata Editor behavior. Language can now be
	   changed without restart.
	* Expanded Build Information tab.

Effects:
	* New Vocal Remover plug-in, improvements for Generate effects.

Compilation:
	* Fixes when building Audacity with libraries disabled.
	* Improvements to make Mac and Solaris builds easier.

Security:
	* Full fix for issue CVE-2007-6061 on systems where temporary directories
	   can be changed by other users (thanks to Michael Schwendt).

Miscellaneous:
	* Updated translations for many locales.
	* Several stability improvements.


Changes in 1.3.4:

New Features
	* New Welcome Screen with introduction to Audacity
	* Enhanced Windows Shell integration, so Audacity shows up in lots of
		Windows places such as "Open With".
	* New keyboard command: 'Mix and Render to New Track'
		(bound to Ctrl+Shift+M).
	* New keyboard shortcut: "Shift-A" starts playback when stopped,
		or performs "Stop and Select" when playing.
	* Added support for VAMP audio analysis plug-ins.
	* Solo button solos only one track at a time, and a track cannot be both
		mute and solo.

Interface:
	* Keyboard shortcuts for making short/long jumps along the timeline.
	* Added 'Snap To' in the Selection Bar.
	* Made keyboard navigation easier when multiple menu items with the
		same first letter exist.
	* Enhanced interface for label editing.
	* Layout of OK/Cancel buttons consistency improved.
	* Preferences:
		* "Select all audio in project, if none selected" (on by default)
		* "Beep on completion of longer activities" (system bell, not
			main output).
		* Other preferences cleaned up and explanations improved.
	* Envelopes: Many fixes when copying / pasting / repeating.
	* Many translation updates.
	* Track height fixed in several cases.
	* CleanSpeech mode switching without closing and re-opening fixed.

Opening/saving formats:
	* Metadata editor added for OGG, FLAC and WAV/AIFF exports, and
		general improvements in this area.
	* Import of metadata improved.
	* Muted tracks are no longer audible in the exported mix.

Effects:
	* Truncate Silence: support for multiple and stereo tracks.
	* Dtmf Generator:
		* added support for keypad letters
		* added an amplitude control.
	* Compressor: variable decay time added.
	* Equalization:
		* Clicks at start / end prevented
		* Improvements to saved curves being found
		* Preview works correctly
	* 'Merge' command appears in Undo history.
	* Clipping detected more reliably.
	* Nyquist plug-ins reviewed and enhanced.
	* Better (and more) progress bars.
	* Cancelling effect always restores previous audio.
	* Several improvement to effects in batch mode.

Recording / Playback:
	* Improvements to latency correction.
	* Updated version of portaudio-v19 library.

Note that Help is no longer built in, but accessible on the Web via links
in Audacity.


Changes in 1.3.3:

Opening/saving formats:
   * Import
      * Import of audio from QuickTime (mov, aac, m4a) files is now
        supported on OS X.
      * Broadcast Wave Format (BWF) wave files can now be imported.
   * Export
      * Metadata can be added to OGG files
      * Improved export option selection
      * Additional export options added to MP3 and FLAC file formats
      * Command line exporter now supported on Windows and OS X

Effects:
   * EQ effect
      * Responsiveness improved.
      * Several enhancements added.
      * Batch support added.
   * New Auto Duck effect
   * Added previewing to AudioUnit effects
   * Much improved Noise Removal effect
   * Effects previewing can now be canceled
   * New DTMF Tone Generator effect
   * Additional options available in Noise effect
   * Improved the Tone Generation effects

Other features:
   * Major speed improvement in Spectrogram rendering
   * Increased support for drag and drop on OS X
   * Support added for building against wxWidgets 2.8
   * Support opening multiple Audacity Projects at once from Explorer on
     Windows
   * Improved main window sliders
   * New support for snapping while selecting and sliding
   * Improved track focus handling and visual feedback
   * Speed improvements and handling of resizing/zooming in tracks
   * Spectrum view can now be zoomed.
   * New internal file cache to improve handling of project files over
     networks

Also:
   * Many improvements to language specific translations
   * Numerous stability improvements


Changes in 1.3.1 and 1.3.2:

o Improved accessibility for the visually impaired
      + Improvements for screen readers, accessibility of
        tracks, and hot keys
o Usability improvements
      + New selection bar
      + New features for label tracks
      + Improved toolbar docking flexibility
      + Menu renaming and reorganization
      + Selection, ruler, and playback control improvements
o Auto-save and automatic crash recovery
o Many bug fixes and stability improvements
o Major improvements to some built-in effects (Repair, Equalization)
  and fixes to others
o New features and bug fixes for Nyquist
o Restructured Preferences dialog
o Improved batch processing
o File format export improvements
o Timer recording
o Intel Mac support


Changes in 1.3.0:

   New features
   The new features in Audacity 1.3 have been grouped into the
   following six major categories.

   1. Collapse/Expand Tracks
      In Audacity 1.3, every track has an upward-pointing triangle at
      the bottom of the label area on the left side of the track.

   2. Multiple clips per track
      In Audacity 1.2, there is one audio 'clip' per track. There is
      no easy way to time-shift part of a track without moving the
      rest. In Audacity 1.3, you can split a single track into multiple
      clips. You can move these clips around between different tracks,
      making it easy to construct complex compositions out of hundreds
      of smaller audio samples.

   3. Selection Bar
      In Audacity 1.2, the current selection is contained in a
      status bar at the bottom of the window. In Audacity 1.3,
      this is replaced by a fully functional Selection Bar, which
      displays and controls the current selection (your choice of
      Start and End, or Start and Length), and the current audio
      position. The selection bar is fully editable - just click
      in any field and type to change the current selection precisely.
      In addition, many formatting options allow you to view times in
      different units, such as samples, CD frames, or NTSC video frames.

   4. Improved Label Tracks
      Label Tracks are Audacity's way for you to create markings 3
      and annotations within your project. In Audacity 1.3, Label
      Tracks are much improved, with support for overlapping labels,
      and support for modifying both the left and right edge of the
      label region just by clicking and dragging.

   5. QuickTime and Audio Units on Mac OS X
      On Mac OS X, Audacity can now import and audio file supported
      by Apple's QuickTime technology. This includes .MOV and .MP4
      (AAC) files. Unfortunately encrypted audio files (such as
      those from the iTunes Music Store) cannot be imported directly
      into Audacity - Apple does not allow this to be done easily
      because it would be too easy to circumvent the encryption this way.

      Also on Mac OS X, Audacity now supports Audio Unit plug-ins.
      Audacity searches for Audio Units in the usual location, in the
      system or user's Library folder.

   6. Other features
      Better performance with large projects

      Project integrity check on open

      Transcription toolbar

      Upload

      Batch

      Cut lines

      CleanSpeech


Changes in 1.2.4:

  * The File menu now includes a list of recent files.

  * The "Generate Silence" effect now prompts for a length.

  * Audacity is now built with Vorbis 1.1, which features better encoding
    quality and file compression.

  * Dragging sound files into the Audacity window now works on Mac OS X
    and Linux, as well as Windows.  (Before, it worked only on Windows.)

  * The "View History" window can now discard old undo levels to save disk
    space on Windows.  (This previously worked only on Linux and Mac.)

  * "Preferences" command is now in Edit menu.

  * "Plot Spectrum" command is now in Analyze menu.

  * Opening a project file saved by a later version of Audacity displays
    an intelligent error message.  Also, trying to import a project file
    (instead of open it) displays an intelligent error message.

  * Audacity now compiles in Visual C++ .NET 2003.

  * Other minor bug fixes.

  * New or updated translations: Arabic (ar), Czech (cs), Finnish (fi),
    Hungarian (hu), Japanese (ja), Norwegian (nb), Slovenian (sl),
    Simplified Chinese (zh_CN), Traditional Chinese (zh_TW).


Changes in 1.2.3:

  * Fixed a bug that caused recording to stop or display incorrectly
    after about 50 minutes on some Windows systems.  (This was partly
    fixed in Audacity 1.2.2, but still didn't work on some systems.)

  * The Change Pitch and Change Tempo effects have been upgraded to
    use a new version of the SoundTouch library by Olli Parviainen,
    with better speed and higher quality.

  * libsndfile has been upgraded to version 1.0.11.

  * Fixed a bug that caused the program to run slowly when using the
    Envelope tool.

  * Shift-clicking on a mute or solo button now un-mutes (or un-solos)
    all other tracks.

  * Nyquist plug-ins can now accept strings as input.  Also, a "Debug"
    button has been added to Nyquist effect dialogs, which allows you
    to see all of the output produced by Nyquist, for aid in debugging.

  * When the audio file referenced ("aliased") by an Audacity project is
    missing, Audacity will now always play silence.  Before, Audacity
    would sometimes repeat the most recent audio that was played previously.

  * VU Meters will now always reset when audio I/O has stopped.

  * Fixed a major Mac-only bug that was causing Audacity to crash at seemingly
    random times, but especially during audio playback and recording.

  * New or updated translations: Italian (it), Hungarian (hu),
    Ukrainian (uk), Spanish (es). Polish (pl), Simplified Chinese (zh),
    Norsk-Bokmal (nb), French (fr).


Changes in 1.2.2:

  * VU Meters added for both playback and recording.  Click on
    the recording meter to monitor the input without recording.

  * Export Multiple - new feature that lets you export multiple
    files at once, either by track, or split based on labels.

  * Attempt to automatically correct latency in full-duplex recordings.
    (This does not work perfectly, and is not yet supported on all
    systems.  It will improve in future versions.)

  * Fixed a serious bug that could cause data loss when you save and
    then reload and re-edit an Audacity project containing repeated
    or duplicate data.

  * MP3 tags dialog will only pop up the first time you export as
    MP3; after that it will not pop up again as long as you have
    filled in at least one tag.

  * You can now add a label at the current playback position - in
    the Project menu, with a shortcut of Ctrl+M.

  * Clicking on a label now selects all of the tracks, making it
    easier to use the label track to recall selections.

  * Windows: Fixed a crash in the Time Track "Set Rate" command.

  * Fixed a bug that caused problems with recordings over 45 minutes
    on some Windows systems.

  * Mac OS X: Improved support for the Griffin iMic by fixing a bug
    that was causing it to always record in mono instead of stereo.

  * Added support for Software Playthrough (listen to what you're
    recording while recording it, or while monitoring using a VU
    meter) - this makes it possible, for example, to record using one
    audio device while listening to it play through a separate device.

  * Unix/Linux: Fixed freeze caused by captured mouse when audio
    device hangs.  (Audacity may not respond, but it will no longer
    freeze all of X.)

  * Fixed a cosmetic bug that caused improper waveform display if
    you tried to open an Audacity project saved on a different
    platform (e.g., copying a project from a Mac to a PC).

  * Fixed bug that could cause instability when pasting, splitting,
    or duplicating a label track.

  * You can now change the font of a label track by choosing "Font..."
    from the label track's pop-up menu.

  * Basic printing support has been added.  Currently it scales the
    entire project to fit exactly on one page.  Try printing in
    landscape orientation for best results.

  * Mac OS X and Windows: Audacity ships with a newer version (1.0.1)
    of the Ogg Vorbis encoder.  Vorbis compression will now have higher
    quality and smaller file sizes.

  * Fix a bug that occasionally caused crashes when applying effects
    to split tracks.

  * Zoom In / Zoom Out now properly disable when they're not available.

  * Fixed disk memory leak in Preview

  * Other minor bug fixes and performance improvements.


Changes in 1.2.1:

  * The following translations have been added or updated:  Finnish,
    French, Hungarian, Italian, Japanese, Norwegian, Polish, Russian.

  * Fix a bug that could cause data to be lost when pasting audio
    from one project into another, after the first project has been
    saved and closed.

  * Fix a possible crash when opening or resizing the Equalization
    window, especially when using large system fonts.

  * Don't allow percentages less than -100% in Change Pitch/Speed/Tempo
    effects (fixes a possible crash).

  * Fix a crash when the temporary directory is not available on startup.

  * Correctly load ID3 tags saved in Audacity project files.

  * On Linux and OS X, store lockfiles in the temp directory instead of
    the user's home directory.  This fixes problems in lab environments
    where users have restricted or network-mounted home directories.

  * Fix a bug that prevented Nyquist effects from running when certain
    regional settings were activated.

  * Fix a bug in the Quick Mix command that could cause old temporary
    files to not be deleted.

  * Linux: Fix endianness problems in playback on PowerPC.

  * Linux: Fix compilation problem in Nyquist on MIPS.

  * Linux: Include a more recent PortAudio v19 snapshot (fixes compilation
    problems when building with the --with-portaudio=v19 option).

  * Two new Nyquist plug-ins: "Cross Fade In" and "Cross Fade Out."

  * Other minor bug-fixes.


Changes in 1.2.0:

  * New cross-fade effects.

  * Fix problem where samples were drawn in the wrong position
    when zoomed all the way in.  This caused the drawing tool
    to move a different sample than the one under the cursor.

  * Don't use id3v2.4 tags, which are not yet supported by
    most players.  (This was fixed in 1.2.0-pre2, but appeared
    again by accident in 1.2.0-pre3.)

  * Correctly display translated messages in the status bar.

  * When the cursor is on-screen, the Zoom In button now zooms
    to the area around the cursor.

  * Mac OS X: Fixed audio problems on the Apple PowerMac G5.

  * Linux/ALSA: Work around a bug in ALSA's OSS emulation that
    caused Audacity's playback cursor to move too quickly.

  * Microsoft Windows: The Audacity source code should now
    compile out of the box on Windows.

  * Many new/updated translations.


Changes in 1.2.0-pre4:

  * Fixed problems that could occur when importing certain
    non-seekable PCM audio files, such as GSM610.

  * Fixed bug that was causing the samples to shift off-screen
    horizonally when zoomed in very far and the track had a
    time-shift offset.

  * Fixed bugs in the new resampler that added noise to resampled
    audio on some systems. If you experienced noise when exporting
    to a WAV, MP3 or OGG file you may have been bitten by this bug.

  * Fixed bug that led to occasional crashes when using the
    time-shift tool in conjunction with high zoom factors.

  * Dithering is now only applied on export when it is really
    necessary (e.g. when converting float samples to 16-bit).

  * Files that only contain mono tracks are now automatically
    exported to stereo files when they contain tracks which are
    panned to the left or the right.

  * The Delete key can now be used to delete the current selection,
    in addition to the Backspace key.

  * Fixed bug where Audacity didn't ask whether to save
    changes if you close the project or exit while recording.

  * Mac OS X: Supports Playthrough (listen to what you're recording
    while recording it) if your hardware device supports it.

  * Mac OS X: Audacity is now a package (you can right-click on
    Audacity.app and select 'Show Package Contents').  Launch time
    has improved significantly.

  * MS Windows: Fixed problem that caused Windows XP to use
    the short name of a file ("TESTFI~1.AUP"), which led to
    problems when the file was later opened again using the
    long file name.

  * MS Windows: Fixed bug that caused file exports to fail
    if the destination directory was the root folder of a
    Windows drive.

  * MS Windows: Audacity's application information which
    is written to the Windows registry now always contains
    the full path to the executable.

  * MS Windows: Fixed problems in trying to set the Windows
    registry as non-admin user, for file-type associations.

  * Make sure the "Save" command is enabled after changing
    gain and pan sliders.

  * Updated translations.  Added translator credits to the
    "About" window in localized versions.


Changes in 1.2.0-pre3:

  * Fixed bug where Export is grayed out when nothing is
    selected.

  * Fixed crash caused by opening Audacity on a computer with
    a high-end sound card with no mixer support.

  * Fixed crash in Import Raw.

  * Fixed New Stereo Track.

  * Cosmetic fixes for Mac OS X.

  * Support for the VST Enabler on Windows added.

  * Fixed crash if you close Audacity while the Preferences
    dialog is open.

  * Fixed duplicate-character bug in Mac OS X Label Tracks.

  * The recording level control on Linux now adjusts the IGAIN,
    rather than the playthrough level of the recording source.

  * Fixed bug that caused corruption to 16-bit stereo recordings.

  * Fixed bug that caused data loss if you deleted all tracks in
    a saved project and then open a new file into the same window.

  * Added support for alternate audio button order (in Interface
    preferences)

  * Added preliminary support for wxX11

  * Added fully transparent Windows XP icon

  * Fixed crash if you try to record (or play) and no audio
    devices exist, or if the audio device doesn't support the
    mode you selected.

  * Audacity no longer sets the process priority to high while
    recording on Windows.  Users can still do this manually
    using the Task Manager.

  * Fixed bug that caused last ~100 ms of the selection to get
    cut off on Windows.

  * Fixed FFT Filter and Equalization effects dialogs.

  * Fixed bugs in Unix build system (DESTDIR in locale directory,
    choosing libsamplerate instead of libresample)

  * Support for LADSPA plug-ins on Windows added, and
    three open source LADSPA plug-ins ported to Windows
    (GVerb reverb, SC4 compressor, and Hard Limiter)


Changes in 1.2.0-pre2:

  * Online help completed.  The full manual is nearly complete
    and will be posted to the website for online browsing shortly.

  * Audacity will no longer let you do unsafe editing operations
    while playing or recording.  This eliminates many potential
    crashes.

  * Fixed ability to cancel Quit button.

  * New resampling library, with no restrictions on the maximum or
    minimum rate of resampling.

  * Audacity now supports LADSPA plug-ins on all platforms, and
    supports VST plug-ins through an optional LADSPA plug-in
    called the "VST Enabler", which you can download separately.
    Because of licensing issues, Audacity cannot be distributed
    with VST support built-in.

  * Mac OS X keyboard shortcut problems have been fixed.

  * Mac OS X audio muting problems have been fixed.

  * Mac OS X playback/recording cursor sync problems have been fixed.

  * Silence now displays a straight line again, instead of nothing.

  * Added a vertical ruler to the Waveform dB display.

  * Fixed crash in Change Pitch.

  * You can now Paste if nothing is selected.

  * Canceling an Import operation doesn't cause an extra error
    dialog to appear.

  * Audacity now handles filenames with international characters
    correctly.

  * Now outputs ID3v2.3 tags (instead of ID3v2.4), to be
    compatible with more MP3 players.

  * Minor improvements to build system on Unix systems.


New features in Audacity 1.2:
  * User Interface
    - Vertical zooming of tracks.
    - Improved look and placement of toolbars.
    - New custom mouse cursors.
    - Complete implementation of editable keyboard shortcuts.
    - Find zero-crossings.
    - Mouse wheel can be used to zoom in and out.
    - Multi-Tool mode.
    - Amplify using envelope.
    - Labels can store selections (like Audacity 1.0.0).

  * Effects
    - Repeat Last Effect command
    - Improved VST plug-in support
    - Most effects now have a Preview button
    - Compressor (Dynamic Range Compressor)
    - Change Pitch (without changing tempo)
    - Change Tempo (without changing pitch)
    - Change Speed (changing both pitch and tempo)
    - Repeat (useful for creating loops)
    - Normalize (adjust volume and DC bias)

  * Audio I/O
    - 1-second preview command.
    - Looped play.

  * File I/O
    - Audacity 1.2.0 opens project files from all previous versions
      of Audacity from 0.98 through 1.1.3.
    - Open multiple files from the same dialog.
    - Use a text file to specify a list of audio files to open with offsets.

  * Updated user manual

  * Bug fixes
    - Project files with special characters are no longer invalid.
    - "Scratchy" noises caused by bad clipping are fixed.
    - Audacity no longer exports invalid Ogg files, and does not cut off the
      last few seconds of exported Ogg files.
    - Mono MP3 files now export at the correct speed.
    - Many incorrect results from the Envelope tool have been fixed.
    - The "Export Labels" command now overwrites existing files correctly.
    - The "Plot Spectrum" window displays the correct octave numbers for
      notes.
    - Several memory leaks are fixed.


New features in Audacity 1.1.3:
  * User Interface
    - New Mixer toolbar allows you to control the output
      volume, input volume, and input source directly
      from Audacity.
    - Every track now has its own gain and pan controls.

  * File I/O
    - Uses improved project file format.  (Unfortunately reading
      previous formats, including 1.1.1, is not supported.)
    - Block files (stored in Audacity project directories) now
      use the standard AU format.  Though some Audacity
      meta-information is in these files, they can now be
      read by many other popular audio programs as well.
    - Fixed some bugs relating to reading/writing audio
      files with more than 16 bits per sample.
    - Import RAW is functional again, with a simpler GUI
      but support for far more file formats.  The
      autodetection algorithms are much more accurate than
      in 1.0.

  * Audio I/O
    - Completely rewritten audio I/O, with lower latency
      and minimal chance of buffer underruns while
      recording.

  * Resampling
    - Using high quality resampling algorithms, with the
      option of better quality for mixing than for real-time
      playback

    - Preliminary support for Time Tracks, for changing
      playback speed over time.

  * Many more bug fixes and new features


New features in Audacity 1.1.2:
  * User Interface
    - Fixed bug in Windows version, for track menu commands
	  "Name..." and "Split Stereo Track"/"Make Stereo Track".
  * Effects
    - Nyquist support on Windows (supports plug-ins written
	  in Nyquist, an interpreted functional language based
	  on Lisp).


New features in Audacity 1.1.1:

  * User Interface
    - Tooltips appear in Statusbar.
    - Vertical cursor follows play/record
    - Pause button
    - Drawing tool (with three different modes)
    - Vertical Resizing of stereo tracks is more fun.
    - Adjust selection by click-dragging selection boundary
    - Toolbar button context-sensitive enabling/disabling
    - Better zooming functionality (centers region)
    - Multiple ways to display the cursor position and selection
    - Snap-to selection mode
    - Drag tracks up and down
    - Align and group align functions
    - Cursor save/restore
    - Working history window
  * Effects
    - Effects broken down into three menus: Generate, Effect, and
      Analyze
    - Generate menu lets you generate silence, noise, or a tone
    - Nyquist support (supports plug-ins written in Nyquist,
      an interpreted functional language based on Lisp)
  * Localization
    - Improved localization support
    - More languages available
    - Language selection dialog on startup
  * Mac OS X
    - Support for more audio hardware
    - Support for full-duplex (play while recording)
    - Support for MP3 exporting using LameLib Carbon
  * Unix
    - Audacity now has a man page (it describes command-line
      options and how to set the search path)
  * File Formats
    - Uses libsndfile 1.0, which fixes some bugs and
      improves performance
  * Searching for Files:
    - On Windows and Mac OS, Audacity now looks for
      translations in the "Languages" folder and all plug-ins
      in the "Plug-ins" folder, relative to the program.
    - On Unix, Audacity looks for translations in
      <prefix>/share/locale and looks for everything else
      in <prefix>/share/audacity and also in any paths in
      the AUDACITY_PATH environment variable


New features in Audacity 1.1.0:

  * Core audio processing:
    - Support for 24-bit and 32-bit sample formats
    - Automatic real-time resampling (using linear
        interpolation)
  * Effects:
    - Support LADSPA plugins on Linux / Unix
  * File formats:
    - New XML-based Audacity project format
    - Full Ogg Vorbis support now (importing and exporting)
    - Export to any command-line programs on Unix
    - Support for reading and writing many more types of
        uncompressed audio files, including ADPCM WAV files.
  * Toolbars
    - New toolbar drawing code; automatically adopts your
        operating system's colors
    - New toolbar buttons (Skip to Start, Skip to End)
    - New Edit toolbar
    - Toolbar buttons disable when they're not available
  * User Interface
    - Fully customizable keyboard commands
    - Autoscroll while playing or recording
    - New Ruler, used in main view and in
        FFT Filter effect
    - The waveform now displays the average value in a lighter
        color inside the peak values
  * Localization
    - Audacity can now be localized to different foreign
      languages.


New libraries in Audacity 1.1:

  * libmad for fast MP3 importing
  * libid3tag for editing MP3 file information
  * libsndfile to read and write more audio file formats
  * PortAudio for cross-platform audio playing and recording