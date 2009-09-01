<?php
/*
 * Copyright 2004 Matt Brubeck
 * 2008-9 Gale Andrews, Vaughan Johnson
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/3.0/
 */
  require_once "main.inc.php";
  $pageId = "";
  $pageTitle = _("Download");
  include "../include/header.inc.php";

  include "../latest/versions.inc.php";
  include "../beta/versions.inc.php";
  $stable_version = stable_version;
  $beta_version = beta_version;
?>

<h2><?=$pageTitle?></h2>

<p><?=_("Select your operating system to download the latest version of the free Audacity sound editor.")?></p>

<div class="download_container">
  <div class="downloads" id="stable_downloads">
    <div class="downloads_header">
      <h3><?=_("Stable: 1.2 - for all users")?></h3>
      <p><?=_("Latest versions:")?></p>
    </div>
    <div>
      <h3><a href="windows"><img alt="Windows" src="../images/new_win.png">
      <?=_("Windows")?></a><span class="downloads_smaller"><?=_("&nbsp;&nbsp;1.2.6")?></span></h3>   
  
      <h3><a href="mac"><img alt="Mac" src="../images/mac.png">
      <?=_("Mac OS X")?></a><br><img src="../images/download_spacer.png"><span class="downloads_smaller"><?=_("&nbsp;1.2.5 (Intel) or 1.2.6 (PPC)")?></span></h3>

      <h3><a href="linux"><img alt="Linux" src="../images/unix.png">
      <?=_("Linux/Unix")?></a><br><img src="../images/download_spacer.png"><span class="downloads_smaller"><?=_("&nbsp;1.2.6 (in source code)")?></span></h3>
  </div>
</div>
  <div class="downloads" id="beta_downloads">
    <div class="downloads_header">
      <h3><?=_("Beta: 1.3 - for advanced users")?></h3>
      <p><?=_("Latest versions:")?></p>
    </div>
    <div>
      <h3><a href="beta_windows"><img alt="Windows" src="../images/new_win.png">
      <?=_("Windows")?></a><span class="downloads_smaller"><?=_("&nbsp;&nbsp;1.3.9")?></span></h3>
  
      <h3><a href="beta_mac"><img alt="Mac" src="../images/mac.png">
      <?=_("Mac OS X")?></a><br><img src="../images/download_spacer.png"><span class="downloads_smaller"><?=_("&nbsp;1.3.9 (Universal Binary)")?></span></h3>
  
      <h3><a href="beta_linux"><img alt="Linux" src="../images/unix.png">
      <?=_("Linux/Unix")?></a><br><img src="../images/download_spacer.png"><span class="downloads_smaller"><?=_("&nbsp;1.3.9 (in source code)")?></span></h3>
  </div>
</div>

<div class="under_downloads">

<h3><?=_("Legacy Downloads")?></h3>
 <p><?=_("For users of <b>Mac OS 9</b>, a legacy 1.0.0 version of Audacity is also available on the ")?><a href="mac"><?=_("Mac downloads page.")?></a></p>

<h3><?=_("Further information")?></h3>
   <ul>
   <li><p><?=_('You can also obtain Audacity as a <a href="buy">CD image file or purchasable CD</a>.')?></p></li>
   <li><p><?=_('If you wish to compile Audacity yourself, please download the <a href="source">source code</a>.')?></p></li>
   <li><p><?=_('You may copy, distribute, modify and/or resell Audacity, under the terms of the  <a href="../about/license">GNU GPL</a>.')?></p></li>
  </ul>

<p>&nbsp;</p>
</div>


<?php
  include "../include/footer.inc.php";
?>
