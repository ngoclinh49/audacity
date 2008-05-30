/*
 * PortMixer
 * Mac OS X / CoreAudio implementation
 *
 * Copyright (c) 2002, 2006
 *
 * Written by Dominic Mazzoni
 *        and Leland Lucius
 *
 * PortMixer is intended to work side-by-side with PortAudio,
 * the Portable Real-Time Audio Library by Ross Bencina and
 * Phil Burk.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <CoreServices/CoreServices.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioConverter.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>

#include "portaudio.h"
#include "pa_mac_core.h"

#include "portmixer.h"
#include "px_mixer.h"

// define value of isInput passed to CoreAudio routines
#define IS_INPUT    (TRUE)
#define IS_OUTPUT   (FALSE)

typedef struct PxInfo
{
   AudioDeviceID  input;
   AudioDeviceID  output;
   UInt32         *srcids;
   char           **srcnames;
   int            numsrcs;
} PxInfo;

int OpenMixer_Mac_CoreAudio(px_mixer *Px, int index)
{
   PxInfo   *info;
   OSStatus err;
   UInt32   outSize;
   UInt32   inID;
   int      i;

   if (!initialize(Px)) {
      return FALSE;
   }

   info = (PxInfo *) Px->info;

   info->input = PaMacCore_GetStreamInputDevice(Px->pa_stream);;
   info->output = PaMacCore_GetStreamOutputDevice(Px->pa_stream);

   if (info->input == kAudioDeviceUnknown) {
      outSize = sizeof(AudioDeviceID);
      err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultInputDevice,
                                     &outSize,
                                     &info->input);
      if (err) {
         return cleanup(Px);
      }
   }

   outSize = sizeof(UInt32);
   err = AudioDeviceGetPropertyInfo(info->input,
                                    0,
                                    IS_INPUT,
                                    kAudioDevicePropertyDataSources,
                                    &outSize,
                                    NULL);
   if (err) {
      return cleanup(Px);
   }

   info->numsrcs  = outSize / sizeof(UInt32);
   info->srcids   = (UInt32 *)malloc(outSize);
   info->srcnames = (char **)calloc(info->numsrcs, sizeof(char *));

   if (info->srcids == NULL || info->srcnames == NULL) {
      return cleanup(Px);
   }

   err = AudioDeviceGetProperty(info->input,
                                0,
                                IS_INPUT,
                                kAudioDevicePropertyDataSources,
                                &outSize,
                                info->srcids);
   if (err) {
      return cleanup(Px);
   }

   for (i = 0; i < info->numsrcs; i++) {
      AudioValueTranslation trans;
      CFStringRef           name;
      Boolean               ok;

      trans.mInputData = &info->srcids[i];
      trans.mInputDataSize = sizeof(UInt32);
      trans.mOutputData = &name;
      trans.mOutputDataSize = sizeof(CFStringRef);

      outSize = sizeof(AudioValueTranslation);
      err = AudioDeviceGetProperty(info->input,
                                   0,
                                   IS_INPUT,
                                   kAudioDevicePropertyDataSourceNameForIDCFString,
                                   &outSize,
                                   &trans);
      if (err) {
         return cleanup(Px);
      }

      info->srcnames[i] = malloc(CFStringGetLength(name)+1);
      if (info->srcnames[i] == NULL) {
         return cleanup(Px);
      }

      ok = CFStringGetCString(name,
                              info->srcnames[i],
                              CFStringGetLength(name)+1,
                              kCFStringEncodingISOLatin1);
      if (!ok) {
         return cleanup(Px);
      }
   }

   return TRUE;
}

static int initialize(px_mixer *Px)
{
   Px->info = calloc(1, sizeof(PxInfo));
   if (Px->info == NULL) {
      return FALSE;
   }

   Px->CloseMixer = close_mixer;
   Px->GetNumMixers = get_num_mixers;
   Px->GetMixerName = get_mixer_name;

// Px->GetMasterVolume = get_master_volume;
// Px->SetMasterVolume = set_master_volume;

   Px->SupportsPCMOutputVolume = supports_pcm_output_volume;
   Px->GetPCMOutputVolume = get_pcm_output_volume;
   Px->SetPCMOutputVolume = set_pcm_output_volume;
   Px->GetNumOutputVolumes = get_num_output_volumes;
   Px->GetOutputVolumeName = get_output_volume_name;
   Px->GetOutputVolume = get_output_volume;
   Px->SetOutputVolume = set_output_volume;
   Px->GetNumInputSources = get_num_input_sources;
   Px->GetInputSourceName = get_input_source_name;
   Px->GetCurrentInputSource = get_current_input_source;
   Px->SetCurrentInputSource = set_current_input_source;
   Px->GetInputVolume = get_input_volume;
   Px->SetInputVolume = set_input_volume;

// Px->SupportsOutputBalance = supports_output_balance;
// Px->GetOutputBalance = get_output_balance;
// Px->SetOutputBalance = set_output_balance;

   Px->SupportsPlaythrough = supports_play_through;
   Px->GetPlaythrough = get_play_through;
   Px->SetPlaythrough = set_play_through;

   return TRUE;
}

static int cleanup(px_mixer *Px)
{
   PxInfo *info = (PxInfo *)Px->info;
   int i;

   if (info) {
      if (info->srcids) {
         free(info->srcids);
      }

      if (info->srcnames) {
         for (i = 0; i < info->numsrcs; i++) {
            if (info->srcnames[i]) {
               free(info->srcnames[i]);
            }
         }
         free(info->srcnames);
      }

      free(info);
      Px->info = NULL;
   }

   return FALSE;
}

static int get_num_mixers(px_mixer *Px)
{
   return 1;
}

static const char *get_mixer_name(px_mixer *Px, int index)
{
   return "CoreAudio";
}

/*
 Px_CloseMixer() closes a mixer opened using Px_OpenMixer and frees any
 memory associated with it. 
*/

