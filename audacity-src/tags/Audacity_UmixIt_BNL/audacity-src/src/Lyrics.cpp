/**********************************************************************

  Audacity: A Digital Audio Editor

  Lyrics.cpp

  Dominic Mazzoni
  Vaughan Johnson
  
**********************************************************************/

#include <math.h>

#include <wx/defs.h>
#include <wx/dcmemory.h>
#include <wx/mimetype.h>

#include "Lyrics.h"
#include "Internat.h"
#include "Project.h" // for GetActiveProject
#include "Branding.h"

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(SyllableArray);

void LinkingHtmlWindow::OnLinkClicked(const wxHtmlLinkInfo& link) 
{
#ifdef __WXMAC__
   wxString openCmd = "open " + link.GetHref();
   ::wxExecute(openCmd);
#else
   wxFileType* pFileType = wxTheMimeTypesManager->GetFileTypeFromExtension(wxT(".htm"));
   if (pFileType == NULL) return;
   wxString openCmd = pFileType->GetOpenCommand(link.GetHref());
   ::wxExecute(openCmd);
#endif
}

BEGIN_EVENT_TABLE(Lyrics, wxPanel)
   EVT_CHAR(Lyrics::OnKeyEvent)
   EVT_PAINT(Lyrics::OnPaint)
   EVT_SIZE(Lyrics::OnSize)
END_EVENT_TABLE()

IMPLEMENT_CLASS(Lyrics, wxPanel)

#define DEFAULT_BRANDING_HEIGHT 105 //vvv UmixIt: Need to figure it out from logo height, etc. This works for UmixIt.

Lyrics::Lyrics(wxWindow* parent, wxWindowID id,
               const wxPoint& pos /*= wxDefaultPosition*/,
               const wxSize& size /*= wxDefaultSize*/):
   wxPanel(parent, id, pos, size),
   mWidth(size.x), mHeight(size.y)
{
   // branding
   //vvv Should change this to just a wxStaticBitmap, but it's working, and no longer showing lyrics.
   AudacityProject* pProject = GetActiveProject();
   Branding* pBranding = pProject->GetBranding();
   wxString strBranding = "";
   if (pBranding) {
      wxString strBrandName = pBranding->GetBrandName();
      wxString strURL = pBranding->GetBrandURL();

      wxString strIMGtag = "";
      wxString strLogoFilename = pBranding->GetBrandLogoFilename();
      if (!strLogoFilename.IsEmpty()){
         wxString strLogoPathname = 
            pProject->GetDirManager()->GetProjectDataDir() + wxFILE_SEP_PATH + strLogoFilename;
         if (::wxFileExists(FILENAME(strLogoPathname))) 
            strIMGtag = "<img src=\"" + FILENAME(strLogoPathname) + "\" alt=\"" + strLogoFilename + "\"><br>";
      }

      if (strURL.IsEmpty()) {
         if (strIMGtag.IsEmpty()) {
            if (!strBrandName.IsEmpty()) 
               strBranding = "<center><h1>" + strBrandName + "</h1></center>\n";
         } else {
            strBranding = "<center>" + strIMGtag + "</center>\n";
         }
      } else {
         if (strIMGtag.IsEmpty()) {
            if (!strBrandName.IsEmpty()) 
               strBranding = 
                  "<center><h1>" + strBrandName + 
                  "</h1><br><a href=\"" + FILENAME(strURL) + "\">" + strURL + "</a></center>\n";
         } else {
            strBranding = 
               "<center><a href=\"" + FILENAME(strURL) + "\">" + strIMGtag + strURL + "</a></center>\n";
         }
      }
   }
   if (!strBranding.IsEmpty()) {
      mBrandingPanel = new LinkingHtmlWindow(this);
      mBrandingPanel->SetPage(strBranding);
      mBrandingHeight = DEFAULT_BRANDING_HEIGHT;
   } else {
      mBrandingHeight = 0;
   }

   Clear();
   Finish(0.0);
   mT = 0.0;

  #ifdef __WXMAC__
   wxSizeEvent dummyEvent;
   OnSize(dummyEvent);
  #endif
}

