/*
 * aeffectx.h - simple header to allow VeSTige compilation and eventually work
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _AEFFECTX_H
#define _AEFFECTX_H

#define CCONST(a, b, c, d)( ( ( (int) a ) << 24 ) |		\
				( ( (int) b ) << 16 ) |		\
				( ( (int) c ) << 8 ) |		\
				( ( (int) d ) << 0 ) )

const int audioMasterAutomate = 0;
const int audioMasterVersion = 1;
const int audioMasterCurrentId = 2;
const int audioMasterIdle = 3;
const int audioMasterPinConnected = 4;
// unsupported? 5
const int audioMasterWantMidi = 6;
const int audioMasterGetTime = 7;
const int audioMasterProcessEvents = 8;
const int audioMasterSetTime = 9;
const int audioMasterTempoAt = 10;
const int audioMasterGetNumAutomatableParameters = 11;
const int audioMasterGetParameterQuantization = 12;
const int audioMasterIOChanged = 13;
const int audioMasterNeedIdle = 14;
const int audioMasterSizeWindow = 15;
const int audioMasterGetSampleRate = 16;
const int audioMasterGetBlockSize = 17;
const int audioMasterGetInputLatency = 18;
const int audioMasterGetOutputLatency = 19;
const int audioMasterGetPreviousPlug = 20;
const int audioMasterGetNextPlug = 21;
const int audioMasterWillReplaceOrAccumulate = 22;
const int audioMasterGetCurrentProcessLevel = 23;
const int audioMasterGetAutomationState = 24;
const int audioMasterOfflineStart = 25;
const int audioMasterOfflineRead = 26;
const int audioMasterOfflineWrite = 27;
const int audioMasterOfflineGetCurrentPass = 28;
const int audioMasterOfflineGetCurrentMetaPass = 29;
const int audioMasterSetOutputSampleRate = 30;
// unsupported? 31
const int audioMasterGetSpeakerArrangement = 31; // deprecated in 2.4?
const int audioMasterGetVendorString = 32;
const int audioMasterGetProductString = 33;
const int audioMasterGetVendorVersion = 34;
const int audioMasterVendorSpecific = 35;
const int audioMasterSetIcon = 36;
const int audioMasterCanDo = 37;
const int audioMasterGetLanguage = 38;
const int audioMasterOpenWindow = 39;
const int audioMasterCloseWindow = 40;
const int audioMasterGetDirectory = 41;
const int audioMasterUpdateDisplay = 42;
const int audioMasterBeginEdit = 43;
const int audioMasterEndEdit = 44;
const int audioMasterOpenFileSelector = 45;
const int audioMasterCloseFileSelector = 46; // currently unused
const int audioMasterEditFile = 47; // currently unused
const int audioMasterGetChunkFile = 48; // currently unused
const int audioMasterGetInputSpeakerArrangement = 49; // currently unused

const int effFlagsHasEditor = 1;
const int effFlagsCanReplacing = 1 << 4; // very likely
const int effFlagsProgramChunks = 1 << 5; // from Ardour
const int effFlagsIsSynth = 1 << 8; // currently unused

const int effOpen = 0;
const int effClose = 1; // currently unused
const int effSetProgram = 2; // currently unused
const int effGetProgram = 3; // currently unused
// The next one was gleaned from http://www.kvraudio.com/forum/viewtopic.php?p=1905347
const int effSetProgramName = 4;
const int effGetProgramName = 5; // currently unused
// The next two were gleaned from http://www.kvraudio.com/forum/viewtopic.php?p=1905347
const int effGetParamLabel = 6;
const int effGetParamDisplay = 7;
const int effGetParamName = 8; // currently unused
const int effSetSampleRate = 10;
const int effSetBlockSize = 11;
const int effMainsChanged = 12;
const int effEditGetRect = 13;
const int effEditOpen = 14;
const int effEditClose = 15;
const int effEditIdle = 19;
const int effEditTop = 20;
const int effGetChunk = 23; // from Ardour
const int effSetChunk = 24; // from Ardour
const int effProcessEvents = 25;
// The next one was gleaned from http://www.kvraudio.com/forum/viewtopic.php?p=1905347
const int effGetProgramNameIndexed = 29;
const int effGetEffectName = 45;
const int effGetParameterProperties = 56; // missing
const int effGetVendorString = 47;
const int effGetProductString = 48;
const int effGetVendorVersion = 49;
const int effCanDo = 51; // currently unused
const int effGetVstVersion = 58; // currently unused

const int kEffectMagic = CCONST( 'V', 's', 't', 'P' );
const int kVstLangEnglish = 1;
const int kVstMidiType = 1;
const int kVstParameterUsesFloatStep = 1 << 2;
const int kVstTempoValid = 1 << 10;
const int kVstTransportPlaying = 1 << 1;


class remoteVstPlugin;


class VstMidiEvent
{
public:
	// 00
	int type;
	// 04
	int byteSize;
	// 08
	int deltaFrames;
	// 0c?
	int flags;
	// 10?
	int noteLength;
	// 14?
	int noteOffset;
	// 18
	char midiData[4];
	// 1c?
	char detune;
	// 1d?
	char noteOffVelocity;
	// 1e?
	char reserved1;
	// 1f?
	char reserved2;

} ;




class VstEvent
{
	char dump[sizeof( VstMidiEvent )];

} ;




class VstEvents
{
public:
	// 00
	int numEvents;
	// 04
	int reserved;
	// 08
	VstEvent * events;

} ;




// Not finished, neither really used
class VstParameterProperties
{
public:
	float stepFloat;
	char label[64];
	int flags;
	int minInteger;
	int maxInteger;
	int stepInteger;
	char shortLabel[8];
	int category;
	char categoryLabel[24];
	char empty[128];

} ;


#include <stdint.h>

class AEffect
{
public:
	// Never use virtual functions!!!
	// 00-03
	int magic;
	// dispatcher 04-07
	int (* dispatcher)( AEffect * , int , int , int , void * , float );
	// process, quite sure 08-0b
	void (* process)( AEffect * , float * * , float * * , int );
	// setParameter 0c-0f
	void (* setParameter)( AEffect * , int , float );
	// getParameter 10-13
	float (* getParameter)( AEffect * , int );
	// programs 14-17
	int numPrograms;
	// Params 18-1b
	int numParams;
	// Input 1c-1f
	int numInputs;
	// Output 20-23
	int numOutputs;
	// flags 24-27
	int flags;
	// Fill somewhere 28-2b
	void * user;
	// Zeroes 2c-2f 30-33 34-37 38-3b
	char empty3[4 + 4 + 4 + 4];
	// 1.0f 3c-3f
	float unkown_float;
	// An object? pointer 40-43
	char empty4[4];
	// Zeroes 44-47
	char empty5[4];
	// Id 48-4b
	int32_t uniqueID;
	// Don't know 4c-4f
	char unknown1[4];
	// processReplacing 50-53
	void (* processReplacing)( AEffect * , float * * , float * * , int );

} ;


class VstTimeInfo
{
public:
	// 00
	double samplePos;
	// 08
	double sampleRate;
	// unconfirmed 10 18
	char empty1[8 + 8];
	// 20?
	double tempo;
	// unconfirmed 28 30 38
	char empty2[8 + 8 + 8];
	// 40?
	int timeSigNumerator;
	// 44?
	int timeSigDenominator;
	// unconfirmed 48 4c 50
	char empty3[4 + 4 + 4];
	// 54
	int flags;

} ;




typedef long int (* audioMasterCallback)( AEffect * , long int , long int ,
						long int , void * , float );
// we don't use it, may be noise
#define VSTCALLBACK




#endif
