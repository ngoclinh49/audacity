/**********************************************************************

  Audacity: A Digital Audio Editor

  DtmfGen.cpp

  Salvo Ventura - Dec 2006

*******************************************************************//**

\class EffectDtmf
\brief An effect for the "Generator" menu to generate DTMF tones

*//*******************************************************************/

#include "DtmfGen.h"
#include "../Audacity.h"
#include "../Project.h"
#include "../Prefs.h"
#include "../ShuttleGui.h"
#include "../WaveTrack.h"

#include <wx/slider.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/valtext.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846  /* pi */
#endif
#define DUTY_MIN 0
#define DUTY_MAX 1000
#define DUTY_SCALE (DUTY_MAX/100.0) // ensure float division
#define FADEINOUT 250.0    // used for fadein/out needed to remove clicking noise
#define AMP_MIN 0
#define AMP_MAX 1

//
// EffectDtmf
//

bool EffectDtmf::PromptUser()
{
   DtmfDialog dlog(mParent, _("DTMF Tone Generator"));


   // dialog will be passed values from effect
   // Effect retrieves values from saved config
   // Dialog will take care of using them to initialize controls
   // If there is a selection, use that duration, otherwise use
   // value from saved config: this is useful is user wants to
   // replace selection with dtmf sequence

   if (mT1 > mT0) {
      // there is a selection: let's fit in there...
      dtmfDuration = mT1 - mT0;
      dlog.dIsSelection = true;
   } else {
      // retrieve last used values
      gPrefs->Read(wxT("/CsPresets/DtmfGen_SequenceDuration"), &dtmfDuration, 1L);
      dlog.dIsSelection = false;
   }
   /// \todo this code shouldn't be using /CsPresets - need to review its use
   gPrefs->Read(wxT("/CsPresets/DtmfGen_String"), &dtmfString, wxT("audacity"));
   gPrefs->Read(wxT("/CsPresets/DtmfGen_DutyCycle"), &dtmfDutyCycle, 550L);
   gPrefs->Read(wxT("/CsPresets/DtmfGen_Amplitude"), &dtmfAmplitude, 0.8f);

   dtmfNTones = wxStrlen(dtmfString);

   // Initialize dialog locals
   dlog.dString = dtmfString;
   dlog.dDutyCycle = dtmfDutyCycle;
   dlog.dDuration = dtmfDuration;
   dlog.dAmplitude = dtmfAmplitude;

   // start dialog
   dlog.Init();
   dlog.ShowModal();

   if (dlog.GetReturnCode() == wxID_CANCEL)
      return false;

   // if there was an OK, retrieve values
   dtmfString = dlog.dString;
   dtmfDutyCycle = dlog.dDutyCycle;
   dtmfDuration = dlog.dDuration;
   dtmfAmplitude = dlog.dAmplitude;
   
   dtmfNTones = dlog.dNTones;
   dtmfTone = dlog.dTone;
   dtmfSilence = dlog.dSilence;

   return true;
}


bool EffectDtmf::TransferParameters( Shuttle & shuttle )
{
   return true;
}