Lyrics::~Lyrics()
{
}

void Lyrics::Clear()
{
   mSyllables.Clear();
   mText = "";

   // Add two dummy syllables at the beginning
   mSyllables.Add(Syllable());
   mSyllables[0].t = -2.0;
   mSyllables.Add(Syllable());
   mSyllables[1].t = -1.0;
}

void Lyrics::Add(double t, wxString syllable)
{
   int i = mSyllables.GetCount();

   if (mSyllables[i-1].t == t) {
      // We can't have two syllables with the same time, so append
      // this to the end of the previous one if they're at the
      // same time.
      mSyllables[i-1].text += syllable;
      mSyllables[i-1].textWithSpace += syllable;
      mSyllables[i-1].char1 += syllable.Length();
      return;
   }

   mSyllables.Add(Syllable());
   mSyllables[i].t = t;
   mSyllables[i].text = syllable;

   // Put a space between syllables unless the previous one
   // ended in a hyphen
   if (i > 0 && mSyllables[i-1].text.Length() > 0 &&
       mSyllables[i-1].text.Right(1)!="-") {
      mSyllables[i].textWithSpace = " " + syllable;
      mText += " ";
   }
   else {
      mSyllables[i].textWithSpace = syllable;
   }

   mSyllables[i].char0 = mText.Length();
   mText += syllable;
   mSyllables[i].char1 = mText.Length();
}

void Lyrics::Finish(double finalT)
{
   // Add 3 dummy syllables at the end
   int i = mSyllables.GetCount();
   mSyllables.Add(Syllable());
   mSyllables[i].t = finalT + 1;
   mSyllables.Add(Syllable());
   mSyllables[i+1].t = finalT + 2;
   mSyllables.Add(Syllable());
   mSyllables[i+2].t = finalT + 3;

   // Mark measurements as invalid
   mMeasurementsDone = false;
   mCurrentSyllable = 0;
}


#define DEFAULT_FONT_POINTSIZE 28
int gKaraokeFontPointSize = DEFAULT_FONT_POINTSIZE;

void Lyrics::SetFont(wxDC *dc)
{
   dc->SetFont(wxFont(gKaraokeFontPointSize, wxSWISS, wxNORMAL, wxNORMAL));
}

void Lyrics::Measure(wxDC *dc)
{
   SetFont(dc);

   int width = 0, height = 0;
   int x = 0;
   unsigned int i;
   for(i=0; i<mSyllables.GetCount(); i++) {
      if (i < 2 || i >= mSyllables.GetCount()-2) {
         dc->GetTextExtent("DUMMY", &width, &height); // Get the correct height even if we're at i=0.
         width = 0;
      }
      else {
         dc->GetTextExtent(mSyllables[i].textWithSpace, &width, &height);
      }

      // Add some space between words; the space is normally small but
      // when there's a long pause relative to the previous word, insert
      // extra space.
      int extraSpacing;
      if (i >= 2 && i < mSyllables.GetCount()-2) {
         double deltaThis = mSyllables[i+1].t - mSyllables[i].t;
         double deltaPrev = mSyllables[i].t - mSyllables[i-1].t;

         double ratio;
         if (deltaPrev > 0.0)
            ratio = deltaThis / deltaPrev;
         else
            ratio = deltaThis;

         if (ratio > 2.0)
            extraSpacing = 15 + (int)(15.0 * ratio);
         else
            extraSpacing = 15;
      }
      else
         extraSpacing = 20;
      
      mSyllables[i].width = width + extraSpacing;
      mSyllables[i].leftX = x;
      mSyllables[i].x = x + width/2;
      x += mSyllables[i].width;
   }
   
   mTextHeight = height;

   mMeasurementsDone = true;
}

