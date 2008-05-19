/**********************************************************************

  Audacity: A Digital Audio Editor

  ASlider.cpp

  Dominic Mazzoni

*******************************************************************//**

\class ASlider
\brief ASlider is a custom slider, allowing for a slicker look and 
feel.

It allows you to use images for the slider background and 
the thumb.

*//****************************************************************//**

\class LWSlider
\brief Lightweight version of ASlider.  In other words it does not 
have a window permanaently associated with it.

*//****************************************************************//**

\class SliderDialog
\brief Pop up dialog used with an LWSlider.

*//****************************************************************//**

\class TipPanel
\brief A wxPanel or a wxPopupWindow used to give the numerical value
of an LWSlider or ASlider.

*//*******************************************************************/

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/window.h>
#endif


#include "../Audacity.h"

#include <math.h>

#include <wx/defs.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/tooltip.h>
#include <wx/debug.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/sizer.h>

#if defined(__WXMSW__) && !defined(__CYGWIN__)
#define USE_POPUPWIN 1
#endif

#if USE_POPUPWIN
#include <wx/popupwin.h>
#endif

#if defined(__WXMAC__)
#include <wx/sysopt.h>
#endif

#include "ASlider.h"
#include "../Experimental.h"

#include "../AColor.h"
#include "../ImageManipulation.h"
#include "../Project.h"
#include "../ShuttleGui.h"

#include "../Theme.h"
#include "../AllThemeResources.h"

//#include "../../images/SliderThumb.xpm"
//#include "../../images/SliderThumbAlpha.xpm"

#if defined __WXMSW__
const int sliderFontSize = 10;
#else
const int sliderFontSize = 12;
#endif

//
// TipPanel
//

#if USE_POPUPWIN
class TipPanel : public wxPopupWindow
#elif defined(__WXMAC__)
// On the Mac, we use a wxFrame as a wxPanel will appear to fall behind
// whatever window it is hovering above if that window is refreshed.  A frame
// does not have this problem.  One unfortunate side effect is that a shadow
// now appears below the tip panel.
class TipPanel : public wxFrame
#else
class TipPanel : public wxPanel
#endif
{
 public:
   TipPanel(wxWindow * parent, wxWindowID id,
            wxString label,
            const wxPoint &pos);
   
   void SetPos(const wxPoint &pos);

   void OnPaint(wxPaintEvent & event);

#if defined(__WXMAC__)
   bool Show(bool show);
#endif

   wxString label;
   wxString origLabel;

   wxWindow *mParent;

   DECLARE_EVENT_TABLE()
};

#if USE_POPUPWIN

BEGIN_EVENT_TABLE(TipPanel, wxPopupWindow)
   EVT_PAINT(TipPanel::OnPaint)
END_EVENT_TABLE()

TipPanel::TipPanel(wxWindow *parent, wxWindowID id,
                   wxString label, const wxPoint &pos):
   wxPopupWindow(parent)
{
   this->label = label;
   this->origLabel = label;
   mParent = parent;
   SetPos(pos);
}

void TipPanel::SetPos(const wxPoint& pos)
{
   int x = pos.x;
   int y = pos.y;

   if (mParent)
      mParent->ClientToScreen(&x,&y);

   wxClientDC dc(this);
   wxFont labelFont(sliderFontSize, wxSWISS, wxNORMAL, wxNORMAL);
   dc.SetFont(labelFont);
   int width, height;
   dc.GetTextExtent(origLabel, &width, &height);
   height += 4;
   SetSize(x - width/2, y, width, height);
}

#else

#if defined(__WXMAC__)
BEGIN_EVENT_TABLE(TipPanel, wxFrame)
   EVT_PAINT(TipPanel::OnPaint)
END_EVENT_TABLE()

TipPanel::TipPanel(wxWindow *parent, wxWindowID id,
                   wxString label,
                   const wxPoint &pos):
   wxFrame(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize,
           wxNO_BORDER | wxFRAME_NO_TASKBAR)
#else
BEGIN_EVENT_TABLE(TipPanel, wxPanel)
   EVT_PAINT(TipPanel::OnPaint)
END_EVENT_TABLE()

TipPanel::TipPanel(wxWindow *parent, wxWindowID id,
                   wxString label,
                   const wxPoint &pos):
   wxPanel(parent, id)
#endif

{
   this->label = label;
   this->origLabel = label;
   SetPos(pos);
}

void TipPanel::SetPos(const wxPoint& pos)
{
   wxClientDC dc(this);
   wxFont labelFont(sliderFontSize, wxSWISS, wxNORMAL, wxNORMAL);
   dc.SetFont(labelFont);
   int width, height;
   dc.GetTextExtent(origLabel, &width, &height);
   width += 4;
   height += 4;
   int left = pos.x - width/2;
   if (left < 0)
      left = 0;
   SetSize(left, pos.y, width, height);
   Raise();
}