static void close_mixer(px_mixer *Px)
{
   PxInfo *info = (PxInfo *)Px->info;

   cleanup(Px);
}

/*
|| PCM output volume
*/

static PxVolume get_volume(AudioDeviceID device, Boolean isInput)
{
   OSStatus err;
   UInt32   outSize;
   Float32  vol, maxvol = 0.0;
   UInt32   mute, anymuted = 0;
   int ch;
   PxVolume max;

   for (ch = 0; ch <= 2; ch++) {
      outSize = sizeof(Float32);
      err = AudioDeviceGetProperty(device,
                                   ch,
                                   isInput,
                                   kAudioDevicePropertyVolumeScalar,
                                   &outSize,
                                   &vol);
      if (!err) {
         if (vol > maxvol)
            maxvol = vol;
      }

      outSize = sizeof(UInt32);
      err = AudioDeviceGetProperty(device,
                                   ch,
                                   isInput,
                                   kAudioDevicePropertyMute,
                                   &outSize,
                                   &mute);

      if (!err) {
         if (mute)
            anymuted = 1;
      }
   }

   if (anymuted)
         maxvol = 0.0;

   return maxvol;
}

static void set_volume(AudioDeviceID device, Boolean isInput, PxVolume volume)
{
   Float32 vol = volume;
   UInt32 mute = 0;
   int ch;
   OSStatus err;

   /* Implement a passive attitude towards muting.  If they
      drag the volume above 0.05, unmute it.  But if they
      drag the volume down below that, just set the volume,
      don't actually mute.
   */

   for (ch = 0; ch <= 2; ch++) {
      err =  AudioDeviceSetProperty(device,
                                    0,
                                    ch,
                                    isInput,
                                    kAudioDevicePropertyVolumeScalar,
                                    sizeof(Float32),
                                    &vol);
      if (vol > 0.05) {
         err =  AudioDeviceSetProperty(device,
                                       0,
                                       ch,
                                       isInput,
                                       kAudioDevicePropertyMute,
                                       sizeof(UInt32),
                                       &mute);
      }
   }
}

