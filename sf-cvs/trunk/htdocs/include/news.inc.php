<?php
/*
 * Copyright 2004 Matt Brubeck
 * 2007-8 Vaughan Johnson, Gale Andrews
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
class NewsItem {
  var $id;
  var $date;
  var $title;
  var $body;

  // Constructor.
  function NewsItem($date, $title, $body) {
    $this->date = $date;
    $this->title = $title;
    $this->body = $body;
  }

  // Returns the date, in the preferred format for the current locale.
  function dateStr() {
    // i18n-hint: Controls how dates are formatted.
    // See http://www.php.net/manual/function.strftime.php for details.
    return locale_to_unicode(strftime(_("%B %d, %Y"), $this->date));
  }
}

$news_items = array();
function add_news_item($dateStr, $id, $title, $body) {
  global $news_items;
  $date = strtotime($dateStr);
  $key = strftime("%Y-%m-%d", $date)."/".$id;
  $news_items[$key] = new NewsItem($date, $title, $body);
}
function most_recent_news_item() {
}

// Add news items below in reverse-chronological order
// (most recent first).

add_news_item(
  "June 09, 2008",
        "PCWorld_100_Best_2008",
  _("Audacity in PC World 100 Best Products of 2008"),
  _("
<p>
The editors of <em>PC World</em> have chosen Audacity as one of the 
<a href=\"http://www.pcworld.com/article/id,146161-page,12-c,electronics/article.html\">
	PC World 100 Best Products of 2008
</a>.
</p>
<p align=\"center\">
	<a href=\"http://www.pcworld.com/article/id,146161-page,12-c,electronics/article.html\">
		<img src=\"../images/PC_World_100BestLogo2008_small.jpg\" 
			alt=\"PC World 100 Best Products of 2008\"></img>
	</a>
</p>
<p>
The 
<a href=\"http://www.pcworld.com/article/id,146161-page,12-c,electronics/article.html\">article</a> 
is available now on PCWorld.com, and will also be featured
in the July 2008 issue of PC World, which hits newsstands June 17.
</p>
<p>
The respected 100 Best Products Awards honor products that meld practical features with
innovation and reflect the rapidly changing technology marketplace. To select the winners,
PC World's editors examined hundreds of products, including those that have appeared in the
magazine over the past 12 months. The 100 winning products and services were selected for their
exemplary design and usability, features, performance, and innovation.
</p>
<p>
&ldquo;These awards go to the best technology products we've seen in the last 12 months,&rdquo; 
stated Edward N. Albro, editor of PC World. &ldquo;Our editors looked at hundreds of hardware,
software and Web products to compile this list of outstanding performers. Congratulations to
Audacity.&rdquo; 
</p>
"));

add_news_item(
  "May 08, 2008",
        "1.3.5-release",
  _("Audacity 1.3.5 Released"),
  _("
<p>
The Audacity Team is pleased to announce the release of
<a href=\"/download\">Audacity 1.3.5 (beta)</a> for Windows, 
Mac and Linux/Unix. Changes include improvements and new 
features for recording, import/export and the user interface. 
Because it is a work in progress and does not yet come with 
complete documentation or translations into foreign languages, 
it is recommended for more advanced users. For all users, 
<a href=\"/download\">Audacity 1.2.6</a> is a stable release, 
complete and fully documented. You can have Audacity 1.2.6 and 
1.3.5 installed on the same machine.
</p>

<b>Beta version 1.3.5</b>
<dl>
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
</dl>
		
<p>
See the included README.txt for more information about this release and <a href=\"/download/features-1.3-a\">New Features in 1.3</a>
for more information about the 1.3.x beta series.
</p>
"));

add_news_item(
  "April 21, 2008",
        "GSoC_2008",
  _("Audacity is mentoring five Google Summer of Code (GSoC) 2008 students"),
  _("
<p>
	Audacity has been approved to mentor
	five <a href=\"http://code.google.com/soc/2008/audacity/about.html\">students</a> for
       <a href=\"http://code.google.com/soc/2008/\">Google Summer of Code 2008</a>!
	GSoC offers student developers $4,500 stipends to write code for various open source projects.
</p>
<p align=\"center\">
	<a href=\"http://code.google.com/soc/2008/\">
	  <img src=\"http://google-summer-of-code.googlecode.com/files/soc08-198x128_white.jpg\" alt=\"Google Summer of Code 2008\"></img>
	</a>
</p>
<p>
	Congratulations to the five approved students and thanks to all who applied for their interest in Audacity!
</p>
"));

add_news_item(
  "April 18, 2008",
        "GSoC_2008",
  _("Audacity is a mentoring organization Google Summer of Code (GSoC) 2008 "),
  _("
<p>
	Audacity is a mentoring organization
	for the <a href=\"http://code.google.com/soc/2008/\">Google Summer of Code 2008</a>!
	GSoC offers student developers $4,500 stipends to write code for various open source projects.
</p>
<p align=\"center\">
	<a href=\"http://code.google.com/soc/2008/\">
	  <img src=\"http://google-summer-of-code.googlecode.com/files/soc08-198x128_white.jpg\" alt=\"Google Summer of Code 2008\"></img>
	</a>
</p>
<p>
	All the student applications are in, and scored. Announcements will be made on April 21.
	Thanks for your interest!
</p>
"));

add_news_item(
  "March 24, 2008",
        "GSoC_2008",
  _("Call for Students! Apply <em>NOW</em> for Audacity projects in Google Summer of Code (GSoC) 2008 "),
  _("
<p>
	Students! Interested in supporting Audacity and earning a stipend this summer? Extended application deadline!
	<em><a href=\"http://groups.google.com/group/google-summer-of-code-announce/web/guide-to-the-gsoc-web-app-for-student-applicants\">
		Apply by 5:00 PM PDT on April 7, 2008 (00:00 UTC on April 8, 2008)!</a></em>
</p>
<p>
	Audacity is a mentoring organization
	for the <a href=\"http://code.google.com/soc/2008/\">Google Summer of Code 2008</a>!
	GSoC offers student developers $4,500 stipends to write code for various open source projects.
</p>
<p align=\"center\">
	<a href=\"http://code.google.com/soc/2008/\">
	  <img src=\"http://google-summer-of-code.googlecode.com/files/soc08-198x128_white.jpg\" alt=\"Google Summer of Code 2008\"></img>
	</a>
</p>
<p>
	We are now seeking student participants.
	We look for evidence that the student has a real interest in
	our project, 'Do they actually use it?' rather than just choosing some project that is part of GSoC.
</p>
<p>
	If you are an interested student, please follow these steps:
	<ol>
		<li>Review the <a href=\"http://groups.google.com/group/google-summer-of-code-announce/web/guide-to-the-gsoc-web-app-for-student-applicants\">student app documentation</a>.
			Also check the <a href=\"http://code.google.com/opensource/gsoc/2008/faqs.html\">GSoC FAQs</a>,
			especially from <a href=\"http://code.google.com/opensource/gsoc/2008/faqs.html#0.1_student_apply\">'How does a student apply?'</a>.</li>
		<li>Make sure you are <a href=\"http://code.google.com/opensource/gsoc/2008/faqs.html#0.1_eligibility\">eligible</a>.</li>
		<li>On the Audacity Wiki, check the <a href=\"http://www.audacityteam.org/wiki/index.php?title=GSoC_Ideas\">project ideas page</a>.</li>
		<li>If you find an interesting idea, or have one of your own, check <a href=\"http://www.audacityteam.org/wiki/index.php?title=Writing_GSoC_Proposals\">Writing GSoC Proposals</a>.</li>
		<li>Then <a href=\"mailto:summerofcode@audacityteam.org\">email us</a>. Far better to discuss plans with us before applying.</li>
	</ol>
</p>
<p>
If you know any students who might be interested, possibly those already involved in music/audio
research, please point them to our <a href=\"http://audacity.sourceforge.net/\">website</a> for this information.
</p>
"));

add_news_item(
  "March 17, 2008",
        "GSoC_2008",
  _("Audacity and Google Summer of Code (GSoC) 2008 "),
  _("
<p>
We are very happy to announce that Audacity has been accepted as a mentoring organization
for the <a href=\"http://code.google.com/soc/2008/\">Google Summer of Code 2008</a>!
GSoC 'offers student developers stipends to write code for various open source projects'.
</p>
<p>
We are now seeking student participants. As we wrote in our application:
At student selection stage, we look for evidence that the student has a real interest in
our project, 'Do they actually use it?' rather than just choosing some project that is part of GSoC.
</p>
<p>
If you are an interested student, please follow these steps:
<ol>
	<li>Review the <a href=\"http://code.google.com/opensource/gsoc/2008/faqs.html\">GSoC FAQ</a>,
		especially from <a href=\"http://code.google.com/opensource/gsoc/2008/faqs.html#0.1_student_apply\">'How does a student apply?'</a>.</li>
	<li>Make sure you are <a href=\"http://code.google.com/opensource/gsoc/2008/faqs.html#0.1_eligibility\">eligible</a>.</li>
	<li>On the Audacity Wiki, check the <a href=\"http://www.audacityteam.org/wiki/index.php?title=GSoC_Ideas\">project ideas page</a>.</li>
	<li>If you find an interesting idea, or have one of your own, then check <a href=\"http://www.audacityteam.org/wiki/index.php?title=Writing_GSoC_Proposals\">Writing GSoC Proposals</a>.</li>
	<li>Then be in touch with us -- far better to discuss plans with us before applying.</li>
</ol>
</p>
<p>
If you know any students who might be interested, possibly those already involved in music/audio
research, please point them to our <a href=\"http://audacity.sourceforge.net/\">website</a> for this information.
</p>
"));

add_news_item(
  "February 23, 2008",
        "LinuxQuestions_Award_2007",
  _("LinuxQuestions.org Audio Authoring Award 2007"),
  _("
<p>
Audacity was chosen as
<a href=\"http://www.linuxquestions.org/questions/2007-linuxquestions.org-members-choice-awards-79/audio-authoring-application-of-the-year-610225/?s=753ec1178602a18491f741f03855304d\">
Audio Authoring Application of the Year</a> in the 2007 LinuxQuestions.org Members Choice poll. LinuxQuestions.org is an active online Linux community. In the voting, Audacity received nearly 70% of the 444 votes cast. Thanks to everyone who voted.
</p>
"));

add_news_item(
  "November 13, 2007",
        "1.3.4-release",
  _("Audacity 1.3.4 Released"),
  _("
<p>
The Audacity Team is pleased to announce the release of
<a href=\"/download\">Audacity 1.3.4 (beta)</a>, which includes several
new features and user interface improvements, such as:
<ul>
	<li>New Welcome Screen with introduction to Audacity.</li>
	<li>New 'Mix and Render to New Track' command.</li>
	<li>Support for VAMP audio analysis plug-ins.</li>
	<li>More keyboard shortcuts and navigation.</li>
	<li>Reworked solo/mute handling.</li>
	<li>New preference: Select all audio in project, if none selected (on by default).</li>
	<li>New preference: Beep on completion of longer activities.</li>
	<li>Envelopes: Many fixes when copying, pasting, or repeating.</li>
	<li>Many translation updates.</li>
	<li>Metadata editor added for OGG, FLAC and WAV/AIFF exports. Metadata import improved.</li>
	<li>Muted tracks are no longer audible in the exported mix.</li>
	<li>Improvements to latency correction.</li>
</ul>
</p>
<p>
Note that this release is for Windows and Linux/Unix only.
The latest Audacity beta for Mac OS is version 1.3.3.
</p>
<p>
See <a href=\"/download/features-1.3-a\">New Features in 1.3</a>
for more information about 1.3.4 and the 1.3.x beta series.
</p>
<p>
Check out the new e-book on
<a href=\"http://www.informit.com/store/product.aspx?isbn=0132366576&rl=1\">Podcasting with Audacity</a>.
</p>
"));

add_news_item(
  "July 26, 2007",
        "SourceForge_CCA_2007",
  _("SourceForge Community Choice Awards 2007"),
  _("
<p>
Audacity won the
<a href=\"http://sourceforge.net/community/index.php/landing-pages/cca07/\">
	SourceForge Community Choice Awards 2007 -- Multimedia Category
</a>!
Congratulations to the other finalists.
<b>Big</b> thanks to SourceForge, to everyone who voted, and to all
who contribute to making Audacity great!
</p>
"));

add_news_item(
  "May 18, 2007",
        "1.3.3-release",
  _("Audacity 1.3.3 Released"),
  _("
<p>
The Audacity Team is pleased to announce the release of
<a href=\"/download\">Audacity 1.3.3 (beta)</a>, which
contains numerous new features and capabilities beyond the 1.3.2 (beta) release.
Because it is a work
in progress and does not yet come with complete documentation or translations
into foreign languages, it is recommended for more advanced users.
For all users, <a href=\"/download\">Audacity 1.2.6</a>
is a stable release, complete and fully documented.  You can have
both Audacity 1.2.6 and 1.3.3 installed simultaneously.
</p>

<p>
See <a href=\"/download/features-1.3-a\">New Features in 1.3</a>
for more information about the 1.3.x beta series.
</p>

<b>Beta version 1.3.3</b>
<dl><dt></dt><dd><!-- indent cheat -->
<dl>
	<dt> Opening/saving formats </dt>
		<dd> Import
			<ul>
				<li>Import of audio from QuickTime (mov, aac, m4a) files now supported on OS X.</li>
				<li>Broadcast Wave Format (BWF) wave files can now be imported.</li>
			</ul>
		</dd>
		<dd> Export
			<ul>
				<li>Metadata can be added to OGG files.</li>
				<li>Improved Export option selection.</li>
				<li>Additional export options added to MP3 and FLAC file formats.</li>
				<li>Command line exporter now supported on Windows and OS X.</li>
			</ul>
		</dd>
	<dt> Effects </dt>
		<dd> EQ effect
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
</dd></dl>
"));

add_news_item(
  "October 30, 2006",
        "1.3.2-release",
  _("Audacity 1.3.2 and 1.2.5 Released"),
  _("<p>
The Audacity developers have been busy with many new features over
the past year.  We're pleased to announce
<a href=\"/download/features-1.3-a\">Audacity 1.3.2 (beta)</a>, which
contains dozens of new features and capabilities.  Because it is a work
in progress and does not yet come with complete documentation or translations
into foreign languages, it is recommended for more advanced users.
For all users, <a href=\"/download\">Audacity 1.2.5</a>
is a minor bug-fix update that addresses some problems with Audacity 1.2.4,
but does not add any significant new features.
It is complete and fully documented.  You can have
both Audacity 1.2.5 and 1.3.2 installed simultaneously.
Also, we have just made available a set of 92
<a href=\"/download/plugins\">LADSPA plug-ins for Windows</a>
(for both Audacity 1.2.x and 1.3.x).

</p><p>

See the <a href=\"/download/release-notes\">1.2.5 Release Notes</a> for a complete list of changes and known problems in Audacity 1.2.5, or see
<a href=\"/download/features-1.3-a\">New Features in 1.3</a>
for information about the new beta version.

</p>

<b>Beta version 1.3.2</b>
<dl><dt></dt><dd><!-- indent cheat -->
<dl>
       <dt> Usability improvements </dt>
             <dd> New selection bar </dd>
             <dd> New features for label tracks </dd>
             <dd> Improved toolbar docking flexibility </dd>
             <dd> Menu renaming and reorganization </dd>
             <dd> Selection, ruler, and playback control improvements </dd>
       <dt> Major improvements to some built-in effects </dt>
              <dd> New Repair effect </dd>
              <dd> Improved Equalization effect </dd>
              <dd> Many fixes and minor improvements to other effects </dd>
       <dt> Improved accessibility for the visually impaired </dt>
             <dd> Improvements for screen readers, accessibility of tracks, and hot keys </dd>
       <dt> Timer recording </dt>
       <dt> Auto-save and automatic crash recovery </dt>
       <dt> New features and bug fixes for Nyquist </dt>
       <dt> Restructured Preferences dialog </dt>
       <dt> Improved batch processing </dt>
       <dt> File format export improvements </dt>
       <dt> Intel Mac support </dt>
       <dt> Many bug fixes and stability improvements </dt>
</dl>
</dd></dl>

<b>Stable version 1.2.5</b>
<dl><dt></dt><dd><!-- indent cheat -->
<dl>
       <dt> Support for new file formats, including FLAC </dt>
       <dt> Fixes for Mac audio problems </dt>
       <dt> Fix Generate Silence bug, a crash issue, and Linux GCC build issues </dt>
       <dt> Intel Mac support </dt>
</dl>
</dd></dl>")
);

add_news_item(
  "August 14, 2006",
	"AudacityStore",
  _("The Audacity Store Is Open"),
  _('
	<p>You are invited to try out the new <a href="http://audacitystore.com/">Audacity Store</a>, which features
		Audacity-logo items (T-shirts, embroidered polo shirts, embroidered messenger bags), and consumer electronics.
	</p>
	<p align="center">
		<a href="http://audacitystore.com/">
		  <img src="../images/Audacity Store_banner_50pct.jpg" alt="Audacity Store"></img>
		</a>
	</p>
	<p><a href="http://audacity.sourceforge.net/community/donate">Learn more about how Audacity raises money...
		</a>
	</p>
	<h3>New Releases on the Way</h3>
	<p>Also, we will soon release updates to both the stable 1.2.x and development 1.3.x lines.
		The new 1.3.x version will be 1.3.2 -- no official 1.3.1 because there are lots of changes
			and several unofficial 1.3.1 builds have already been posted.
		Major changes in the 1.3.2 release include:
		<dl><dt></dt><dd><!-- indent cheat -->
			<dl>
				<dt>Preliminary Intel Mac Support</dt>
					<dd>pre-release builds at <a href="http://audacityteam.org/mac">http://audacityteam.org/mac</a></dd>
				<dt>Dependencies dialog</dt>
					<dd>lets user see dependencies of project on other files, and
						copy audio data directly into the project</dd>
				<dt>Crash Recovery</dt><dd>automatically saved data makes recovery fast and easy</dd>
				<dt>New Repair Effect</dt><dd>smooths corrupted waveforms</dd>
				<dt>UI Changes</dt>
					<dd>themes (custom user interfaces), new toolbar docking features, new
						time-specification controls, increased accessibility support, history
						window changes, lots more</dd>
				<dt>Selection Bar Improvements</dt><dd>increased control and bug fixes</dd>
				<dt>Equalization Effect Improvements</dt><dd>better layout of Graphic EQ, faster animation of curve, increased control</dd>
				<dt>Many More LADSPA effects for Windows</dt><dd>most LADSPA effects now ported to Windows</dd>
				<dt>Numerous Bug Fixes</dt><dd>fixes to hotkeys, directories, Portable Audacity, crashes, etc.</dd>
				<dt></dt><dd></dd>
			</dl>
		</dd></dl>
	</p>
	')
);

add_news_item(
  "November 28, 2005",
	"1.2.4-release",
  _("Audacity 1.2.4 and 1.3.0 Released"),
  _('<p>Audacity 1.2.4 is a new stable version of Audacity.  It includes a couple of bug fixes and minor improvements and is recommended for all users.  Audacity 1.3.0 is a beta release that contains hundreds of <a href="/download/features-1.3-a">new features</a>, but this version is unfinished and unstable, and is recommended primarily for advanced users.  You can install both Audacity 1.2 and 1.3 simultaneously.</p>
<p>See the <a href="/download/release-notes">1.2.4 Release Notes</a> for a complete list of changes and known problems in Audacity 1.2.4, or see <a href="/download/features-1.3-a">New Features in 1.3</a> for information about the new beta version.</p>
<p>Finally, the best way to get help with Audacity is now the new
<a href="http://audacityteam.org/forum/">Audacity Forum</a>.
</p>')
);

add_news_item(
  "November 19, 2004",
	"1.2.3-release",
  _("Audacity 1.2.3 Released"),
  _('<p>Audacity 1.2.3 is a new stable version of Audacity. This version fixes a bug that interfered with long recordings on some Windows systems, and another bug that causes random crashes on Mac OS X. It also includes several updated translations, and other bug fixes and minor improvements.</p>
<p>See the <a href="/download/release-notes">Release Notes</a> for a complete list of changes and known problems in this version.</p>'));

?>