#if defined(__WXMAC__)
bool TipPanel::Show(bool show)
{
   // Save original transition
   int trans = wxSystemOptions::GetOptionInt( wxMAC_WINDOW_PLAIN_TRANSITION );

   // Disable window animation
   wxSystemOptions::SetOption( wxMAC_WINDOW_PLAIN_TRANSITION, 1 );

   // Show/Hide the window
   bool shown = wxFrame::Show(show);

   // Disable window animation
   wxSystemOptions::SetOption( wxMAC_WINDOW_PLAIN_TRANSITION, trans );

   return shown;
}
#endif

#endif

void TipPanel::OnPaint(wxPaintEvent& event)
{
   wxPaintDC dc(this);
   int width, height, textWidth, textHeight;

   wxFont labelFont(sliderFontSize, wxSWISS, wxNORMAL, wxNORMAL);
   dc.SetFont(labelFont);
   GetClientSize(&width, &height);
#if defined(__WXMAC__)
   dc.SetPen(AColor::tooltipBrush.GetColour());
#else
   dc.SetPen(*wxBLACK_PEN);
#endif
   dc.SetBrush(AColor::tooltipBrush);
   dc.DrawRectangle(0, 0, width, height);
   dc.GetTextExtent(label, &textWidth, &textHeight);
   dc.DrawText(label, (width-textWidth)/2, (height-textHeight)/2);
}

//
// SliderDialog
//

BEGIN_EVENT_TABLE(SliderDialog, wxDialog)
   EVT_SLIDER(wxID_ANY,SliderDialog::OnSlider)
END_EVENT_TABLE();

SliderDialog::SliderDialog(wxWindow * parent, wxWindowID id,
                           const wxString & title, 
                           wxPoint position,
                           wxSize size,
                           int style, 
                           float value):
   wxDialog(parent,id,title,position),
   mStyle(style)
{
   //Use a vertical sizer
   wxBoxSizer * vs = new wxBoxSizer(wxVERTICAL);

   //Add the text
   mTextCtrl = new wxTextCtrl(this,
                              SLIDER_DIALOG_TEXTCTRL,
                              wxT(""),
                              wxDefaultPosition,
                              wxDefaultSize,
                              0,
                              wxTextValidator(wxFILTER_NUMERIC));
   vs->Add(mTextCtrl,0,wxEXPAND|wxALL,5);

   //Add a slider 
   mSlider = new ASlider(this,wxID_ANY,title,wxDefaultPosition,size,style,false);
   vs->Add(mSlider, 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );
   
   //Create buttons 
   vs->Add(CreateStdButtonSizer(this, eCancelButton|eOkButton), 0, wxEXPAND);
      
   //lay it out
   SetSizerAndFit(vs);

   mTextCtrl->SetSelection(-1,-1);
   mTextCtrl->SetFocus();

   mSlider->Set(value);
}

SliderDialog::~SliderDialog()
{
}

bool SliderDialog::TransferDataToWindow()
{
   mTextCtrl->SetValue(wxString::Format(wxT("%g"), mSlider->Get(false)));

   return true;
}

bool SliderDialog::TransferDataFromWindow()
{
   double value;

   mTextCtrl->GetValue().ToDouble(&value);
   if (mStyle == DB_SLIDER)
      value = pow(10.0, value / 20.0);
   mSlider->Set(value);

   return true;
}

void SliderDialog::OnSlider(wxCommandEvent & event)
{
   TransferDataToWindow();

   event.Skip(false);
}

float SliderDialog::Get()
{
   return mSlider->Get(false);
}

//
// LWSlider
//

// Construct customizable slider
LWSlider::LWSlider(wxWindow * parent,
         wxString name,
         const wxPoint &pos,
         const wxSize &size,
         float minValue,
         float maxValue,
         float stepValue,
         bool canUseShift,
         int style,
         bool heavyweight /* = false */,
         bool popup /* = true */
         )
{
   Init(parent, name, pos, size, minValue, maxValue,
        stepValue, canUseShift, style, heavyweight, popup, 1.0);
}

// Construct predefined slider
LWSlider::LWSlider(wxWindow *parent,
                   wxString name,
                   const wxPoint &pos,
                   const wxSize &size,
                   int style,
                   bool heavyweight /* = false */,
                   bool popup /* = true */)
{
   wxString leftLabel, rightLabel;
   float minValue, maxValue, stepValue;
   float speed = 1.0;

   switch(style)
   {
   case PAN_SLIDER:
      minValue = -1.0f;
      maxValue = +1.0f;
      stepValue = 0.1f;
      break;
   case DB_SLIDER:
      minValue = -36.0f;
      maxValue = 36.0f;
      stepValue = 1.0f;
      speed = 0.5;
      break;
   case FRAC_SLIDER:
      minValue = 0.0f;
      maxValue = 1.0f;
      stepValue = STEP_CONTINUOUS;
      break;
   case SPEED_SLIDER:
      minValue = 0.01f;
      maxValue = 3.0f;
      stepValue = STEP_CONTINUOUS;
      break;

   default:
      minValue = 0.0f;
      maxValue = 1.0f;
      stepValue = 0.0f;
      wxASSERT(false); // undefined style
   }

   Init(parent, name, pos, size, minValue, maxValue, stepValue,
        true, style, heavyweight, popup, speed);
}

