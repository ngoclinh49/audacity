<?

# perform a redirect to the latest version, so links to it can remain static

include 'mirror.inc.php';
include 'versions.inc.php';
header('Location: '.download_url('audacity-macosx-ppc-'.macosx_ppc_version.'.dmg'));

?>
