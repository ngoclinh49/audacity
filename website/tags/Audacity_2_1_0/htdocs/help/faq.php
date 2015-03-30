<?php
/*
 * Copyright 2003 Dominic Mazzoni
 * Copyright 2005 Matt Brubeck
 * Copyright 2007 - 14 Gale Andrews, Vaughan Johnson
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/3.0/
 */
  require_once "main.inc.php";
  require_once "../latest/versions.inc.php";
  $pageId = "faq";
  $pageTitle = _("Frequently Asked Questions");
  include "../include/header.inc.php";

    echo "<h2>$pageTitle</h2>";

echo "<div class=\"advice\">";
echo "<p>";
echo "<b>";
printf(_('Frequently Asked Questions (FAQ) in English for the current <a href="/download/">Audacity %s</a> version are included in the Manual.'), src_version);
echo "</b></p>";
echo "<p><ul>"; 
echo "<li><a href=\"http://manual.audacityteam.org/o/man/faq.html\">"._('View the FAQ in the online Manual')."</a>.</li>";
echo "<li>"._('In Audacity, click Help > Manual (in web browser), then the "FAQ" link in the "Navigation" box on the left. The Manual is included with the <a href="http://audacity.sourceforge.net/download/windows">Windows .exe</a> or <a href="http://audacity.sourceforge.net/download/mac">Mac OS X .dmg</a> Audacity installers.')."</li></ul></p>";
echo "</div>";

echo "<h3>"._('Other languages')."</h3>";
echo "<p>"._('View the localized FAQ in the online Manual:')."</p>";
echo "<p><ul>";
echo "<li>"._('<a href="http://manual.audacityteam.org/man/FAQ/fr">French</a>')."</li>";
echo "<li>"._('<a href="http://manual.audacityteam.org/man/FAQ/it">Italian</a>')."</li>";
echo "<li>"._('<a href="http://manual.audacityteam.org/man/FAQ/es">Spanish</a>')."</li></ul></p>";

echo "<p>";
printf(_('<b>Note: </b>For the above locales, the translation of the remainder of the Manual is not yet fully complete. The content of the translated Manual relates to development versions of Audacity and so may not relate to the current Audacity %s version.'), src_version);
echo "</p>";

echo "<p>"._('For all other languages, please check <a href="faq_i18n">Older Frequently Asked Questions for international help</a>, which remain largely valid for Audacity 2.0.x. The Manual will be updated for other languages in due course. Volunteers to help with <a href="http://manual.audacityteam.org/man/Help:Translating">translating the Manual<a/> are welcomed.')."</p>";

echo "<h3>"._('Still need help?')."</h3>";

echo "<p>"._('If the Frequently Asked Questions don\'t resolve your query, see the 
<a href="documentation">documentation</a>, or <a href="../contact/">
contact us</a> for help.')."</p>";


  include "../include/footer.inc.php";
?>