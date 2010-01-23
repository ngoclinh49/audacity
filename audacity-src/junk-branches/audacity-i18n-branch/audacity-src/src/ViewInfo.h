/**********************************************************************

  Audacity: A Digital Audio Editor

  ViewInfo.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_VIEWINFO__
#define __AUDACITY_VIEWINFO__

struct ViewInfo {

   // Current selection

   double sel0;
   double sel1;

   // Scroll info

   int vpos;                    // vertical scroll pos

   double h;                    // h pos in secs
   double screen;               // screen width in secs
   double total;                // total width in secs
   double zoom;                 // pixels per second
   double lastZoom;

   // Actual scroll bar positions, in pixels
   int sbarH;
   int sbarScreen;
   int sbarTotal;

   int scrollStep;
};

#endif