bool EffectDtmf::MakeDtmfTone(float *buffer, sampleCount len, float fs, wxChar tone, sampleCount last, longSampleCount total, float amplitude)
{

/*
  --------------------------------------------
              1209 Hz 1336 Hz 1477 Hz 1633 Hz

                          ABC     DEF
   697 Hz          1       2       3       A

                  GHI     JKL     MNO
   770 Hz          4       5       6       B

                  PQRS     TUV     WXYZ
   852 Hz          7       8       9       C

                          oper
   941 Hz          *       0       #       D
  --------------------------------------------
  Essentially we need to generate two sin with
  frequencies according to this table, and sum
  them up.
  sin wave is generated by:
   s(n)=sin(2*pi*n*f/fs)

  We will precalculate:
     A= 2*pi*f1/fs
     B= 2*pi*f2/fs

  And use two switch statements to select the frequency

  Note: added support for letters, like those on the keypad
        This support is only for lowercase letters: uppercase
        are still considered to be the 'military'/carrier extra
        tones.
*/

   float f1, f2=0.0;
   float A,B;

   // select low tone: left column
   switch (tone) {
      case '1':   case '2':   case '3':   case 'A': 
      case 'a':   case 'b':   case 'c':
      case 'd':   case 'e':   case 'f':
         f1=697;
         break;
      case '4':   case '5':   case '6':   case 'B':   
      case 'g':   case 'h':   case 'i':
      case 'j':   case 'k':   case 'l':
      case 'm':   case 'n':   case 'o':
         f1=770;
         break;
      case '7':   case '8':   case '9':   case 'C':   
      case 'p':   case 'q':   case 'r':   case 's':
      case 't':   case 'u':   case 'v':
      case 'w':   case 'x':   case 'y':   case 'z':
         f1=852;
         break;
      case '*':   case '0':   case '#':   case 'D':   
         f1=941;
         break;
      default:
         f1=0;
   }

   // select high tone: top row
   switch (tone) {
      case '1':   case '4':   case '7':   case '*':
      case 'g':   case 'h':   case 'i':
      case 'p':   case 'q':   case 'r':   case 's':
         f2=1209;
         break;
      case '2':   case '5':   case '8':   case '0':
      case 'a':   case 'b':   case 'c':
      case 'j':   case 'k':   case 'l':
      case 't':   case 'u':   case 'v':
         f2=1336;
         break;
      case '3':   case '6':   case '9':   case '#':
      case 'd':   case 'e':   case 'f':
      case 'm':   case 'n':   case 'o':
      case 'w':   case 'x':   case 'y':   case 'z':
         f2=1477;
         break;
      case 'A':   case 'B':   case 'C':   case 'D':
         f2=1633;
         break;
      default:
         f2=0;
   }

   // precalculations
   A=B=2*M_PI/fs;
   A*=f1;
   B*=f2;

   // now generate the wave: 'last' is used to avoid phase errors
   // when inside the inner for loop of the Process() function.
   for(sampleCount i=0; i<len; i++) {
      buffer[i]=amplitude*0.5*(sin(A*(i+last))+sin(B*(i+last)));
   }

   // generate a fade-in of duration 1/250th of second
   if (last==0) {
      A=(fs/FADEINOUT);
      for(sampleCount i=0; i<A; i++) {
         buffer[i]*=i/A;
      }
   }

   // generate a fade-out of duration 1/250th of second
   if (last==total-len) {
      // we are at the last buffer of 'len' size, so, offset is to
      // backup 'A' samples, from 'len'
      A=(fs/FADEINOUT);
      sampleCount offset=len-(longSampleCount)(fs/FADEINOUT);
      // protect against negative offset, which can occur if too a 
      // small selection is made
      if (offset>=0) {
         for(sampleCount i=0; i<A; i++) {
            buffer[i+offset]*=(1-(i/A));
         }
      }
   }
   return true;
}