void LWSlider::Init(wxWindow * parent,
                    wxString name,
                    const wxPoint &pos,
                    const wxSize &size,
                    float minValue,
                    float maxValue,
                    float stepValue,
                    bool canUseShift,
                    int style,
                    bool heavyweight,
                    bool popup,
                    float speed
     )
{
   mName = name;
   mStyle = style;
   mIsDragging = false;
   mWidth = size.x;
   mHeight = size.y;
   mParent = parent;
   mHW = heavyweight;
   mPopup = popup;
   mSpeed = speed;
   mID = wxID_ANY;
   mMinValue = minValue;
   mMaxValue = maxValue;
   mStepValue = stepValue;
   mCanUseShift = canUseShift;
   mCurrentValue = 0.0f;
   mBitmap = NULL;

   // Get the Thumb bitmap.  Generic version fo rnow...
//#ifdef USE_AQUA
//   mThumbBitmap = &theTheme.Bitmap( bmpMacSliderThumb );
//#else
   mThumbBitmap = &theTheme.Bitmap( bmpSliderThumb );
//#endif

//   mThumbBitmap = new wxBitmap( SliderThumb );
//   mThumbBitmap->SetMask( new wxMask( wxBitmap( SliderThumbAlpha ), *wxBLACK ) );

   Draw();

   mPopWin = NULL;
   Move(pos);
   CreatePopWin();
}

LWSlider::~LWSlider()
{
   delete mBitmap;
//   delete mThumbBitmap;
   delete mPopWin;
}

wxWindowID LWSlider::GetId()
{
   return mID;
}

void LWSlider::SetId(wxWindowID id)
{
   mID = id;
}

wxWindow* LWSlider::GetToolTipParent() const
{
   wxWindow *top = mParent;
   while(top && top->GetParent()) {
      top = top->GetParent();
   }
   
   return top;
}

void LWSlider::CreatePopWin()
{
   if (mPopWin)
      return;

   wxString maxStr = mName + wxT(": 000000");

   if (mStyle == PAN_SLIDER || mStyle == DB_SLIDER || mStyle == SPEED_SLIDER)
      maxStr += wxT("000");

   mPopWin = new TipPanel(GetToolTipParent(), -1, maxStr, wxDefaultPosition);
   mPopWin->Hide();
}

void LWSlider::SetPopWinPosition()
{
   wxPoint pt(mWidth/2 + mLeft, mHeight + mTop + 1);
   pt = mParent->ClientToScreen(pt);

#if !defined(__WXMAC__)
   if (GetToolTipParent())
      pt = GetToolTipParent()->ScreenToClient(pt);
#endif

   if (mPopWin)
      ((TipPanel *)mPopWin)->SetPos(pt);
}

void LWSlider::Move(const wxPoint &newpos)
{
   mLeft = newpos.x;
   mTop = newpos.y;
}

void LWSlider::RecreateTipWin()
{
   delete mPopWin;
   mPopWin = NULL;
   CreatePopWin();
}

void LWSlider::OnPaint(wxDC &dc, bool selected)
{
   //thumbPos should be in pixels
   int thumbPos = ValueToPosition(mCurrentValue);
   int thumbY = mCenterY - (mThumbHeight/2);

#if defined(__WXMSW__)
   if( mHW )
   {
      dc.Clear();
   }
#endif

   dc.DrawBitmap(*mBitmap, mLeft, mTop, true);
   dc.DrawBitmap(*mThumbBitmap, mLeft+thumbPos, mTop+thumbY, true);

   if (mPopWin)
      mPopWin->Refresh();
}

void LWSlider::OnSize( wxSizeEvent & event )
{
   mWidth = event.GetSize().GetX();
   mHeight = event.GetSize().GetY();

   Draw();

   Refresh();
}

