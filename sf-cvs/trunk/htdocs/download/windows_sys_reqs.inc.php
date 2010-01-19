<?php
/*
 * Copyright 2007 onwards by Vaughan Johnnson
 * This file is licensed under a Creative Commons license:
 * http://creativecommons.org/licenses/by/2.0/
 */
?>

<p>
  <?=_('The values in the "Recommended RAM/processor speed" column below are for tasks like recording for an hour, or editing three 20 minute tracks simultaneously. The values in the "Minimum RAM/processor speed" column will be fine for smaller/shorter tasks, especially if unnecessary programs are closed.')?>
</p>

<table class="winsysreq">
  <tr>
    <th><?=_("Windows version")?></th>
    <th><?=_("Recommended RAM/<br>processor speed")?></th>
    <th><?=_("Minimum RAM/<br>processor speed")?></th>
  </tr>
  <tr>
    <td class="version">Windows 98, ME</td>
    <td class="ramspeed">128 MB / 500 MHz</td>
    <td class="ramspeed">64 MB / 300 MHz</td>
  </tr>
  <tr>
    <td>Windows 2000, XP</td>
    <td>512 MB/1 GHz</td>
    <td>128 MB/300 MHz</td>
  </tr>
  <tr>
    <td>Windows Vista Home Basic</td>
    <td>2 GB / 1 GHz</td>
    <td>512 MB / 1 GHz</td>
  </tr>
  <tr>
    <td>Windows Vista Home Premium/<br>Business/Ultimate</td>
    <td>4 GB / 2 GHz</td>
    <td>1 GB / 1 GHz</td>
  </tr>
</table>

<p>&nbsp;</p>
<p>
  <?=_("Generally, Audacity works best on computers meeting more than the minimum requirements in the table above. Where Audacity is to be used for lengthy multi-track projects, we recommend using Windows 2000, XP or Vista running on machines of substantially higher specification than the minimum stated above.")?>
</p>