static int supports_pcm_output_volume(px_mixer *Px)
{
	return 1 ;
}

static PxVolume get_pcm_output_volume(px_mixer *Px)
{
   PxInfo *info = (PxInfo *)Px->info;

   return get_volume(info->output, IS_OUTPUT);
}

static void set_pcm_output_volume(px_mixer *Px, PxVolume volume)
{
   PxInfo *info = (PxInfo *)Px->info;

   set_volume(info->output, IS_OUTPUT, volume);
}

/*
|| All output volumes
*/

int get_num_output_volumes(px_mixer *Px)
{
   return 1;
}

const char *get_output_volume_name(px_mixer *Px, int i)
{
   if (i == 0)
      return "PCM";
   else
      return "";
}

static PxVolume get_output_volume(px_mixer *Px, int i)
{
   return get_pcm_output_volume(Px);
}

static void set_output_volume(px_mixer *Px, int i, PxVolume volume)
{
   set_pcm_output_volume(Px, volume);

   return;
}

/*
|| Input sources
*/

static int get_num_input_sources(px_mixer *Px)
{
   PxInfo *info = (PxInfo *)Px->info;

   return info->numsrcs;
}

static const char *get_input_source_name(px_mixer *Px, int i)
{
   PxInfo *info = (PxInfo *)Px->info;

   if (i >= info->numsrcs)
      return NULL;

   return info->srcnames[i];
}

static int get_current_input_source(px_mixer *Px)
{
   PxInfo   *info = (PxInfo *)Px->info;
   OSStatus err;
   UInt32   outSize;
   UInt32   outID = 0;
   int      i;

   outSize = sizeof(UInt32);
   err = AudioDeviceGetProperty(info->input,
                                0,
                                IS_INPUT,
                                kAudioDevicePropertyDataSource,
                                &outSize,
                                &outID);

   if (!err) {
      for (i = 0; i < info->numsrcs; i++) {
         if (info->srcids[i] == outID) {
            return i;
         }
      }
   }

   return -1;
}

static void set_current_input_source(px_mixer *Px, int i)
{
   PxInfo *info = (PxInfo *)Px->info;
   OSStatus err;

   if (i >= info->numsrcs) {
      return;
   }

   err = AudioDeviceSetProperty(info->input,
                                0,
                                0,
                                IS_INPUT,
                                kAudioDevicePropertyDataSource,
                                sizeof(UInt32),
                                &info->srcids[i]);

   return;
}

/*
|| Input volume
*/

static PxVolume get_input_volume(px_mixer *Px)
{
   PxInfo *info = (PxInfo *)Px->info;

   return get_volume(info->input, IS_INPUT);
}

static void set_input_volume(px_mixer *Px, PxVolume volume)
{
   PxInfo *info = (PxInfo *)Px->info;

   set_volume(info->input, IS_INPUT, volume);

   return;
}

/*
|| Playthrough
*/

static int supports_play_through(px_mixer *Px)
{
   return TRUE;
}

static PxVolume get_play_through(px_mixer *Px)
{
   PxInfo   *info = (PxInfo *)Px->info;
   OSStatus err;
   UInt32   outSize;
   UInt32   flag;

   outSize = sizeof(UInt32);
   err =  AudioDeviceGetProperty(info->output,
                                 0,
                                 IS_OUTPUT,
                                 kAudioDevicePropertyPlayThru,
                                 &outSize,
                                 &flag);
   if (err)
      return 0.0;
 
   if (flag)
      return 1.0;
   else
      return 0.0;
}

static void set_play_through(px_mixer *Px, PxVolume volume)
{
   PxInfo *info = (PxInfo *)Px->info;
   UInt32 flag = (volume > 0.01);
   OSStatus err;

   err =  AudioDeviceSetProperty(info->output,
                                 0,
                                 0,
                                 IS_OUTPUT,
                                 kAudioDevicePropertyPlayThru,
                                 sizeof(UInt32),
                                 &flag);
}