void LWSlider::Draw()
{
   if( mBitmap )
   {
      delete mBitmap;
      mBitmap = NULL;
   }

   mCenterY = mHeight - 9;
   mThumbWidth = mThumbBitmap->GetWidth();
   mThumbHeight = mThumbBitmap->GetHeight();

   mLeftX = mThumbWidth/2;
   mRightX = mWidth - mThumbWidth/2 - 1;
   mWidthX = mRightX - mLeftX;

   wxMemoryDC *dc = new wxMemoryDC();
   mBitmap = new wxBitmap(mWidth, mHeight);

   // Set background to an unused color.  This allows for easy
   // mask creation.  And garbage can be displayed if it isn't
   // cleared.
   dc->SelectObject(*mBitmap);

   wxColour TransparentColour = wxColour( 255, 254, 255 ); 
   // DO-THEME Mask colour!!  JC-Aug-2007
   // Needed with experimental theming!
   //  ... because windows blends against this colour.
#ifdef EXPERIMENTAL_THEMING
   TransparentColour = theTheme.Colour( clrTrackInfo );
#endif

   dc->SetBackground( wxBrush( TransparentColour  ) );
   dc->Clear();

   AColor::Dark(dc, false);
   dc->DrawLine(mLeftX, mCenterY+1, mRightX+2, mCenterY+1);

   // Draw +/- or L/R first.  We need to draw these before the tick marks.
   if (mStyle == PAN_SLIDER)
   {
      // sliderFontSize is for the tooltip.
      // we need something smaller here...
      wxFont labelFont(sliderFontSize-3, wxSWISS, wxNORMAL, wxNORMAL);
      dc->SetFont(labelFont);

#ifdef EXPERIMENTAL_THEMING
      dc->SetTextForeground( theTheme.Colour( clrTrackPanelText ));
      // TransparentColour should be same as clrTrackInfo.
      dc->SetTextBackground( theTheme.Colour( clrTrackInfo ) );
      dc->SetBackground( theTheme.Colour( clrTrackInfo ) );
      // HAVE to use solid and not transparent here,
      // otherwise windows will do it's clever font optimisation trick, 
      // but against a default colour of white, which is not OK on a dark
      // background.
      dc->SetBackgroundMode( wxSOLID );
#else
      dc->SetTextForeground( wxColour( 0,0,0) );
      dc->SetTextBackground( wxColour( 255,255,255));
#endif

      /* i18n-hint: One-letter abbreviation for Left, in the Pan slider */
      dc->DrawText(_("L"), mLeftX, 0);

      /* i18n-hint: One-letter abbreviation for Right, in the Pan slider */
      dc->DrawText(_("R"), mRightX-6,0);
   } else
   {
      // draw the '-' and the '+'
#ifdef EXPERIMENTAL_THEMING
      wxPen pen( theTheme.Colour( clrTrackPanelText ));
      dc->SetPen( pen );
#else
      dc->SetPen(*wxBLACK_PEN);
#endif
      dc->DrawLine(mLeftX, mCenterY-10, mLeftX+5, mCenterY-10);
      dc->DrawLine(mRightX-5, mCenterY-10, mRightX, mCenterY-10);
      dc->DrawLine(mRightX-3, mCenterY-12, mRightX-3, mCenterY-7);
   }


   int divs = 10;
   double upp = divs / (double)(mWidthX-1);
   double d = 0;
   int int_d = -1;
   for(int p=0; p<=mWidthX; p++) {
      if (((int)d) > int_d) {
         int_d = (int)d;
         int ht = (int_d==0 || int_d==divs? 5: 3);
         AColor::Light(dc, false);
         dc->DrawLine(mLeftX+p, mCenterY-ht, mLeftX+p, mCenterY);
         AColor::Dark(dc, false);
         dc->DrawLine(mLeftX+p+1, mCenterY-ht+1, mLeftX+p+1, mCenterY);
      }
      d += upp;
   }

   
   // Must preceed creating the mask as that will attempt to
   // select the bitmap into another DC.
   delete dc;

   mBitmap->SetMask( new wxMask( *mBitmap, TransparentColour ) );
}


void LWSlider::FormatPopWin()
{
   wxString label;
   wxString valstr;

   switch(mStyle) {
   case FRAC_SLIDER:
      label.Printf(wxT("%s: %.1f"), mName.c_str(), mCurrentValue);
      break;
     
   case DB_SLIDER:
      valstr.Printf(wxT("%.1f"), mCurrentValue);
      if (valstr.Right(1) == wxT("0"))
         valstr = valstr.Left(valstr.Length() - 2);
      if (mCurrentValue > 0)
         valstr = wxT("+") + valstr;
      
      label.Printf(wxT("%s: %s dB"), mName.c_str(), valstr.c_str());
      break;
   case PAN_SLIDER:
      if (mCurrentValue == 0.0)
         label.Printf(wxT("%s: %s"), mName.c_str(),
                      _("Center"));
      else {
         if (mCurrentValue < 0.0)
            label.Printf(wxT("%s: %.0f%% %s"), mName.c_str(),
                         -mCurrentValue * 100.0f, _("Left"));
         else /* if (val > 0.0) */
            label.Printf(wxT("%s: %.0f%% %s"), mName.c_str(),
                         mCurrentValue * 100.0f, _("Right"));
      }
        
      break;
   case SPEED_SLIDER:
      label.Printf(wxT("%s: %.2fx"), mName.c_str(), mCurrentValue);
      break;
   }

   ((TipPanel *)mPopWin)->label = label;
}

bool LWSlider::ShowDialog()
{
   return DoShowDialog( mParent->ClientToScreen(wxPoint( mLeft, mTop ) ) );
}