// Binary-search for the syllable with the largest time not greater than t
int Lyrics::FindSyllable(double t)
{
   int i1, i2;

   i1 = 0;
   i2 = mSyllables.GetCount();
   while (i2 > i1+1) {
      int pmid = (i1+i2)/2;
      if (mSyllables[pmid].t > t)
         i2 = pmid;
      else
         i1 = pmid;
   }

   if (i1 < 2)
      i1 = 2;
   if (i1 > mSyllables.GetCount() - 3)
      i1 = mSyllables.GetCount() - 3;

   return i1;
}

// Given the current time t, returns the x/y position of the scrolling
// karaoke display.  For some syllable i, when t==mSyllables[i].t,
// it will return mSyllables[i].x for outX and 0 for outY.
// In-between words, outX is interpolated using smooth acceleration
// between the two neighboring words, and y is a positive number indicating
// the bouncing ball height
void Lyrics::GetKaraokePosition(double t,
                                int *outX, double *outY)
{
   *outX = 0;
   *outY = 0;

   if (t < mSyllables[2].t || t > mSyllables[mSyllables.GetCount()-3].t)
      return;

   int i0, i1, i2, i3;
   int x0, x1, x2, x3;
   double t0, t1, t2, t3;
   i1 = FindSyllable(t);
   i2 = i1 + 1;

   // Because we've padded the syllables with two dummies at the beginning
   // and end, we know that i0...i3 will always exist.  Also, we've made
   // sure that we don't ever have two of the same time, so t2>t1 strictly.
   //
   //                          t
   //                          \/
   // time:  t0           t1                 t2              t3
   // pos:   x0           x1                 x2              x3
   // index: i0           i1                 i2              i3
   // vel:               vel1               vel2

   i0 = i1 - 1;
   i3 = i2 + 1;

   x0 = mSyllables[i0].x;
   x1 = mSyllables[i1].x;
   x2 = mSyllables[i2].x;
   x3 = mSyllables[i3].x;

   t0 = mSyllables[i0].t;
   t1 = mSyllables[i1].t;
   t2 = mSyllables[i2].t;
   t3 = mSyllables[i3].t;

   double linear_vel0 = (x1 - x0) / (t1 - t0);
   double linear_vel1 = (x2 - x1) / (t2 - t1);
   double linear_vel2 = (x3 - x2) / (t3 - t2);

   // average velocities
   double v1 = (linear_vel0 + linear_vel1) / 2;
   double v2 = (linear_vel1 + linear_vel2) / 2;

   // Solve a cubic equation f(t) = at^3 + bt^2 + ct + d
   // which gives the position x as a function of
   // (t - t1), by constraining f(0), f'(0), f(t2-t1), f'(t2-t1)
   double delta_t = t2 - t1;
   double delta_x = x2 - x1;
   v1 *= delta_t;
   v2 *= delta_t;
   double a = v1 + v2 - 2*delta_x;
   double b = 3*delta_x - 2*v1 - v2;
   double c = v1;
   double d = x1;

   t = (t - t1) / (t2 - t1);
   double xx = a*t*t*t + b*t*t + c*t + d;

   // Unfortunately sometimes our cubic goes backwards.  This is a quick
   // hack to stop that from happening.
   if (xx < x1)
      xx = x1;

   *outX = (int)xx;

   // The y position is a simple cosine curve; the max height is a
   // function of the time.
   double height = t2 - t1 > 4.0? 1.0: sqrt((t2-t1)/4.0);
   #ifdef __WXMSW__ 
      #define M_PI 3.1415926535897932385 
   #endif
   *outY = height * sin(M_PI * t);
}

void Lyrics::Update(double t, bool bForce /* = false */)
{
   mT = t;

   wxRect karaokeRect(0, 0, mWidth, mKaraokeHeight);
   this->Refresh(false, &karaokeRect);

   int i = FindSyllable(t);
   if (!bForce && (i == mCurrentSyllable))
      return;

   // mBrandingPanel->Refresh(false);

   mCurrentSyllable = i;   
}

