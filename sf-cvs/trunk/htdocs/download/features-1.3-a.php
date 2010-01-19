<?php
/*
 * Copyright 2005-2009 onwards Dominic Mazzoni, Gale Andrews
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/3.0/
 */
  require_once "main.inc.php";
  $pageId = "features-1.3-a";
  $pageTitle = _("New features in Audacity 1.3");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<h3><?=_('Beta version')?></h3>
<p>
<?=_('Audacity 1.3 Beta is our new, not quite finished, development version which will be the basis of our next "stable" version.')?></p>

<p><?=_('
 <ul>
  <li>Contains dozens of new, exciting features. Very occasionally, these might need final polishing or not be retained in later versions.</li>
  <li>Occasionally, a feature might not work as it did before, or might be temporarily disabled.</li>
  <li>Some parts of the program are not yet documented or translated into different languages.</li>
 </ul>
')?></p>

<h3 id="details"><?=_("Changes in Audacity 1.3.11")?></h3>

<?=_("
<dl>
   <dt> Bug fixes for: </dt>
	       <dd> Imports and Exports: 
                 <ul>
                       <li> Bug when exporting partial selections caused
                             too much audio to be exported is fixed</li>
                       <li> Fix corrupt files exported through FFmpeg when 
                             metadata included (metadata is now exported 
                             correctly in M4A files)</li>
                       <li> Prevent saving a new Audacity Project over an 
                             existing one as this could corrupt both projects</li>
                       <li> Improved help for files that cannot be imported 
                             because the relevant optional library is missing</li>
                 </ul>       
          </dd>

	       <dd> Effects: 
                 <ul>
                       <li> Allow effects which change the length of the audio 
                             they work on to also be applied to selected label
                             tracks, thus keeping them synchronized</li>
                       <li> Fixed inability in Nyquist plug-ins to add labels 
                             to an existing label track</li>
                       <li> <i>(Mac)</i>&nbsp; Equalization window was corrupted 
                             after Preview</li>
                       <li> <i>(Linux 64-bit)</i>&nbsp; Fixed crash Generating 
                              Click Track</li>
                 </ul>
          </dd>

	       <dd> Audio Devices: 
                 <ul>
                       <li> Fixed bug causing recording to stop short when the 
                             recording sample rate is not supported by the sound
                             device and libsamplerate is used for resampling</li>
                       <li> Fix crash when opening Preferences on a machine where 
                             there are no available audio devices</li>
                       <li> Fixes for bugs using Timer Record and Sound Activated 
                              Recording</li>
                 </ul>
          </dd>

	       <dd> User Interface: 
                 <ul> 
                       <li> Sizes of some dialogs adjusted to ensure they fit on
                             the screen</li>
                       <li> Fix for supposedly \"hidden\" items appearing on screen
                             with large monitors</li>
                       <li> Various keyboard shortcut and translation fixes</li>
                 </ul>
          </dd>

	       <dd> Other bug fixes: 
                 <ul>
                       <li> Several timing-dependent crashes and minor incorrect 
                             behaviours have been fixed</li>
                       <li> Windows installer now installs correctly over previous
                             versions of Audacity</li>
                 </ul>
          </dd>
")?>

<?=_("
   <dt> Changes and improvements:</dt>
           <dd>
                 <ul>
                       <li> <i>(Windows)</i>&nbsp; Better icon file with higher 
                             resolution and transparency</li>
                       <li> New SoundFinder plug-in to label regions of audio
                             between silences, so allowing silences between tracks
                             to be excluded when exporting multiple</li>
                 </ul>
          </dd>
</dl>
<p>
Please be aware of 
<a href=\"http://wiki.audacityteam.org/index.php?title=Release_Notes_1.3.11#Known_Issues_at_Release\">
Known Issues at Release</a> (also viewable in the included README.txt).
</p>
")?>

<h3><?=_("Changes in Audacity 1.3.10")?></h3>

<?=_("
<dl>
   <dt> Bug fixes for: </dt>
	       <dd> Imports and Exports: 
                 <ul>
                       <li> Freeze importing audio files when Default View Mode
                             set to Pitch (EAC)</li>
                       <li> Simultaneous On-Demand imports sorted incorrectly</li>
                       <li> WAV or AIFF files imported as noise if Preferences set to 
                             copy in the data at 24-bit quality</li>
                       <li> WMA exports corrupted if they contained metadata</li> 
                       <li> Metadata Editor appeared before the Export window when 
                             exporting to any format </li>
                 </ul>       
           </dd>

           <dd> Effects:
                 <ul>  
                       <li> Crash or hang using Equalization on longer tracks</li>
                       <li> Reverse did not reverse clip boundaries</li>
                       <li> Nyquist:</li>
                             <ul> 
                                   <li> Excessive memory consumption led to slow 
                                         processing or crashes</li>
                                   <li> Values appearing in text boxes not always 
                                         the default or previously entered values</li>
                                   <li> Errors running in European locales where 
                                         comma used as decimal separator</li>
                              </ul>
                       <li> VST effects remained in Effect menu even when re-scanned and 
                             no longer available </li> 
                       <li> Truncate Silence produced incorrect results if silences 
                             spanned a block boundary </li>    

                 </ul>
           </dd>

           <dd> Other bug fixes: 
                 <ul> 
                       <li> Spurious \"not writable/disk full\" errors when saving projects </li>
                       <li> Playing, rendering or exporting multiple tracks led to 
                             desynchronized playback or loss of audio data </li>
                       <li> Crash opening Preferences when no recording and/or playback
                             devices enabled or connected </li>
                       <li> Preferences window: OK button did not respond to ENTER when
                             a tab selected in left-hand panel </li>
                       <li> Mixer Board solo button handling</li> 
                       <li> <i>(Windows)</i> After a period launching correctly, Audacity
                              sometimes did not come up on top at launch </li>
                       <li> <i>(Mac OS X)</i> Correctly installed Help folder could not
                              be found</li>  
                       <li> <i>(Mac OS X and Linux)</i> Output slider could affect VU 
                              playback meter which then did not reflect actual waveform
                              volume level</li>
                       <li> <i>(Linux)</i> Undoing or redoing a label edit could cause 
                              a crash</li>
                 </ul>
           </dd>
")?>

<?=_("
   <dt> Changes and improvements:</dt>
           <dd>
                 <ul>
                       <li> Linked audio and label tracks disabled until a future Beta
                             version so they can be bug fixed</li>
                       <li> Input volume slider will be disabled if it doesn't have proper
                             control of system slider; use the system slider instead</li>
                       <li> Proper support for copying/pasting label-with-audio including 
                             label text; new Edit > Paste Text to New Label menu item to
                             paste system clipboard </li>    
                       <li> Contrast Tool now modeless, more intuitive handling of multiple
                             project windows, various other minor improvements </li>
                 </ul>
           </dd>
</dl>
<p>
Please be aware of 
<a href=\"http://wiki.audacityteam.org/index.php?title=Release_Notes_1.3.10#Known_Issues_at_Release\">
Known Issues at Release</a> (also viewable in the included README.txt).
</p>
")?>

<h3><?=_("New in Audacity 1.3.9")?></h3>

<?=_("
<dl>
   <dt> Bug fixes for: </dt>
      <dd>   
            <ul>
                   <li>Crash, slow launch or excessive CPU/memory use 
                        arising from automatic VST support:</li>
                        <ul>
                           <li>VST instrument plug-ins should now be correctly ignored</li>
                           <li>VST effects now scanned only at start of first session
                                that detects them, then cached</li>
                           <li>Effects are now not loaded or opened until needed</li>
                           <li>New \"Effects\" tab in Preferences to enable/disable 
                                VST effects and enable VST rescan on next launch</li>
                        </ul>
                    <li>Default View Mode now works</li>
                    <li>Chains now always apply their stored parameters rather than 
                         those last used in Effect menu</li>
                    <li>Non-MP3 files imported via drag or Recent Files caused
                         crash if filter in file open window set to MP3</li>
                    <li>AAC exports (using the optional 
                         <a href=\"http://manual.audacityteam.org/index.php?title=FAQ:Installation_and_Plug-Ins#installffmpeg\">
                         FFmpeg library</a>) silenced</li>
                    <li>Generating audio always fitted the project in the window; 
                         fit now done only if generating in new track</li>  
                    <li>View menu items/shortcuts incorrectly disabled when playing
                         or recording</li>
                    <li>DTMF generator defaulted to zero duration on open</li>   	
                    <li>Unwanted interactions between linked audio and label tracks</li>
                    <li><i>(Windows XP)</i>&nbsp; Failure to launch on some machines due to
                             \"incorrect configuration\" issue</li>
                    <li><i>(Windows)</i>&nbsp; Crash importing a stereo file while a screen reader
                             such as JAWS is running</li>
                    <li><i>(Mac OS X)</i>&nbsp;:</li>
                        <ul>
                           <li>Audio Units effects applied to all tracks in project even
                                if not selected</li>
                           <li>QuickTime importer now handles files greater than 16-bit 
                                or 64000 Hz</li>
                        </ul>
                    <li>Various other interface bugs</li>
            </ul>  
      </dd> 
       
   <dt>Improvements:</dt>
      <dd>   
            <ul>
                    <li>Compressor: new option to compress based on peaks, improved
                        attack and decay time support</li>
                    <li>Mixer Board: improved design, more responsive meters and 
                        now interacts fully with Track Panel in main window</li>
            </ul>
      </dd>
</dl>

<p>
Please be aware of 
<a href=\"http://wiki.audacityteam.org/index.php?title=Release_Notes_1.3.9#Known_Issues_at_Release\">
Known Issues at Release</a> (also viewable in the included README.txt).
</p>
")?>


<h3><?=_("New in Audacity 1.3.8")?></h3>

<?=_("
<dl>
   <dt> New Features </dt>
	       <dd> Effects and Analysis:
                 <ul>
                        <li>VST Effects now display GUI by default</li> 
                        <li>Faster Equalization and Noise Removal; improved 
                            Truncate Silence and Click Track</li>
                        <li>Chains applied to files now clear temporary data after 
                            processing each file</li> 
                        <li>Updated Nyquist implementation with support for SAL 
                            syntax and improved memory management</li> 
                        <li>Plot Spectrum now analyzes up to 237.8 seconds of audio,
                            with separate windows for each project and improved 
                            display; new preferences for Spectrograms</li> 
                        <li>Contrast Analysis tool now modeless for easier use</li>
                 </ul>
          </dd>

          <dd> Interface:
                 <ul>  
                        <li>Draft Manual/Quick Help included in Windows and Mac 
                            installers</li> 
                        <li>New \"Mixer Board\" view with per-track VU meters</li> 
                        <li>Mute, solo, gain, pan and track height saved in projects</li>
                        <li>More compact Preferences window with easier-to-use Keyboard
                            tab and new toolbars shortcuts</li>   
                        <li>New Screenshot Tools and improved screen reader support</li>  
                 </ul>
          </dd>

          <dd> Other:
                 <ul>
                        <li>Record more than 16 channels (hardware/drivers permitting)</li>
                        <li>Improved support for non-mmap ALSA devices such as PulseAudio</li> 
                        <li>32-bit float data over 0 dB now handled without clipping</li>
                        <li>\"Stop\" option when importing preserves already imported data</li>
                        <li>AMR NB import and export now supported on all platforms if the optional 
                            <a href=\"http://manual.audacityteam.org/index.php?title=FAQ:Installation_and_Plug-Ins#installffmpeg\">
                            FFmpeg library</a> is installed</li>
                        <li>Faster waveform drawing and better response in multi-track 
                            projects</li> 
                 </ul> 
          </dd>

   <dt> Bug fixes for: </dt>
          <dd>   
                 <ul>
                        <li>Export Multiple: failed if empty label encountered; files silenced 
                             if overwriting imported WAV files without copying them in</li>   
                        <li>Metadata Editor hidden if it was on a now unavailable second
                            monitor</li>
                        <li>Misaligned audio after \"Split New\" or Noise Removal effect</li>
                        <li>Incorrect label movement and paste with linked audio and label
                            tracks</li>      
                        <li>Equalization, Cut Preview and Advanced Mixing Options dialogue</li>
                        <li><i>(Linux)</i> Mixer Toolbar should now adjust levels and select
                            input sources properly</li>  
                        <li>\"Audio cache\" preference caused crashes - data is now only
                            cached in memory if available RAM is above a level defined
                            in preferences</li> 
                        <li>Various other crashes</li>
                 </ul>
          </dd>

</dl>

<p>
Please be aware of 
<a href=\"http://wiki.audacityteam.org/index.php?title=Release_Notes_1.3.8#Known_Issues_at_Release\">
Known Issues at Release</a> (also viewable in the included README.txt).
</p>
")?>


<h3><?=_("New in Audacity 1.3.7")?></h3>
<?php
   // i18n-hint: The following are new features in
   // old versions of Audacity, which have never been
   // translated up to now. Feel free to translate 
   // them as a low priority.  
echo _('<dl>
	<dt> Improvements and new features </dt>
	      <dd>   <ul>   
                      <li>F11 Full Screen mode</li>
                      <li>High-quality "Sliding Time Scale/Pitch Shift" effect</li>
                      <li>"Audio Contrast Analyzer" for testing audio on the
                           internet for
                           <a href="http://www.eramp.com/WCAG_2_audio_contrast_tool_help.htm">
                           WCAG2 accessibility compliance</a></li>
                      <li>On Windows, sound devices can now be opened using the more
                           efficient DirectSound API</li>
                      <li>As a first step towards improving latency correction, a fixed
                           rather than variable correction is now applied, customizable
                           in Preferences                    
                      <li>Numerous minor interface improvements such as Metadata
                           Editor navigation, new "hh:mm:ss + hundredths" selection format
                </ul>
         </dd>

	<dt> Numerous usability and stability fixes </dt>
	      <dd> Cross-platform Bug Fixes:
                <ul>
                     <li>Muting/soloing caused incorrect channel results in exported
                          stereo files</li>
                     <li>Nyquist effects: pasted unmodified audio into the result,
                          no progress bar, truncation of processed audio on
                          cancel</li> 
                     <li>Noise Removal: pasted unmodified audio into the result, 
                          unwanted filtering tail</li>
                     <li>Uncompressed exports:</li>
                            <ul>
                                  <li>Export as WAV could be corrupted if
                                       overwriting to same file</li>
                                  <li>Export multiple to uncompressed
                                       formats only produced 16-bit WAV</li>
                            </ul> 
                     <li>Compressed exports:</li>
                            <ul>
                                  <li>MP3 exports now produce correct bit rate
                                       mode, quality and length, with improved 
                                       metadata visibility in player applications 
                                       (Windows users will require the
                                       <a href="http://lame.buanzo.com.ar/">
                                       latest version of the LAME encoder</a>) 
                                  <li>WMA exports containing metadata are now correct</li> 
                            </ul>
                     <li>Restored support for multi-channel recording devices that
                          can record more than two channels using Audacity</li>
                </ul>
         </dd>');?>

<?php
   // i18n-hint: The following are new features in
   // old versions of Audacity, which have never been
   // translated up to now. Feel free to translate 
   // them as a low priority.  
echo _('<dd>Platform-specific Bug Fixes:
                <ul>
                     <li><i>Windows Vista:</i> fixed crash opening Preferences with no sound
                          devices enabled and connected </li>
                     <li><i>Mac OS X and Linux:</i></li>
                         <ul>      
                                <li>fixes for spurious clipping, label typing, no shortcuts after
                                     running effects</li>                           
                                <li>project rate now always changes to respect that of first 
                                   imported file</li>
                         </ul>
                     <li><i>Mac OS X only:</i></li>
                         <ul>
                                <li>fixes for inactive or corrupted menus, hidden dialogues, 
                                     portable settings not detected, and inability to set 
                                     independent Command and Control shortcuts</li>
                                <li>FFmpeg installer now available</li>
                         </ul> 
                </ul>
         </dd>    
   </dt>    
</dl>

<p>
Please see the included README.txt for full details of changes, and for issues known at time of Release.
</p>');?>

<h3><?=_("New in Audacity 1.3.6")?></h3>
<?php
   // i18n-hint: The following are new features in
   // old versions of Audacity, which have never been
   // translated up to now. Feel free to translate 
   // them as a low priority.  
echo _('<dl>
	<dt> Major new capabilities </dt>
         <dd>   <ul>
                      <li><b>FFmpeg support</b> (downloadable separately) permits import and
                             export of a much wider range of file formats, including WMA, 
                             M4A and AC3, plus import of audio from video files</li>
                      <li><b>On-demand loading</b> of uncompressed files eliminates the 
                             wait before files can be played or edited</li> 
                      <li><b>Linked audio and label tracks</b> allow labels to move with
                             their corresponding audio when cutting, pasting or changing
                             speed or tempo</li>
         </ul>   </dd>
   </dt>

   <dt> Other features </dt>
         <dd>   <ul>
                       <li>Hierarchical plug-in grouping for built-in plug-ins</li>
                       <li>Sound activated recording</li>
                       <li>Ability to save smaller, compressed project files</li>
                       <li>MIDI files can now be imported, cut-and-paste edited, then exported</li>
         </ul>   </dd>      
   </dt>

   <dt> Miscellaneous </dt>
         <dd>   <ul>
                       <li>Transport menu for easy access to frequently used
                           recording/playback commands and preferences</li>       
                       <li>Default View Mode Preference to choose Waveform, 
                           Spectrum or Pitch view</li>
                       <li>Several bug fixes</li>
         </ul>  </dd>         
   </dt>
</dl>');?>

<h3><?=_("New in Audacity 1.3.5")?></h3>
<?php
   // i18n-hint: The following are new features in
   // old versions of Audacity, which have never been
   // translated up to now. Feel free to translate 
   // them as a low priority.  
echo _('<dl>
	<dt> Recording / Playback </dt>
	      <dd>   <ul>
                           <li>Several bugs fixed so that latency correction should be better, 
                               and more devices work correctly.</li>
                           <li>Problems with invalid sample rates under Linux should be much rarer.</li>
                           <li>Newer version of Portaudio library.</li>
                           <li>New feature to record onto the end of an existing track
                               (hold Shift while clicking Record).</li>
             </dd>   </ul>
       </dt>

	<dt> Import / Export </dt>
	      <dd>   <ul>
                           <li>Updated versions of Libogg, Libvorbis, Libflac, Libsndfile and Twolame
                               libraries.
                           <li>Handling of unsupported file formats more informative.</li>
                           <li>Handling of file names with slashes on OS X improved.</li>
                           <li>New dialog allows replacement of illegal file name characters on all platforms.</li>
             </dd>   </ul>
       </dt>

	<dt> Interface </dt>
	      <dd>   <ul>
                           <li>Improved scaling and layout for rulers and VU meters.</li>
                           <li>Envelope fixes/improvements including full control of undo/redo.</li>
                           <li>New keyboard shortcuts and improved menu navigation.</li>
                           <li>Preferences:</li>
                               <ul>
                                    <li>More intuitive tab arrangement.</li> 
                                    <li>New options for mute/solo and Metadata Editor behavior.</li> 
                                    <li>Language can now be changed without restart.</li>
                               </ul>
			      <li>Expanded Build Information tab.</li>
           </dd>   </ul>
       </dt>

	<dt> Effects </dt>
	      <dd>   <ul>
                           <li>New Vocal Remover plug-in, improvements for Generate effects.</li>
             </dd>   </ul>
       </dt>

	<dt> Compilation </dt>
	      <dd>   <ul>
                           <li>Fixes when building Audacity with libraries disabled.</li>
                           <li>Improvements to make Mac and Solaris builds easier.</li>
             </dd>   </ul>
       </dt>

	<dt> Security </dt>
	      <dd>   <ul>
                           <li>Full fix for issue CVE-2007-6061 on systems where temporary directories
                                can be changed by other users (thanks to Michael Schwendt).</li>
             </dd>   </ul>
       </dt>


	<dt> Miscellaneous </dt>
	      <dd>   <ul>
                           <li>Updated translations for many locales.</li>
                           <li>Several stability improvements.</li> 
             </dd>   </ul>
       </dt>
</dl>');?>

<h3><?=_("New in Audacity 1.3.4")?></h3>
<?php
   // i18n-hint: The following are new features in
   // old versions of Audacity, which have never been
   // translated up to now. Feel free to translate 
   // them as a low priority.  
echo _('<dl>
  <dt></dt>
  <dd>
    <!-- indent cheat -->
    <dl>
      <dt> New features </dt>
      <dd>New Welcome Screen with introduction to Audacity.</dd>
      <dd>Enhanced Windows Shell integration, so Audacity shows up in lots of
          Windows places such as "Open With".</dd>
      <dd>New keyboard command: "Mix and Render to New Track" (bound to Ctrl+Shift+M).</dd>
      <dd>New keyboard shortcut: "Shift-A" starts playback when stopped,
          or performs "Stop and Select" when playing.</dd>
      <dd>Added support for VAMP audio analysis plug-ins.</dd>
      <dd>Solo button solos only one track at a time, and a track cannot be both mute and solo.</dd>

      <dt> Interface </dt>
      <dd>Keyboard shortcuts for making short/long jumps along the timeline.</dd>
      <dd>Added "Snap To" in the Selection Bar.</dd>
      <dd>Made keyboard navigation easier when multiple menu items with the same first letter exist.</dd>
      <dd>Enhanced interface for label editing.</dd>
      <dd>Layout of OK/Cancel buttons consistency improved.</dd>
      <dd>
        Preferences:
        <ul>
          <li>"Select all audio in project, if none selected" (on by default).</li>
          <li>"Beep on completion of longer activities" (system bell, not main output).</li>
          <li>Other preferences cleaned up and explanations improved.</li>
        </ul>
      </dd>
      <dd>Envelopes: Many fixes when copying / pasting / repeating.</dd>
      <dd>Many translation updates.</dd>
      <dd>Track height fixed in several cases.</dd>
      <dd>CleanSpeech mode switching without closing and re-opening fixed.</dd>

      <dt> Opening/Saving Formats </dt>
      <dd>Metadata editor added for OGG, FLAC and WAV/AIFF exports, and general improvements.</dd>
      <dd>Metadata import improved.</dd>
      <dd>Muted tracks are no longer audible in the exported mix.</dd>

      <dt> Effects </dt>
      <dd>Truncate Silence: support for multiple and stereo tracks.</dd>
      <dd>
        Dtmf Generator:
        <ul>
          <li>Added support for keypad letters.</li>
          <li>Added an amplitude control.</li>
        </ul>
      </dd>
      <dd>Compressor: variable decay time added.</dd>
      <dd>
        Equalization:
        <ul>
          <li>Clicks at start / end prevented.</li>
          <li>Improvements to saved curves being found.</li>
          <li>Preview works correctly.</li>
        </ul>
      </dd>
      <dd>"Merge" command appears in Undo history.</dd>
      <dd>Clipping detected more reliably.</dd>
      <dd>Nyquist plug-ins reviewed and enhanced.</dd>
      <dd>Better (and more) progress bars.</dd>
      <dd>Canceling effect always restores previous audio.</dd>
      <dd>Several improvement to effects in batch mode.</dd>

      <dt> Recording/Playback </dt>
      <dd>Improvements to latency correction.</dd>
      <dd>Updated version of portaudio-v19 library.</dd>

      <dt>
        Note that Help is no longer built in, but accessible on the Web via links in Audacity.
      </dt>
    </dl>
  </dd>
</dl>');?>

<h3 id="olderdetails"><?=_("New features in Audacity 1.3.3")?></h3>
<?php
   // i18n-hint: The following are new features in
   // old versions of Audacity, which have never been
   // translated up to now. Feel free to translate 
   // them as a low priority.  
echo _('<dl>
  <dt></dt>
  <dd>
    <!-- indent cheat -->
    <dl>
      <dt> Opening/saving formats </dt>
      <dd>
        Import
        <ul>
          <li>Import of audio from QuickTime (mov, aac, m4a) files now supported on OS X.</li>
          <li>Broadcast Wave Format (BWF) wave files can now be imported.</li>
        </ul>
      </dd>
      <dd>
        Export
        <ul>
          <li>Metadata can be added to OGG files.</li>
          <li>Improved Export option selection.</li>
          <li>Additional export options added to MP3 and FLAC file formats.</li>
          <li>Command line exporter now supported on Windows and OS X.</li>
        </ul>
      </dd>
      <dt> Effects </dt>
      <dd>
        EQ effect
        <ul>
          <li>Responsiveness improved.</li>
          <li>Several enhancements added.</li>
          <li>Batch support added.</li>
        </ul>
      </dd>
      <dd> New Auto Duck effect </dd>
      <dd> Added previewing to AudioUnit effects. </dd>
      <dd> Much improved Noise Removal effect </dd>
      <dd> Effects previewing can now be canceled. </dd>
      <dd> New DTMF Tone Generator effect </dd>
      <dd> Additional options available in Noise effect. </dd>
      <dd> Improved the Tone Generation effects. </dd>
      <dt> Other features </dt>
      <dd> New built-in screen capture utility </dd>
      <dd> Major speed improvement in Spectrogram rendering </dd>
      <dd> Increased support for drag and drop on OS X. </dd>
      <dd> Support added for building against wxWidgets 2.8.x. </dd>
      <dd> Can now open multiple Audacity Projects at once from Explorer on Windows. </dd>
      <dd> Improved main window sliders. </dd>
      <dd> New support for snapping while selecting and sliding </dd>
      <dd> Improved track focus handling and visual feedback. </dd>
      <dd> Speed improvements and handling of resizing/zooming in tracks </dd>
      <dd> Spectrum view can now be zoomed. </dd>
      <dd> New internal file cache to improve handling of project files over networks </dd>
      <dt> Also </dt>
      <dd> Many improvements to language specific translations </dd>
      <dd> Numerous stability improvements </dd>
    </dl>
  </dd>
</dl>');?>

<h3><?=_("New features in Audacity 1.3.1 and 1.3.2")?></h3>

<ol>
<li><a href="features-1.3-h.php"><?=_("Improved Toolbar Docking")?></a></li>
<li><a href="features-1.3-i.php"><?=_("Track focus for improved keyboard navigation")?></a></li>
<li><a href="features-1.3-j.php"><?=_("Repair and Equalization effects")?></a></li>
<li><a href="features-1.3-k.php"><?=_("Timer Recording")?></a></li>
<li><a href="features-1.3-l.php"><?=_("Project saving and recovery")?></a></li>
<li><a href="features-1.3-d.php"><?=_("Selection Bar")?></a></li>
<li><a href="features-1.3-f.php"><?=_("Mac OS X features")?></a></li>
</ol>

<h3><?=_("New features in Audacity 1.3.0")?></h3>

<ol>
<li><a href="features-1.3-b.php"><?=_("Collapse/Expand Tracks")?></a></li>
<li><a href="features-1.3-c.php"><?=_("Multiple clips per track")?></a></li>
<li><a href="features-1.3-e.php"><?=_("Improved Label Tracks")?></a></li>
<li><a href="features-1.3-g.php"><?=_("Other features")?></a></li>
</ol>

<?php
  include "../include/footer.inc.php";
?>