bool LWSlider::DoShowDialog(wxPoint pos)
{
   float value;
   bool changed = false;

   SliderDialog * dialog =
      new SliderDialog( mParent,
                        wxID_ANY,
                        mName,
                        pos,
                        wxSize( mWidth, mHeight ),
                        mStyle,
                        Get());

   if( dialog->ShowModal() == wxID_OK )
   {
      value = dialog->Get();
      if( value != mCurrentValue )
      {
         mCurrentValue = value;
         changed = true;
      }
   }

   dialog->Destroy();

   return changed;
}

void LWSlider::OnMouseEvent(wxMouseEvent & event)
{
   if( event.Entering() )
   {
#if wxUSE_TOOLTIPS // Not available in wxX11
      // Display the tooltip in the status bar
      if( mParent->GetToolTip() )
      {
         wxString tip = mParent->GetToolTip()->GetTip();
         GetActiveProject()->TP_DisplayStatusMessage(tip);
         Refresh();
      }
#endif
   }
   else if( event.Leaving() )
   {
      GetActiveProject()->TP_DisplayStatusMessage(wxT(""));
      Refresh();
   }

   float prevValue = mCurrentValue;

   // Figure out the thumb position
   wxRect r;
   r.x = mLeft + ValueToPosition(mCurrentValue);
   r.y = mTop + (mCenterY - (mThumbHeight / 2));
   r.width = mThumbWidth;
   r.height = mThumbHeight;

   wxRect tolerantThumbRect = r;
   tolerantThumbRect.Inflate(3, 3);

   // Should probably use a right click instead/also
   if( event.ButtonDClick() && mPopup )
   {
      //On a double-click, we should pop up a dialog.
      DoShowDialog(mParent->ClientToScreen(wxPoint(event.m_x,event.m_y)));
   }
   else if( event.ButtonDown() )
   {
      mParent->SetFocus();

      // Thumb clicked?
      //
      // Do not change position until first drag.  This helps
      // with unintended value changes.
      if( tolerantThumbRect.Inside( event.GetPosition() ) )
      {
         // Remember mouse position and current value
         mClickX = event.m_x;
         mClickValue = mCurrentValue;

         mIsDragging = true;
      }
      // Clicked to set location?
      else
      {
         mCurrentValue = ClickPositionToValue(event.m_x, event.ShiftDown());
      }

      mParent->CaptureMouse();

      FormatPopWin();
      SetPopWinPosition();
      mPopWin->Show();
   }
   else if( event.ButtonUp() )
   {
      mIsDragging = false;
      if (mParent->HasCapture())
         mParent->ReleaseMouse();
      mPopWin->Hide();
      ((TipPanel *)mPopWin)->SetPos(wxPoint(-1000, -1000));
   }
   else if( event.Dragging() && mIsDragging )
   {
      if (event.m_y < (r.y - 2 * r.height) ||
          event.m_y > (r.y + 3 * r.height)) {
         // If the mouse y coordinate is relatively far from the slider,
         // snap back to the original position
         mCurrentValue = mClickValue;
      }
      else {
         // Otherwise, move the slider to the right position based
         // on the mouse position
         mCurrentValue = DragPositionToValue(event.m_x, event.ShiftDown());
      }
   }
   else if( event.m_wheelRotation != 0 )
   {

      //Calculate the number of steps in a given direction this event
      //represents (allows for two or more clicks on a single event.)
      int steps =  event.m_wheelRotation /
         (event.m_wheelDelta > 0 ? event.m_wheelDelta : 120);

      if( steps < 0 )
      {
         Decrease( -steps );
      }
      else
      {
         Increase( steps );
      }
      SendUpdate( mCurrentValue );
   }

   if( prevValue != mCurrentValue )
      SendUpdate( mCurrentValue );
}

void LWSlider::OnKeyEvent(wxKeyEvent & event)
{
   switch( event.GetKeyCode() )
   {
      case WXK_RIGHT:
      case WXK_UP:
         Increase( 1 );
         SendUpdate( mCurrentValue );
      break;

      case WXK_LEFT:
      case WXK_DOWN:
         Decrease( 1 );
         SendUpdate( mCurrentValue );
      break;

      case WXK_PAGEUP:
#if !wxCHECK_VERSION(2,7,0)
      case WXK_PRIOR:
#endif
         Increase( 5 );
         SendUpdate( mCurrentValue );
      break;

      case WXK_PAGEDOWN:
#if !wxCHECK_VERSION(2,7,0)
      case WXK_NEXT:
#endif
         Decrease( 5 );
         SendUpdate( mCurrentValue );
      break;

      case WXK_HOME:
         SendUpdate( mMinValue );
      break;

      case WXK_END:
         SendUpdate( mMaxValue );
      break;

      case WXK_TAB:
      {
        wxNavigationKeyEvent nevent;
        nevent.SetWindowChange( event.ControlDown() );
        nevent.SetDirection( !event.ShiftDown() );
        nevent.SetEventObject( mParent );
        nevent.SetCurrentFocus( mParent );
        mParent->GetParent()->ProcessEvent( nevent );
      }
      break;

      case WXK_RETURN:
      case WXK_NUMPAD_ENTER:
      {
         wxTopLevelWindow *tlw = wxDynamicCast(wxGetTopLevelParent(mParent), wxTopLevelWindow);
         wxWindow *def = tlw->GetDefaultItem();
         if (def && def->IsEnabled()) {
            wxCommandEvent cevent(wxEVT_COMMAND_BUTTON_CLICKED,
                                  def->GetId());
            mParent->ProcessEvent(cevent);
         }
      }

      default:
         // Allow it to propagate
         event.Skip();
      break;
   }

   event.Skip();
}

