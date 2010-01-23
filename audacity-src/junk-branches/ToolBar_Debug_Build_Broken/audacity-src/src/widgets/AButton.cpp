/**********************************************************************

  Audacity: A Digital Audio Editor

  AButton.cpp

  Dominic Mazzoni

  This is a custom button class for Audacity.  The main feature it
  supports that a wxButton does not is mouseovers.  It uses an image
  for all of its states: up, over, down, and disabled, allowing any
  sort of customization you want.  Currently it does not support
  transparency effects, so the images must be rectangular and
  opaque.

**********************************************************************/

#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/image.h>

#include "AButton.h"

BEGIN_EVENT_TABLE(AButton, wxWindow)
    EVT_MOUSE_EVENTS(AButton::OnMouseEvent)
    EVT_PAINT(AButton::OnPaint)
END_EVENT_TABLE()

AButton::AButton(wxWindow * parent, wxWindowID id,
                 const wxPoint & pos,
                 const wxSize & size,
                 char **upXPM,
                 char **overXPM,
                 char **downXPM,
                 char **disXPM):
   wxWindow(parent, id, pos, size)
{
   mWasShiftDown = false;
   mButtonIsDown = false;
   mButtonState = AButtonUp;
   mIsClicking = false;
   mEnabled = true;

   mBitmap[0] = new wxBitmap((const char **) upXPM);
   mBitmap[1] = new wxBitmap((const char **) overXPM);
   mBitmap[2] = new wxBitmap((const char **) downXPM);
   mBitmap[3] = new wxBitmap((const char **) disXPM);

   GetSize(&mWidth, &mHeight);
}

AButton::AButton(wxWindow * parent, wxWindowID id,
                const wxPoint & pos,
                const wxSize & size,
                wxImage *up,
                wxImage *over,
                wxImage *down,
                wxImage *dis):
   wxWindow(parent, id, pos, size)
{
   mWasShiftDown = false;
   mButtonIsDown = false;
   mButtonState = AButtonUp;
   mIsClicking = false;
   mEnabled = true;
   
   mBitmap[0] = new wxBitmap(up->ConvertToBitmap());
   mBitmap[1] = new wxBitmap(over->ConvertToBitmap());
   mBitmap[2] = new wxBitmap(down->ConvertToBitmap());
   mBitmap[3] = new wxBitmap(dis->ConvertToBitmap());
   
   GetSize(&mWidth, &mHeight);
}

AButton::~AButton()
{
   delete mBitmap[0];
   delete mBitmap[1];
   delete mBitmap[2];
   delete mBitmap[3];
}

void AButton::OnPaint(wxPaintEvent & event)
{
   wxPaintDC dc(this);
#ifdef __WXMAC__
   dc.DrawBitmap(*mBitmap[mButtonState], 0, 0);
#else
   wxMemoryDC memDC;
   memDC.SelectObject(*mBitmap[mButtonState]);
   dc.Blit(0, 0, mWidth, mHeight, &memDC, 0, 0, wxCOPY, FALSE);
#endif
}

void AButton::OnMouseEvent(wxMouseEvent & event)
{
   if (mButtonIsDown || !mEnabled)
      return;

   if (event.ButtonUp()) {
      mIsClicking = false;

      //ReleaseMouse();

      if (event.m_x >= 0 && event.m_y >= 0 &&
          event.m_x < mWidth && event.m_y < mHeight) {
         mButtonState = AButtonDown;
         mButtonIsDown = true;

         mWasShiftDown = event.ShiftDown();

         wxCommandEvent *e =
             new wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
         GetParent()->ProcessEvent(*e);
         delete e;
      }

      this->Refresh(false);
      return;
   }

   if (event.ButtonDown()) {
      mIsClicking = true;
      //CaptureMouse();
   }

   if (mIsClicking) {
      if (event.m_x >= 0 && event.m_y >= 0 &&
          event.m_x < mWidth && event.m_y < mHeight) {
         mButtonState = AButtonDown;
      } else
         mButtonState = AButtonUp;
      this->Refresh(false);
   } else {
      if (event.Entering()) {
         mButtonState = AButtonOver;
         this->Refresh(false);
      }

      if (event.Leaving()) {
         mButtonState = AButtonUp;
         this->Refresh(false);
      }
   }
}

bool AButton::WasShiftDown()
{
   return mWasShiftDown;
}

void AButton::Enable()
{
   mEnabled = true;
   if (mButtonIsDown)
      mButtonState = AButtonDown;
   else
      mButtonState = AButtonUp;
   this->Refresh(false);
}

void AButton::Disable()
{
   mEnabled = false;
   mButtonState = AButtonDis;
   this->Refresh(false);
}

void AButton::PushDown()
{
   mButtonIsDown = true;
   mButtonState = AButtonDown;
   this->Refresh(false);
}

void AButton::PopUp()
{
   mButtonIsDown = false;
   mButtonState = AButtonUp;
   this->Refresh(false);
}
