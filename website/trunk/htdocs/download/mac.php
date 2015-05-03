<?php
/*
 * Copyright 2004 - 2014
 * Matt Brubeck
 * Dominic Mazzoni
 * Richard Ash
 * Gale Andrews 
 * Vaughan Johnson
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/3.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  require_once "../latest/mirror.inc.php";
  $pageId = "mac";
  $pageTitle = _("Mac OS X");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<h3><?=_("Recommended Downloads - Latest Version of Audacity")?></h3>
<ul>
  <li>
    <p>
      <?php printf(_('<a href="%s">Audacity %s .dmg file</a> (%.1lf MB, includes help files)'), 
                    download_url($macosx_url), macosx_version, macosx_size)?>
    </p>
  </li>
  <li>
    <p>
      <?php printf(_('<a href="%s">Audacity %s zip file</a> (%.1lf MB, smaller, but no help files)'),
                    download_url($macosx_zip_url), macosx_version, macosx_zip_size)?>
    </p>
  </li>
</ul>

<p>
  &nbsp;&nbsp;&nbsp;<a href="#sysreq"><?=_("System Requirements")?></a>
</p>

<p> <b><?=_("Installation instructions (.dmg files)")?>:</b>
  <ol>
   <li><?=_("Double-click the downloaded DMG to mount it.")?></li>
   <li><?=_("Do not double-click the \"Audacity\" folder in the DMG window. Drag the entire \"Audacity\" folder 
icon to the <b>/Applications</b> folder icon on the right (or to any other location of your choosing).")?></li>
   <li><?=_("Eject the DMG at bottom left of Finder, then launch Audacity.app from the \"Audacity\" folder 
in /Applications or from your chosen location.")?></li> 
 </ol>
</p>

<div class="advice">
<?=_('<b>Known Issue:</b> Security settings on OS X 10.7 or later may block Audacity being 
launched. Most users on OS X 10.9.5 will experience this, due to changes made by Apple.')?>
<p><ul><li>
<?=_('To permanently enable Audacity launch, right-click or control-click on the Audacity 
application in Finder, choose "Open", then in the dialog box that appears, choose "Open".')?>
</li></ul></p>
</div>

<h3 id="optional"><?=_("Optional Downloads")?></h3>
<h4><?=_("Plug-ins and Libraries")?></h4>
<ul>
  <li><p><a href="<?=download_url("swh-plugins-mac-0.4.15.zip")?>"><?=_("LADSPA plug-ins zip file</a> - over 90 plug-ins.")?></p></li>
  <li>
    <p>
      <a href="plugins">
        <?=_("Plug-Ins")?>
      </a> - <?=_("Download additional effects and filters.")?>
    </p>
  </li>
  <li>
    <p>
      <a href="http://manual.audacityteam.org/help/manual/man/faq_installation_and_plug_ins.html#maclame">
        <?=_("LAME MP3 encoder")?>
      </a> - <?=_("Allows Audacity to export MP3 files.")?>
    </p>
  </li>
  <li>
    <p>
      <a href="http://manual.audacityteam.org/o/man/faq_installation_and_plug_ins.html#ffdown">
        <?=_("FFmpeg import/export library")?>
      </a> - <?=_("Allows Audacity to import and export many additional audio formats such as AC3, AMR(NB), M4A and WMA, and to import audio from video files.")?>
    </p>
  </li>
</ul>

<h4>
  <?=_("Alternative Download Links")?>
</h4>
<ul>
  <li>
       <?php printf(_('<a href="%s">OldFoss</a> hosts the current Audacity version and all previous versions.'), "http://www.oldfoss.com/Audacity.html")?>
      </li>
      <li>
        <?php printf(_('<a href="%s">Google Code</a> hosts selected previous versions up to and including Audacity 2.0.5.'), "https://code.google.com/p/audacity/downloads/list?can=1")?>
      </li>
</ul>

<h4>
  <?=_("Audacity Nightly Builds")?>
</h4>
<ul>
  <li><p><?php echo _('<b>For advanced users</b>, <a href="http://wiki.audacityteam.org/index.php?title=Nightly_Builds#mac">Nightly Builds</a> are available for testing purposes.')?>
    <?php include "beta_nightly.inc.php"; ?></p></li>
</ul>

<h3 id="sysreq"><?=_("System Requirements")?></h3>
<p>
<div class="advice">
<b><?=_("Audacity for Mac is released as a Universal Binary. It runs on <b>OS X 10.4 to 10.10.x</b>.")?></b>
<br>
<b><?=_("Audacity runs best with at least 1 GB RAM and a 1 GHz processor (2 GB RAM/2 GHz on OS X 10.7 or later).")?></b> 
<p><?=_("For lengthy multi-track projects, we recommend a minimum of 2 GB RAM and 2 GHz processor (4 GB RAM on OS X 10.7 or later).")?></p>
</div>
</p>

<?php
  include "../include/footer.inc.php";
?>