void Lyrics::OnKeyEvent(wxKeyEvent & event)
{
   GetActiveProject()->HandleKeyDown(event);
}

void Lyrics::OnPaint(wxPaintEvent &evt)
{
   wxPaintDC dc(this);

   if (!mMeasurementsDone)
      Measure(&dc);

  #ifdef __WXMAC__
   // Mac OS X automatically double-buffers the screen for you,
   // so our bitmap is unneccessary
   HandlePaint(dc);
  #else
   wxBitmap bitmap(mWidth, mKaraokeHeight);
   wxMemoryDC memDC;
   memDC.SelectObject(bitmap);
   HandlePaint(memDC);
   dc.Blit(0, 0, mWidth, mKaraokeHeight, &memDC, 0, 0, wxCOPY, FALSE);
  #endif
}

void Lyrics::OnSize(wxSizeEvent &evt)
{
   GetClientSize(&mWidth, &mHeight);
   //vvv UmixIt: Remove lower lyrics display for BNL release.
   int nTenthOfHeight = mHeight / 10;
   mBrandingHeight = (DEFAULT_BRANDING_HEIGHT > nTenthOfHeight) ? DEFAULT_BRANDING_HEIGHT : nTenthOfHeight;
   mKaraokeHeight = (mHeight - mBrandingHeight); // gray bar? * 9 / 10; // mHeight * 3 / 4; // mKaraokeHeight = mHeight / 2;
   // mBrandingPanel->SetSize(0, mKaraokeHeight, mWidth, mHeight - mKaraokeHeight);
   mBrandingPanel->SetSize(0, mKaraokeHeight, // gray bar? (mHeight - mBrandingHeight), 
                           mWidth, mBrandingHeight);
   
   gKaraokeFontPointSize = (int)((float)(DEFAULT_FONT_POINTSIZE * mHeight) / (float)LYRICS_DEFAULT_HEIGHT);
   // Usually don't get the size window we want, usually less than 
   // LYRICS_DEFAULT_HEIGHT, so bump it a little.
   gKaraokeFontPointSize += 2;
   this->Update(mT, true);

   wxPaintEvent ignore;
   mMeasurementsDone = false;
   this->OnPaint(ignore);
}

void Lyrics::HandlePaint(wxDC &dc)
{
   dc.SetBrush(*wxWHITE_BRUSH);
   dc.DrawRectangle(0, 0, mWidth, mKaraokeHeight);

   int ctr = mWidth / 2;
   int x;
   double y;
   GetKaraokePosition(mT, &x, &y);
   
   dc.SetTextForeground(wxColour(238, 0, 102));
   bool changedColor = false;

   SetFont(&dc);
   unsigned int i;
   wxCoord yTextTop = mKaraokeHeight - mTextHeight - 4;
   for(i=0; i<mSyllables.GetCount(); i++) {
      if (mSyllables[i].x + mSyllables[i].width < (x - ctr))
         continue;
      if (mSyllables[i].x > x + ctr)
         continue;

      if (!changedColor && mSyllables[i].x >= x) {
         dc.SetTextForeground(*wxBLACK);
         changedColor = true;
      }

      wxString text = mSyllables[i].text;
      if (text.Length() > 0 && text.Right(1)=="_") {
         text = text.Left(text.Length() - 1);
      }

      dc.DrawText(text,
                  mSyllables[i].leftX + ctr - x,
                  yTextTop);
   }

   int ballRadius = (int)(mTextHeight / 3.0);
   int bounceTop = ballRadius * 2;
   int bounceHeight = yTextTop - bounceTop;
   int yi = (int)(yTextTop - 4 - (y * bounceHeight));

   if (mT >= 0.0) {
      wxRect ball(ctr - ballRadius, yi - ballRadius, 2 * ballRadius, 2 * ballRadius);
      dc.SetBrush(wxBrush(wxColour(238, 0, 102), wxSOLID));
      dc.DrawEllipse(ball);
   }
}

// Indentation settings for Vim and Emacs.
// Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// 