void LWSlider::SendUpdate( float newValue )
{
   mCurrentValue = newValue;
   FormatPopWin();
   mPopWin->Refresh();
   Refresh();

   wxCommandEvent e( wxEVT_COMMAND_SLIDER_UPDATED, mID );
   int intValue = (int)( ( mCurrentValue - mMinValue ) * 1000.0f /
                         ( mMaxValue - mMinValue ) );
   e.SetInt( intValue );
   mParent->ProcessEvent( e );
}

int LWSlider::ValueToPosition(float val)
{
   return (int)rint((val - mMinValue) * mWidthX / (mMaxValue - mMinValue));
}

void LWSlider::SetSpeed(float speed)
{
   mSpeed = speed;
}

// Given the mouse x coordinate in xPos, compute the new value
// of the slider when clicking to set a new position.
float LWSlider::ClickPositionToValue(int xPos, bool shiftDown)
{
   int pos = (xPos - mLeft - (mThumbWidth / 2));

   if (pos <= 0)
      return mMinValue;
   if (pos >= mWidthX)
      return mMaxValue;

   float val = (pos / (float)mWidthX)
      * (mMaxValue - mMinValue) + mMinValue;

   if (val < mMinValue)
      val = mMinValue;
   if (val > mMaxValue)
      val = mMaxValue;

   if (!(mCanUseShift && shiftDown) && mStepValue != STEP_CONTINUOUS)
   {
      // MM: If shift is not down, or we don't allow usage
      // of shift key at all, trim value to steps of
      // provided size.
      val = (int)(val / mStepValue + 0.5 * (val>0?1.0f:-1.0f)) * mStepValue;
   }

   return val;
}

// Given the mouse x coordinate in xPos, compute the new value
// of the slider during a drag.
float LWSlider::DragPositionToValue(int xPos, bool shiftDown)
{
   int delta = (xPos - mClickX);
   float val = mClickValue +
      mSpeed * (delta / (float)mWidthX) * (mMaxValue - mMinValue);

   if (val < mMinValue)
      val = mMinValue;
   if (val > mMaxValue)
      val = mMaxValue;

   if (!(mCanUseShift && shiftDown) && mStepValue != STEP_CONTINUOUS)
   {
      // MM: If shift is not down, or we don't allow usage
      // of shift key at all, trim value to steps of
      // provided size.
      val = (int)(val / mStepValue + 0.5 * (val>0?1.0f:-1.0f)) * mStepValue;
   }

   return val;
}

float LWSlider::Get( bool convert )
{
   if (mStyle == DB_SLIDER)
      return ( convert ? pow(10.0f, mCurrentValue / 20.0f) : mCurrentValue );
   else
      return mCurrentValue;
}

void LWSlider::Set(float value)
{
   if(mIsDragging)
      return;
   if (mStyle == DB_SLIDER)
      mCurrentValue = 20.0f*log10(value);
   else
      mCurrentValue = value;

   if (mCurrentValue < mMinValue)
      mCurrentValue = mMinValue;
   if (mCurrentValue > mMaxValue)
      mCurrentValue = mMaxValue;

   Refresh();
}

void LWSlider::Increase(int steps)
{
   float stepValue = mStepValue;

   if( stepValue == 0.0 )
   {
      stepValue = ( mMaxValue - mMinValue ) / 10.0;
   }

   mCurrentValue += ( steps * stepValue );

   if( mCurrentValue < mMinValue )
   {
      mCurrentValue = mMinValue;
   }
   else if( mCurrentValue > mMaxValue )
   {
      mCurrentValue = mMaxValue;
   }

   Refresh();
}

void LWSlider::Decrease(int steps)
{
   float stepValue  = mStepValue;

   if( stepValue == 0.0 )
   {
      stepValue = ( mMaxValue - mMinValue ) / 10.0;
   }

   mCurrentValue -= ( steps * stepValue );

   if( mCurrentValue < mMinValue )
   {
      mCurrentValue = mMinValue;
   }
   else if( mCurrentValue > mMaxValue )
   {
      mCurrentValue = mMaxValue;
   }

   Refresh();
}

