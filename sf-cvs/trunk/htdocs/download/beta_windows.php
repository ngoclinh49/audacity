<?php
/*
 * Copyright 2005 Dominic Mazzoni
 * Copyright 2004 Matt Brubeck
 * Copyright 2007-8 Vaughan Johnson, Gale Andrews
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
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
<?php include "recommended.inc.php" ?>
<ul>

  <li><p>Windows 2000/XP/Vista: <?php printf(_('<a href="%s">Audacity %s installer</a> (.exe file, %.1lf MB) - The latest version of the free Audacity audio editor. See <a href="http://audacityteam.org/wiki/index.php?title=Windows_Vista_OS">further information about Vista</a>.'), "../beta/".$win_exe_unicode_url, win_exe_unicode_version, win_exe_unicode_size)?></p></li>

  <li><p>Windows 98/ME: <?php printf(_('<a href="%s">Audacity %s installer</a> (.exe file, %.1lf MB) - The latest version of the free Audacity audio editor.</a>'), "../beta/".$win_exe_url, win_exe_version, win_exe_size)?></p></li>
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
<ul>
  <li><p><?php printf(_('<a href="%s">LADSPA plugins %s installer</a> (.exe file, %.1lf MB) - over 90 plug-ins.'), "../beta/".$ladspa_url, ladspa_version, ladspa_size)?></p></li>

  <?php include "common.inc.php"; ?>
  <li><p>Windows 2000/XP/Vista: <?php printf(_('<a href="%s">Audacity %s zip file</a> (%.1lf MB)
  - If you cannot run the installer because you are in a computer lab or other restricted environment, download and unzip this or the file below instead.'),
    "../beta/".$win_zip_unicode_url, win_zip_unicode_version, win_zip_unicode_size)?></p></li>

  <li><p>Windows 98/ME: <?php printf('<a href="%s">Audacity %s zip file</a> (%.1lf MB)',
    "../beta/".$win_zip_url, win_zip_version, win_zip_size)?></p></li>
</ul>

<?php include "windows_sys_reqs.inc.php" ?>

<?php
  include "../include/footer.inc.php";
?>
