#ifndef VCO_H_
#define VCO_H_

#include "comm/Common.h"
#include "Oscillator.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  0: freq: pitch cv range [0v - 5v]
  1: pulse width: cv mod range [-10v - 10v]

  Outputs:
  0: audio: audio volt range

  Controls:
  0: freq
  1: pulse width
  2: waveform [0 - 1] for sin tri sqr saw
*/

#define VCO_INCOUNT         2
#define VCO_OUTCOUNT        1
#define VCO_CONTROLCOUNT    5

#define VCO_IN_PORT_FREQ    0
#define VCO_IN_PORT_PW      1

#define VCO_OUT_PORT_AUD    0

#define VCO_CONTROL_FREQ    0
#define VCO_CONTROL_PW      1
#define VCO_CONTROL_WAVE    2
#define VCO_CONTROL_UNI     3
#define VCO_CONTROL_DET     4

#define VCO_MIDI_INCOUNT    0
#define VCO_MIDI_OUTCOUNT    0
#define VCO_MIDI_CONTROLCOUNT    0

///////////
// TYPES //
///////////

typedef struct VCO
{
  Module module;

  // input ports
  VoltStream inputPorts[VCO_INCOUNT];

  // output ports
  Volt outputPortsPrev[MODULE_BUFFER_SIZE * VCO_OUTCOUNT];
  Volt outputPortsCurr[MODULE_BUFFER_SIZE * VCO_OUTCOUNT];

  Volt controlsCurr[VCO_CONTROLCOUNT];
  Volt controlsPrev[VCO_CONTROLCOUNT];

  // input ports
  MIDISataStream inputMIDIPorts[VCO_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[VCO_MIDI_OUTCOUNT];
  MIDIData outputMIDIPortsCurr[VCO_MIDI_OUTCOUNT];

  MIDIData midiControlsCurr[VCO_MIDI_CONTROLCOUNT];
  MIDIData midiControlsPrev[VCO_MIDI_CONTROLCOUNT];

  Oscillator osc;

} VCO;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * VCO_init(char* name);

#endif
