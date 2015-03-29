<?php
/*
 * Copyright 2004 - 15 Matt Brubeck, Gale Andrews
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/3.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  require_once "../latest/mirror.inc.php";
  $pageId = "plugins";
  $pageTitle = _("Plug-Ins and Libraries");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>
<p><?=_('You can download and install plug-ins or libraries to add extra functionality to Audacity. Plug-ins can give you extra effects, or more audio generation and analysis capability. Adding libraries can allow you to import or export additional audio formats.')?></p>

<h3><?=_("Plug-In Installation")?></h3>
<p><?=_('To install new LADSPA or Nyquist plug-ins, place them in the <b>Plug-Ins</b> folder inside the Audacity installation folder.  On Windows computers, this is usually under "Program Files".  On Mac OS X, it is usually under "Applications". On Linux, you can use <a href="http://manual.audacityteam.org/help/manual/man/faq_installation_and_plug_ins.html#linux_plugins">various plug-in locations</a>. Restart Audacity, then the addded plug-ins will appear underneath the divider in the "Effect", "Generate" or "Analyze" menus.')?></p>

<h3><?=_("LADSPA Plug-Ins")?></h3>
<p>
  <?php printf(_('Audacity has built-in support for LADSPA plug-ins.  These plug-ins are mostly built for Linux, but some are available for other operating systems too.  Audacity includes some sample LADSPA effects. Windows users can install an additional <a href="%s">set of over 90 LADSPA plug-ins</a>.  There is a similar set of LADSPA plug-ins for <a href="%s">Mac</a>. More information and many LADSPA plug-ins for Linux can be found on the <a href="%s">LADSPA web site</a>.'), download_url($ladspa_url), download_url('swh-plugins-mac-0.4.15.zip'), 'http://www.ladspa.org/')?></p>

<h3><?=_("LV2 Plug-Ins")?></h3>
<p>
  <?php printf(_('Audacity has built-in support for <a href="http://lv2plug.in/">LV2</a> plug-ins, which are an extensible successor of LADSPA effects. LV2 plug-ins are mostly built for Linux, but Audacity supports LV2 on all operating systems. To install LV2 plug-ins, place them in the <a href="%s">system LV2 location</a> then restart Audacity.'), 'http://manual.audacityteam.org/o/man/effect_menu.html#LV2_effects')?></p>

<h3><?=_("Nyquist Plug-Ins")?></h3>
<p><?=_('Audacity has built-in support for Nyquist effects on all operating systems. You can download additional <a href="nyquistplugins">Nyquist plug-ins</a>, or create your own using the <a href="../help/nyquist">Nyquist programming language</a>. Nyquist code can be tested using "Nyquist Prompt" under the Effect menu, or code for Nyquist plug-ins that generate audio can be quickly tested with <a href="http://audacity.sourceforge.net/nyquist/generate.zip">Nyquist Generate Prompt</a>.')?>

<h3><?=_("VST Plug-Ins")?></h3>
<p><?=_('Audacity can load VST effects (but not VST instruments) on all operating systems. The VST Enabler is no longer required.  Enable "Rescan VST effects" in the Effects Preferences and restart Audacity when you want to add new effects. See <a href="http://manual.audacityteam.org/help/manual/man/faq_installation_and_plug_ins.html#vst_install">"How do I install VST plug-ins?"</a> for more information.')?></p>

<p><?=_('VST effects can be found on many plug-in sites such as:')?></p>
<ul>
<li><a href="http://www.hitsquad.com">Hitsquad</a>: &nbsp;<a href="http://www.hitsquad.com/smm/win95/PLUGINS_VST/">Windows</a>, <a href="http://www.hitsquad.com/smm/mac/PLUGINS_VST/">Mac</a></li>
<li><a href="http://www.kvraudio.com/">KVR Audio</a>: &nbsp;<a href="http://www.kvraudio.com/q.php?search=1&os[]=win32&ty[]=e&f1[]=vst&pr[]=f&sh[]=s">Windows</a>, <a href="http://www.kvraudio.com/q.php?search=1&os[]=mac32&ty[]=e&f1[]=vst&pr[]=f&sh[]=s">Mac</a></li>
<li><a href="http://dmoz.org/Computers/Multimedia/Music_and_Audio/Software/Plug-ins/">Open Directory</a> (Windows, Mac).</li>
</ul>

<p><?=_('The <a href="http://wiki.audacityteam.org/wiki/VST_Plug-ins">VST Plug-ins</a> page on the <a href="http://wiki.audacityteam.org/">Audacity Wiki<a/> contains further help for VST plug-ins, and lists a large number of VST plug-ins that have been reported to work well in Audacity.')?></p>

<h3><?=_("Audio Unit Plug-Ins")?></h3>
<p><?=_('On Mac OS X only, Audacity loads <a href="http://manual.audacityteam.org/o/man/effect_menu.html#Audio_Unit_effects">Audio Unit</a> plug-ins from system plug-in directories. Audacity will not recognize any Audio Units in its own "plug-ins" folder.')?></p>  

<h3><?=_("Libraries")?></h3>
<p><?=_('The <b>LAME MP3 encoding</b> library allows Audacity to export audio in the popular <a href="http://wiki.audacityteam.org/index.php?title=MP3">MP3</a> format. To install the LAME library, please read our <a href="http://manual.audacityteam.org/help/manual/man/faq_installation_and_plug_ins.html#lame">LAME FAQ</a>.')?></p>    

<p><?=_('The <b>FFmpeg import/export</b> library allows Audacity to import and export many additional audio formats such as AC3, AMR(NB), M4A and WMA, and to import audio from video files. Audacity 2.0.6 and later requires FFmpeg 1.2 to 2.3.x (or libav 0.8 to 0.10.x). To install FFmpeg, please read our <a href="http://manual.audacityteam.org/help/manual/man/faq_installation_and_plug_ins.html#ffdown">FFmpeg FAQ</a>.')?></p>

<?php
  include "../include/footer.inc.php";
?>