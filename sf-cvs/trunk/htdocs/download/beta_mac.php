<?php
/*
 * Copyright 2004 Matt Brubeck
 * Copyright 2005 Dominic Mazzoni
 * Copyright 2008-9 Gale Andrews
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/3.0/
 */
  require_once "main.inc.php";
  require_once "../beta/versions.inc.php";
  $pageId = "beta_mac";
  $pageTitle = _("Mac OS X");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<?php include "betawarning.inc.php" ?>

<p><?=$downloadTagline?></p>

<h3 id="recdown"><?=_("Recommended Download")?></h3>
<ul>
  <li><p>OS X 10.4 or later (Universal Binary): <?php printf(_('<a href="%s">Audacity %s</a> (.dmg file, %.1lf MB)'), "".$macosx_ub_url, macosx_ub_version, macosx_ub_size)?></p></li>
</ul>

<p> <b><?=_("Installation instructions (.dmg files)")?>:</b>
 <ol>
   <li><?=_("Inside your Applications folder, create a folder called \"Audacity\"")?></li>
   <li><?=_("Double-click the downloaded .dmg to mount it")?></li>
   <li><?=_("Option-drag the whole of the .dmg contents (not the .dmg itself) into the \"Audacity\" folder you created")?></li>
   <li><?=_("Double-click Audacity.app inside the Applications folder to launch it")?></li>
 </ol>
</p>

<h3 id="optional"><?=_("Optional Downloads")?></h3>
<h4>Plug-ins and Libraries</h4>
<ul>
  <li><p><a href="http://ardour.org/files/releases/swh-plugins-0.4.15.dmg">LADSPA plug-ins installer</a> - over 90 plug-ins.</p></li>
<?php include "beta_common.inc.php"; ?>
<ul>
  <li><p>OS X 10.4 or later (Universal Binary): <?php printf(_('<a href="%s">Audacity %s zip file</a> (%.1lf MB)
  - If you want a download without help files, or prefer not to use .dmg files, download and unzip this file instead.'),
    "".$macosx_ub_zip_url, macosx_ub_zip_version, macosx_ub_zip_size)?></p></li>
  <li><p>OS X 10.4 or later (Universal Binary): <?php echo _('<b>For advanced users</b>, <a href="http://wiki.audacityteam.org/index.php?title=Nightly_Builds#Macintosh_Binaries">Nightly Builds</a> are available for testing purposes.')?>
<?php include "beta_nightly.inc.php"; ?></p></li>
</ul>

<h3 id="sysreq"><?=_("System Requirements")?></h3>
<ul>
  <li><?=_("OS X 10.3 and earlier are not supported, however the previous")?> <a href="http://audacity.googlecode.com/files/audacity-macosx-ub-1.3.3.dmg">1.3.3 Beta</a> (.dmg file, 7.2 MB) <?=_("supports OS X 10.3.")?></li>
  <li><?=_("Audacity runs best with at least 64 MB RAM and a 300 MHz processor.")?></li>
</ul>


<?php
  include "../include/footer.inc.php";
?>