bool EffectDtmf::Process()
{
   if (dtmfDuration <= 0.0)
      return false;

   //Iterate over each track
   this->CopyInputWaveTracks(); // Set up m_pOutputWaveTracks.
   bool bGoodResult = true;
   int ntrack = 0;
   TrackListIterator iter(m_pOutputWaveTracks);
   WaveTrack *track = (WaveTrack *)iter.First();
   while (track) {
      // new tmp track, to fill with dtmf sequence
      // we will build the track by adding a tone, then a silence, next tone, and so on...
      WaveTrack *tmp = mFactory->NewWaveTrack(track->GetSampleFormat(), track->GetRate());

      // all dtmf sequence durations in samples from seconds
      numSamplesSequence = (longSampleCount)(dtmfDuration * track->GetRate() + 0.5);
      numSamplesTone = (longSampleCount)(dtmfTone * track->GetRate() + 0.5);
      numSamplesSilence = (longSampleCount)(dtmfSilence * track->GetRate() + 0.5);

      // recalculate the sum, and spread the difference - due to approximations.
      // Since diff should be in the order of "some" samples, a division (resulting in zero)
      // is not sufficient, so we add the additional remaining samples in each tone/silence block,
      // at least until available.
      int diff = numSamplesSequence - (dtmfNTones*numSamplesTone) - (dtmfNTones-1)*numSamplesSilence;
      if (diff>dtmfNTones) {
         // in this case, both these values would change, so it makes sense to recalculate diff
         // otherwise just keep the value we already have

         // should always be the case that dtmfNTones>1, as if 0, we don't even start processing,
         // and with 1 there is no difference to spread (no silence slot)...
         wxASSERT(dtmfNTones > 1);
         numSamplesTone += (diff/(dtmfNTones));
         numSamplesSilence += (diff/(dtmfNTones-1));
         diff = numSamplesSequence - (dtmfNTones*numSamplesTone) - (dtmfNTones-1)*numSamplesSilence;
      }
      // this var will be used as extra samples distributor
      int extra=0;

      longSampleCount i = 0;
      longSampleCount j = 0;
      int n=0; // pointer to string in dtmfString
      sampleCount block;
      bool isTone = true;
      float *data = new float[tmp->GetMaxBlockSize()];

      // for the whole dtmf sequence, we will be generating either tone or silence
      // according to a bool value, and this might be done in small chunks of size
      // 'block', as a single tone might sometimes be larger than the block
      // tone and silence generally have different duration, thus two generation blocks
      //
      // Note: to overcome a 'clicking' noise introduced by the abrupt transition from/to
      // silence, I added a fade in/out of 1/250th of a second (4ms). This can still be
      // tweaked but gives excellent results at 44.1kHz: I haven't tried other freqs.
      // A problem might be if the tone duration is very short (<10ms)... (?)
      //
      // One more problem is to deal with the approximations done when calculating the duration 
      // of both tone and silence: in some cases the final sum might not be same as the initial
      // duration. So, to overcome this, we had a redistribution block up, and now we will spread
      // the remaining samples in every bin in order to achieve the full duration: test case was
      // to generate an 11 tone DTMF sequence, in 4 seconds, and with DutyCycle=75%: after generation
      // you ended up with 3.999s or in other units: 3 seconds and 44097 samples.
      //
      while ((i < numSamplesSequence) && bGoodResult) {
         if (isTone)
         // generate tone
         {
            // the statement takes care of extracting one sample from the diff bin and
            // adding it into the tone block until depletion
            extra=(diff-- > 0?1:0);
            for(j=0; j < numSamplesTone+extra; j+=block) {
               block = tmp->GetBestBlockSize(j);
               if (block > (numSamplesTone+extra - j))
                   block = numSamplesTone+extra - j;

               // generate the tone and append
               MakeDtmfTone(data, block, track->GetRate(), dtmfString[n], j, numSamplesTone, dtmfAmplitude);
               tmp->Append((samplePtr)data, floatSample, block);
            }
            i += numSamplesTone;
            n++;
            if(n>=dtmfNTones)break;
         }
         else
         // generate silence
         {
            // the statement takes care of extracting one sample from the diff bin and
            // adding it into the silence block until depletion
            extra=(diff-- > 0?1:0);
            for(j=0; j < numSamplesSilence+extra; j+=block) {
               block = tmp->GetBestBlockSize(j);
               if (block > (numSamplesSilence+extra - j))
                   block = numSamplesSilence+extra - j;

               // generate silence and append
               memset(data, 0, sizeof(float)*block);
               tmp->Append((samplePtr)data, floatSample, block);
            }
            i += numSamplesSilence;
         }
         // flip flag
         isTone=!isTone;

         //Update the Progress meter
         if (TrackProgress(ntrack, (double)i / numSamplesSequence))
            bGoodResult = false;
      } // finished the whole dtmf sequence

      delete[] data;

      tmp->Flush();
      track->Clear(mT0, mT1);
      track->Paste(mT0, tmp);
      delete tmp;

      if (!bGoodResult)
         break;

      //Iterate to the next track
      ntrack++;
      track = (WaveTrack *)iter.Next();
   }

   if (bGoodResult)
      {
      /*
         save last used values
         save duration unless value was got from selection, so we save only
         when user explicitely setup a value
      */
      if (mT1 == mT0)
         gPrefs->Write(wxT("/CsPresets/DtmfGen_SequenceDuration"), dtmfDuration);

      gPrefs->Write(wxT("/CsPresets/DtmfGen_String"), dtmfString);
      gPrefs->Write(wxT("/CsPresets/DtmfGen_DutyCycle"), dtmfDutyCycle);
      gPrefs->Write(wxT("/CsPresets/DtmfGen_Amplitude"), dtmfAmplitude);

      // Update selection: this is not accurate if my calculations are wrong.
      // To validate, once the effect is done, unselect, and select all, then
      // see what the selection length is being reported (in sec,ms,samples)
      mT1 = mT0 + dtmfDuration;
   }

   this->ReplaceProcessedWaveTracks(bGoodResult); 
   return bGoodResult;
}

//----------------------------------------------------------------------------
// DtmfDialog
//----------------------------------------------------------------------------