void LWSlider::Refresh()
{
   if (mHW)
      mParent->Refresh(false);
}

//
// ASlider
//

BEGIN_EVENT_TABLE(ASlider, wxWindow)
   EVT_CHAR(ASlider::OnKeyEvent)
   EVT_MOUSE_EVENTS(ASlider::OnMouseEvent)
   EVT_PAINT(ASlider::OnPaint)
   EVT_SIZE(ASlider::OnSize)
   EVT_ERASE_BACKGROUND(ASlider::OnErase)
   EVT_SLIDER(wxID_ANY, ASlider::OnSlider)
   EVT_SET_FOCUS(ASlider::OnSetFocus)
   EVT_KILL_FOCUS(ASlider::OnKillFocus)
END_EVENT_TABLE()

ASlider::ASlider( wxWindow * parent,
                  wxWindowID id,
                  wxString name,
                  const wxPoint & pos, 
                  const wxSize & size,
                  int style,
                  bool popup,
                  bool canUseShift,
                  float stepValue ):
   wxPanel( parent, id, pos, size, wxWANTS_CHARS )
{
   mLWSlider = new LWSlider( this,
                             name,
                             wxPoint(0,0),
                             size,
                             style,
                             canUseShift,
                             popup );
   mLWSlider->mStepValue = stepValue;
   mLWSlider->SetId( id );
   SetName( name );

   mSliderIsFocused = false;

#if wxUSE_ACCESSIBILITY
   SetAccessible( new ASliderAx( this ) );
#endif
}


ASlider::~ASlider()
{
   delete mLWSlider;
}

void ASlider::OnSlider(wxCommandEvent &event)
{

   if( event.GetId() == mLWSlider->GetId() )
   {
#if wxUSE_ACCESSIBILITY
      GetAccessible()->NotifyEvent( wxACC_EVENT_OBJECT_VALUECHANGE,
                                    this,
                                    wxOBJID_CLIENT,
                                    wxACC_SELF );
#endif
   }

   event.Skip();
}

void ASlider::OnSize(wxSizeEvent &event)
{
   mLWSlider->OnSize( event );
}

void ASlider::OnErase(wxEraseEvent &event)
{
   // Ignore it to prevent flashing
}

void ASlider::OnPaint(wxPaintEvent &event)
{
   wxPaintDC dc(this);

#ifdef EXPERIMENTAL_THEMING
   wxColour Col(GetParent()->GetBackgroundColour());
   this->SetBackgroundColour( Col );
#endif

   mLWSlider->OnPaint(dc, false);

   if( mSliderIsFocused )
   {
      wxRect r( 0, 0, mLWSlider->mWidth, mLWSlider->mHeight );

      r.Deflate( 1, 1 );

      AColor::DrawFocus( dc, r );
   }
}

void ASlider::OnMouseEvent(wxMouseEvent &event)
{
   mLWSlider->OnMouseEvent(event);
}

void ASlider::OnKeyEvent(wxKeyEvent &event)
{
   mLWSlider->OnKeyEvent(event);
}

void ASlider::OnSetFocus(wxFocusEvent & event)
{
   mSliderIsFocused = true;
   Refresh();
}

void ASlider::OnKillFocus(wxFocusEvent & event)
{
   mSliderIsFocused = false;
   Refresh();
}

void ASlider::RecreateTipWin()
{
   mLWSlider->RecreateTipWin();
}

float ASlider::Get( bool convert )
{
   return mLWSlider->Get( convert );
}

void ASlider::Set(float value)
{
   mLWSlider->Set(value);
}

void ASlider::Increase(int steps)
{
   mLWSlider->Increase(steps);
}

void ASlider::Decrease(int steps)
{
   mLWSlider->Decrease(steps);
}

void ASlider::SetSpeed(float speed)
{
   mLWSlider->SetSpeed(speed);
}

#if wxUSE_ACCESSIBILITY

ASliderAx::ASliderAx( wxWindow * window ):
   wxWindowAccessible( window )
{
}

ASliderAx::~ASliderAx()
{
}

// Retrieves the address of an IDispatch interface for the specified child.
// All objects must support this property.
wxAccStatus ASliderAx::GetChild( int childId, wxAccessible** child )
{
   if( childId == wxACC_SELF )
   {
      *child = this;
   }
   else
   {
      *child = NULL;
   }

   return wxACC_OK;
}

// Gets the number of children.
wxAccStatus ASliderAx::GetChildCount(int* childCount)
{
   *childCount = 3;

   return wxACC_OK;
}

// Gets the default action for this object (0) or > 0 (the action for a child).
// Return wxACC_OK even if there is no action. actionName is the action, or the empty
// string if there is no action.
// The retrieved string describes the action that is performed on an object,
// not what the object does as a result. For example, a toolbar button that prints
// a document has a default action of "Press" rather than "Prints the current document."
wxAccStatus ASliderAx::GetDefaultAction( int childId, wxString *actionName )
{
   actionName->Clear();

   return wxACC_OK;
}

