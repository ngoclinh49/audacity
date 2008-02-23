<?php
/*
 * Copyright 2004 Matt Brubeck
 * Copyright 2005 Dominic Mazzoni
 * 2008 Gale Andrews  
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
  require_once "main.inc.php";
  $pageId = "beta_linux";
  $pageTitle = _("Linux/Unix");
  include "../include/header.inc.php";
?>

<h2><?=$pageTitle?></h2>

<?php include "betawarning.inc.php" ?>
<?php include "betafeatures.inc.php" ?>

<p><?=$downloadTagline?></p>

<h3 id="recdown"><?=_("Downloads")?></h3>

<p><?=_('Installation packages for Audacity on GNU/Linux and other Unix-like systems are provided by individual distributions. However the Beta version has not yet been packaged for very many distributions. The following distributions are known to have Beta packages:')?></p>

<p><?=_('
<ul>
 <li><a href="http://koji.fedoraproject.org/koji/packageinfo?packageID=1352">Fedora</a></li>
 <li><a href="http://packages.gentoo.org/package/media-sound/audacity">Gentoo</a></li>
 <li><A href="http://packman.links2linux.org/package/286">SuSE/Open SUSE</a></li>
 <li>Ubuntu: <a href="http://packages.ubuntu.com/search?keywords=audacity&searchon=names&suite=all&section=all">packages.ubuntu</a> and <a href="http://rpmseek.com/rpm-pl/audacity.html?hl=com&cx=0%3A-%3A0%3A0%3A0%3A0%3A0&qArStr=0&qRtStr=0&qDnStr=109">rpm.seek</a></li>
</ul>
')?></p>

<p><?=_('Otherwise, we recommend either searching the web pages of your distribution for the latest information, or compiling Audacity from <a href="beta_source#recdown">source code</a>.')?></p>

<?php
  include "../include/footer.inc.php";
?>