const static wxChar *dtmfSymbols[] =
{
   wxT("0"), wxT("1"), wxT("2"), wxT("3"),
   wxT("4"), wxT("5"), wxT("6"), wxT("7"),
   wxT("8"), wxT("9"), wxT("*"), wxT("#"),
   wxT("A"), wxT("B"), wxT("C"), wxT("D"),
   wxT("a"), wxT("b"), wxT("c"), wxT("d"),
   wxT("e"), wxT("f"), wxT("g"), wxT("h"),
   wxT("i"), wxT("j"), wxT("k"), wxT("l"),
   wxT("m"), wxT("n"), wxT("o"), wxT("p"),
   wxT("q"), wxT("r"), wxT("s"), wxT("t"),
   wxT("u"), wxT("v"), wxT("w"), wxT("x"),
   wxT("y"), wxT("z")
};

#define ID_DTMF_DUTYCYCLE_SLIDER 10001
#define ID_DTMF_STRING_TEXT      10002
#define ID_DTMF_DURATION_TEXT    10003
#define ID_DTMF_DUTYCYCLE_TEXT   10004
#define ID_DTMF_TONELEN_TEXT     10005
#define ID_DTMF_SILENCE_TEXT     10006


BEGIN_EVENT_TABLE(DtmfDialog, EffectDialog)
    EVT_TEXT(ID_DTMF_STRING_TEXT, DtmfDialog::OnDtmfStringText)
    EVT_TEXT(ID_DTMF_DURATION_TEXT, DtmfDialog::OnDtmfDurationText)
    EVT_COMMAND(wxID_ANY, EVT_TIMETEXTCTRL_UPDATED, DtmfDialog::OnTimeCtrlUpdate)
    EVT_SLIDER(ID_DTMF_DUTYCYCLE_SLIDER, DtmfDialog::OnDutyCycleSlider)
END_EVENT_TABLE()


DtmfDialog::DtmfDialog(wxWindow * parent, const wxString & title): EffectDialog(parent, title, INSERT_EFFECT)
{
   /*
   wxString dString;       // dtmf tone string
   int    dNTones;         // total number of tones to generate
   double dTone;           // duration of a single tone
   double dSilence;        // duration of silence between tones
   double dDuration;       // duration of the whole dtmf tone sequence
   */
   dTone = 0;
   dSilence = 0;
   dDuration = 0;

   mDtmfDurationT = NULL;
}

void DtmfDialog::PopulateOrExchange( ShuttleGui & S )
{
   wxTextValidator vldDtmf(wxFILTER_INCLUDE_CHAR_LIST);
   vldDtmf.SetIncludes(wxArrayString(42, dtmfSymbols));

   S.AddTitle(_("by Salvo Ventura (2006)"));

   S.StartMultiColumn(2, wxEXPAND);
   {
      mDtmfStringT = S.Id(ID_DTMF_STRING_TEXT).AddTextBox(_("DTMF sequence:"), wxT(""), 10);
      mDtmfStringT->SetValidator(vldDtmf);

      S.TieTextBox(_("Amplitude (0-1)"),  dAmplitude, 10);

      S.AddPrompt(_("Duration:"));
      if (mDtmfDurationT == NULL)
      {
         mDtmfDurationT = new
            TimeTextCtrl(this,
                         ID_DTMF_DURATION_TEXT,
                         wxT(""),
                         dDuration,
                         44100,
                         wxDefaultPosition,
                         wxDefaultSize,
                         true);
         /* use this instead of "seconds" because if a selection is passed to the
         * effect, I want it (dDuration) to be used as the duration, and with
         * "seconds" this does not always work properly. For example, it rounds
         * down to zero... */
         mDtmfDurationT->SetFormatString(mDtmfDurationT->GetBuiltinFormat(dIsSelection==true?(wxT("hh:mm:ss + samples")):(wxT("seconds"))));
         mDtmfDurationT->EnableMenu();
      }
      S.AddWindow(mDtmfDurationT);

      S.AddFixedText(_("Tone/silence ratio:"), false);
      S.SetStyle(wxSL_HORIZONTAL | wxEXPAND);
      mDtmfDutyS = S.Id(ID_DTMF_DUTYCYCLE_SLIDER).AddSlider(wxT(""), (int)dDutyCycle, DUTY_MAX, DUTY_MIN);

      S.SetSizeHints(-1,-1);
   }
   S.EndMultiColumn();

   S.StartMultiColumn(2, wxCENTER);
   {
      S.AddFixedText(_("Duty cycle:"), false);
      mDtmfDutyT = S.Id(ID_DTMF_DUTYCYCLE_TEXT).AddVariableText(wxString::Format(wxT("%.1f %%"), (float) dDutyCycle/DUTY_SCALE), false);
      S.AddFixedText(_("Tone duration:"), false);
      mDtmfSilenceT = S.Id(ID_DTMF_TONELEN_TEXT).AddVariableText(wxString::Format(wxString(wxT("%d ")) + _("ms"),  (int) dTone * 1000), false);
      S.AddFixedText(_("Silence duration:"), false);
      mDtmfToneT = S.Id(ID_DTMF_SILENCE_TEXT).AddVariableText(wxString::Format(wxString(wxT("%d ")) + _("ms"), (int) dSilence * 1000), false);
   }
   S.EndMultiColumn();
}