// Returns the description for this object or a child.
wxAccStatus ASliderAx::GetDescription( int childId, wxString *description )
{
   description->Clear();

   return wxACC_OK;
}

// Gets the window with the keyboard focus.
// If childId is 0 and child is NULL, no object in
// this subhierarchy has the focus.
// If this object has the focus, child should be 'this'.
wxAccStatus ASliderAx::GetFocus(int* childId, wxAccessible** child)
{
   *childId = 0;
   *child = this;

   return wxACC_OK;
}

// Returns help text for this object or a child, similar to tooltip text.
wxAccStatus ASliderAx::GetHelpText( int childId, wxString *helpText )
{
   helpText->Clear();

   return wxACC_OK;
}

// Returns the keyboard shortcut for this object or child.
// Return e.g. ALT+K
wxAccStatus ASliderAx::GetKeyboardShortcut( int childId, wxString *shortcut )
{
   shortcut->Clear();

   return wxACC_OK;
}

// Returns the rectangle for this object (id = 0) or a child element (id > 0).
// rect is in screen coordinates.
wxAccStatus ASliderAx::GetLocation( wxRect& rect, int elementId )
{
   ASlider *as = wxDynamicCast( GetWindow(), ASlider );

   rect = as->GetRect();
   rect.SetPosition( as->GetParent()->ClientToScreen( rect.GetPosition() ) );

   return wxACC_OK;
}

// Gets the name of the specified object.
wxAccStatus ASliderAx::GetName(int childId, wxString* name)
{
   ASlider *as = wxDynamicCast( GetWindow(), ASlider );

   *name = as->GetName();

   return wxACC_OK;
}

// Returns a role constant.
wxAccStatus ASliderAx::GetRole(int childId, wxAccRole* role)
{
   switch( childId )
   {
      case 0:
         *role = wxROLE_SYSTEM_SLIDER;
      break;

      case 1:
      case 3:
         *role = wxROLE_SYSTEM_PUSHBUTTON;
      break;

      case 2:
         *role = wxROLE_SYSTEM_INDICATOR;
      break;
   }

   return wxACC_OK;
}

// Gets a variant representing the selected children
// of this object.
// Acceptable values:
// - a null variant (IsNull() returns TRUE)
// - a list variant (GetType() == wxT("list"))
// - an integer representing the selected child element,
//   or 0 if this object is selected (GetType() == wxT("long"))
// - a "void*" pointer to a wxAccessible child object
wxAccStatus ASliderAx::GetSelections( wxVariant *selections )
{
   return wxACC_NOT_IMPLEMENTED;
}

// Returns a state constant.
wxAccStatus ASliderAx::GetState(int childId, long* state)
{
   ASlider *as = wxDynamicCast( GetWindow(), ASlider );

   switch( childId )
   {
      case 0:
         *state = wxACC_STATE_SYSTEM_FOCUSABLE;
      break;

      case 1:
         if( as->mLWSlider->mCurrentValue == as->mLWSlider->mMinValue )
         {
            *state = wxACC_STATE_SYSTEM_INVISIBLE;
         }
      break;

      case 3:
         if( as->mLWSlider->mCurrentValue == as->mLWSlider->mMaxValue )
         {
            *state = wxACC_STATE_SYSTEM_INVISIBLE;
         }
      break;
   }

   // Do not use mSliderIsFocused is not set until after this method
   // is called.
   *state |= ( as == wxWindow::FindFocus() ? wxACC_STATE_SYSTEM_FOCUSED : 0 );

   return wxACC_OK;
}

// Returns a localized string representing the value for the object
// or child.
wxAccStatus ASliderAx::GetValue(int childId, wxString* strValue)
{
   ASlider *as = wxDynamicCast( GetWindow(), ASlider );

   if( childId == 0 )
   {
      switch( as->mLWSlider->mStyle )
      {
         case FRAC_SLIDER:
            strValue->Printf( wxT("%.0f"), as->mLWSlider->mCurrentValue * 100 );
            break;
           
         case DB_SLIDER:
            strValue->Printf( wxT("%.0f"), as->mLWSlider->mCurrentValue );
            break;

         case PAN_SLIDER:
            strValue->Printf( wxT("%.0f"), as->mLWSlider->mCurrentValue * 100 );
            break;

         case SPEED_SLIDER:
            strValue->Printf( wxT("%.0f"), as->mLWSlider->mCurrentValue * 100 );
            break;
      }

      return wxACC_OK;
   }

   return wxACC_NOT_SUPPORTED;
}

#endif

// Indentation settings for Vim and Emacs and unique identifier for Arch, a
// version control system. Please do not modify past this point.
//
// Local Variables:
// c-basic-offset: 3
// indent-tabs-mode: nil
// End:
//
// vim: et sts=3 sw=3
// arch-tag: 0030c06f-7a62-40eb-b833-0a9fb96d1f55

