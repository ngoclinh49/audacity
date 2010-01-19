<?php
/*
 * Copyright 2005 Dominic Mazzoni
 * Copyright 2004 Matt Brubeck
 * Copyright 2007-8 Vaughan Johnson, Gale Andrews
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/3.0/
 */
  require_once "main.inc.php";
  require_once "../beta/versions.inc.php";
  $pageId = "beta_windows";
  $pageTitle = _("Windows");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<?php include "betawarning.inc.php" ?>

<h3 id="recdown"><?=_("Recommended Download")?></h3>
<ul>
  <li><p>Windows 2000/XP/Vista: <?php printf(_('<a href="%s">Audacity %s installer</a> (.exe file, %.1lf MB) - The latest version of the free Audacity audio editor, including help files. <b>See further information about</b> <a href="http://audacityteam.org/wiki/index.php?title=Windows_7_OS">Windows 7</a> <b>and</b> <a href="http://audacityteam.org/wiki/index.php?title=Windows_Vista_OS">Vista</a><b>.</b>'), "".$win_exe_unicode_url, win_exe_unicode_version, win_exe_unicode_size)?></p></li>
<div id="ansi"></div>
  <li><p>Windows 98/ME: <?php printf(_('<a href="%s">Audacity %s installer</a> (.exe file, %.1lf MB) - The latest available version for Windows 98 and ME.</a>'), "".$win_exe_url, win_exe_version, win_exe_size)?></p></li>
</ul>

<h3 id="optional"><?=_("Optional Downloads")?></h3>
<?php
  // i18n-hint:  If this string is translated, it will appear above the list of
  // optional downloads on the "downloads/windows" page.  If there are extra
  // files available in your language, add them here.  Otherwise, do not
  // translate this string.  (Or make the translation identical to the
  // original string.)
  if (_("localized_downloads_windows") != "localized_downloads_windows") {
    echo _("localized_downloads_windows");
  }
?>
<h4><?=_("Plug-ins and Libraries")?></h4>
<ul>
  <li><p><?php printf(_('<a href="%s">LADSPA plug-ins %s installer</a> (.exe file, %.1lf MB) - over 90 plug-ins.'), "../beta/".$ladspa_url, ladspa_version, ladspa_size)?></p></li>
  <?php include "beta_common.inc.php"; ?>
<ul>
  <li><p>Windows 2000/XP/Vista: <?php printf(_('<a href="%s">Audacity %s zip file</a> (%.1lf MB)
  - If you want a download without help files, or cannot run the installer because of restricted permissions, download and unzip this or Windows 98/ME file below instead.'),
    "".$win_zip_unicode_url, win_zip_unicode_version, win_zip_unicode_size)?></p></li>

  <li><p>Windows 2000/XP/Vista: <?php echo _('<b>For advanced users</b>, <a href="http://wiki.audacityteam.org/index.php?title=Nightly_Builds#Windows_Binaries">Nightly Builds</a> are available for testing purposes.')?>
<?php include "beta_nightly.inc.php"; ?></p></li>

  <li><p>Windows 98/ME: <?php printf('<a href="%s">Audacity %s zip file</a> (%.1lf MB)',
    "".$win_zip_url, win_zip_version, win_zip_size)?></p></li>
</ul>
<h3 id="sysreq"><?=_("System Requirements")?></h3>
<p>
  <?=_("Windows 95 and NT are not supported. Windows 7 is <a href=\"http://wiki.audacityteam.org/wiki/Windows_7_OS\">provisionally supported</a>.")?>
</p>
<?php include "windows_sys_reqs.inc.php" ?>

<?php
  include "../include/footer.inc.php";
?>