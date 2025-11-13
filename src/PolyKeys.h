#ifndef POLY_KEYS_H_
#define POLY_KEYS_H_


#include "comm/Common.h"
#include "Module.h"

#include "VCO.h"
#include "ADSR.h"
#include "Filter.h"
#include "Attenuverter.h"

///////////////
//  DEFINES  //
///////////////

/*
*/
#define POLYKEYS_MAX_VOICES 5

#define POLYKEYS_INCOUNT           0
#define POLYKEYS_OUTCOUNT          1
#define POLYKEYS_CONTROLCOUNT      9

#define POLYKEYS_OUTPORT_AUDIO      0

#define POLYKEYS_CONTROL_A  0
#define POLYKEYS_CONTROL_D  1
#define POLYKEYS_CONTROL_S  2
#define POLYKEYS_CONTROL_R  3
#define POLYKEYS_CONTROL_FILTER_ENV_AMT 4
#define POLYKEYS_CONTROL_FILTER_FREQ 5
#define POLYKEYS_CONTROL_FILTER_Q 6
#define POLYKEYS_CONTROL_UNISON 7
#define POLYKEYS_CONTROL_DETUNE 8


#define POLYKEYS_MIDI_INCOUNT    1
#define POLYKEYS_MIDI_OUTCOUNT    0
#define POLYKEYS_MIDI_CONTROLCOUNT    0

#define POLYKEYS_MIDI_INPUT_MIDIIN  0

///////////
// TYPES //
///////////

typedef struct PolyKeysVoice
{
    VCO * vco;
    Filter * flt;
    ADSR * adsr;
    Attenuverter * attn;
    VoltStream voiceOutputBuffer;
    Volt voiceInputFreqBuffer[MIDI_STREAM_BUFFER_SIZE];

    U8 noteAge;
    R4 velocityAmplitudeMultiplier;
    Volt noteFreq;
    U1 note;
    U1 startVelocity;
    bool noteIsOn;
    bool adsrActive; // get this from the adsr for this voice. also this tells us if the note is totally done. if this is false then we dont have to do any audio processing for this guy
    bool firstOnSignal;
} PolyKeysVoice;

typedef struct PolyKeys
{
  Module module;

  // input ports
  R4 * inputPorts[POLYKEYS_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * POLYKEYS_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * POLYKEYS_OUTCOUNT];

  R4 controlsCurr[POLYKEYS_CONTROLCOUNT];
  R4 controlsPrev[POLYKEYS_CONTROLCOUNT];

  // input ports
  MIDIData* inputMIDIPorts[POLYKEYS_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[POLYKEYS_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  MIDIData outputMIDIPortsCurr[POLYKEYS_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];

  atomic_uint midiRingRead[POLYKEYS_MIDI_CONTROLCOUNT];
  atomic_uint midiRingWrite[POLYKEYS_MIDI_CONTROLCOUNT];
  MIDIData midiControlsRingBuffer[POLYKEYS_MIDI_CONTROLCOUNT * MIDI_STREAM_BUFFER_SIZE];

  PolyKeysVoice voices[POLYKEYS_MAX_VOICES];

} PolyKeys;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * PolyKeys_init(char* name);

#endif