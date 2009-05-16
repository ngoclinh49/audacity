<?php
/*
 * Copyright 2004 - 8 Matt Brubeck, Gale Andrews
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/3.0/
 */
  require_once "main.inc.php";
  $pageId = "plugins";
  $pageTitle = _("Plug-Ins");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>
<p><?=_('You can download and install plug-ins to add extra effects to Audacity, or to add more audio generation and analysis capability. Plug-ins appear at the bottom of the "Effect", "Generate" or "Analyze" menus.  To install new plug-ins, place them in the <b>Plug-Ins</b> folder inside the Audacity installation folder.  On Windows computers, this is usually under "Program Files."  On Mac OS X, it is usually under "Applications."')?></p>
<p><?=_('<b>Note:</b> You must restart Audacity to make newly added plug-ins appear in the menus.')?></p>   

<h3><?=_("LADSPA Plug-Ins")?></h3>
<p><?=_('Audacity has built-in support for LADSPA plug-ins.  These plug-ins are mostly built for Linux, but some are available for other operating systems too.  Audacity includes some sample LADSPA effects. Windows users can install an additional <a href="http://audacity.sourceforge.net/beta/ladspa/ladspa-0.4.15.exe">set of over 90 LADSPA plug-ins</a>. There is a similar set of LADSPA plug-ins for <a href="http://ardour.org/files/releases/swh-plugins-0.4.15.dmg">Mac</a>. More information and many LADSPA plug-ins for Linux can be found on the <a href="http://www.ladspa.org/">LADSPA web site</a>.')?></p>

<h3><?=_("Nyquist Plug-Ins")?></h3>
<p><?=_('Audacity has built-in support for Nyquist effects on all operating systems. You can download additional <a href="nyquistplugins">Nyquist plug-ins</a>, or create your own using the <a href="../help/nyquist">Nyquist programming language</a>. Nyquist code can be tested using "Nyquist Prompt" under the Effect menu, or code for Nyquist plug-ins that generate audio can be quickly tested with <a href="http://audacity.sourceforge.net/nyquist/generate.zip">Nyquist Generate Prompt</a>.')?>

<h3><?=_("VST Plug-Ins")?></h3>
<p><?=_('Audacity can load VST effects on Windows and Mac using the optional <a href="../help/faq?s=install&amp;i=vst-enabler">VST Enabler</a>.  VST effects can be found on many plug-in sites such as:')?></p>
<ul>
<li><a href="http://free-loops.com/free-vst-plugins.php">Free-Loops</a> (Windows)</li>
<li><a href="http://www.hitsquad.com">Hitsquad</a>: &nbsp;<a href="http://www.hitsquad.com/smm/win95/PLUGINS_VST/">Windows</a>, <a href="http://www.hitsquad.com/smm/mac/PLUGINS_VST/">Mac</a></li>
<li><a href="http://www.kvraudio.com/">KVR Audio</a>: &nbsp;<a href="http://www.kvraudio.com/get.php?mode=results&st=q&s=6">Windows</a>, <a href="http://www.kvraudio.com/get.php?mode=results&st=q&s=8">Mac</a></li>
<li><a href="http://dmoz.org/Computers/Multimedia/Music_and_Audio/Software/Plug-ins/">Open Directory</a> (Windows, Mac).</li>
</ul>

<p><?=_('The <a href="http://audacityteam.org/wiki/index.php?title=VST_Plug-ins">VST Plug-ins</a> page on our <a href="http://audacityteam.org/wiki/">Wiki<a/> lists a large number of VST plug-ins that have been reported to work well in Audacity.')?></p>

<?php
  include "../include/footer.inc.php";
?>