<?php
// The main version number, when we don't distinguish between platforms
define('stable_version', '1.2.6');

// Note: Sizes are in MB.
define('mac_classic_version', '1.0.0');
define('mac_classic_size',    1.5);

define('macosx_version', '1.2.6a');
define('macosx_size',    3.9);
define('macosx_intel_version', '1.2.5');
define('macosx_intel_size',    3.4);

define('win_exe_version', '1.2.6');
define('win_exe_size',    2.1);
define('win_zip_version', '1.2.6');
define('win_zip_size',    3.0);

define('src_version', '1.2.6');
define('src_size',    5.6);
define('src_suffix',  '.tar.bz2');

$win_exe_url = "audacity-win/audacity-win-".win_exe_version.".exe";
$win_zip_url = "audacity-win-zip/audacity-win-".win_zip_version.".zip";
$src_url = "audacity-src/audacity-src-".src_version.src_suffix;
$macosx_url = "audacity-macosx/audacity-macosx-ppc-".macosx_version.".dmg";
$macosx_intel_url = "audacity-macosx-intel/audacity-macosx-intel-".macosx_intel_version.".dmg";
$mac_classic_url = "audacity-mac/audacity-mac-".mac_classic_version.".sit";
?>