bool DtmfDialog::TransferDataToWindow()
 {
   mDtmfDutyS->SetValue((int)dDutyCycle);
   mDtmfStringT->SetValue(dString);
   mDtmfDurationT->SetTimeValue(dDuration);

   return true;
}

bool DtmfDialog::TransferDataFromWindow()
{
   EffectDialog::TransferDataFromWindow();
   dAmplitude = TrapDouble(dAmplitude, AMP_MIN, AMP_MAX);
   // recalculate to make sure all values are up-to-date. This is especially
   // important if the user did not change any values in the dialog
   Recalculate();

   return true;
}

/*
 *
 */

void DtmfDialog::Recalculate(void) {

   // remember that dDutyCycle is in range (0-1000)
   double slot;

   dString = mDtmfStringT->GetValue();
   dDuration = mDtmfDurationT->GetTimeValue();

   dNTones = wxStrlen(dString);
   dDutyCycle = TrapLong(mDtmfDutyS->GetValue(), DUTY_MIN, DUTY_MAX);

   if (dNTones==0) {
      // no tones, all zero: don't do anything
      // this should take care of the case where user got an empty
      // dtmf sequence into the generator: track won't be generated
      dTone = 0;
      dDuration = 0;
      dSilence = dDuration;
   } else
     if (dNTones==1) {
        // single tone, as long as the sequence
          dSilence = 0;
          dTone = dDuration;

     } else {
        // Don't be fooled by the fact that you divide the sequence into dNTones:
        // the last slot will only contain a tone, not ending with silence.
        // Given this, the right thing to do is to divide the sequence duration
        // by dNTones tones and (dNTones-1) silences each sized according to the duty
        // cycle: original division was:
        // slot=dDuration / (dNTones*(dDutyCycle/DUTY_MAX)+(dNTones-1)*(1.0-dDutyCycle/DUTY_MAX))
        // which can be simplified in the one below.
        // Then just take the part that belongs to tone or silence.
        //
        slot=dDuration/((double)dNTones+(dDutyCycle/DUTY_MAX)-1);
        dTone = slot * (dDutyCycle/DUTY_MAX); // seconds
        dSilence = slot * (1.0 - (dDutyCycle/DUTY_MAX)); // seconds

        // Note that in the extremes we have:
        // - dutyCycle=100%, this means no silence, so each tone will measure dDuration/dNTones
        // - dutyCycle=0%, this means no tones, so each silence slot will measure dDuration/(NTones-1)
        // But we always count:
        // - dNTones tones
        // - dNTones-1 silences
     }

   mDtmfDutyT->SetLabel(wxString::Format(wxT("%.1f %%"), (float)dDutyCycle/DUTY_SCALE));
   mDtmfSilenceT->SetLabel(wxString::Format(wxString(wxT("%d ")) + _("ms"),  (int) (dTone * 1000)));
   mDtmfToneT->SetLabel(wxString::Format(wxString(wxT("%d ")) + _("ms"),  (int) (dSilence * 1000)));
}

void DtmfDialog::OnDutyCycleSlider(wxCommandEvent & event) {
   Recalculate();
}


void DtmfDialog::OnDtmfStringText(wxCommandEvent & event) {
   Recalculate();
}

void DtmfDialog::OnDtmfDurationText(wxCommandEvent & event) {
   Recalculate();
}

void DtmfDialog::OnTimeCtrlUpdate(wxCommandEvent & event) {
   this->Fit();
}
