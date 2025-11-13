#ifndef FILTER_H_
#define FILTER_H_

#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  0: audio: audio range [-5v - 5v]
  1: freq: cv mod [0v - 10v]

  Outputs:
  0: audio out: audio range [-5v - 5v]

  Controls:
  0: frequency: cv mod [0v - 10v]
  1: Q resonance: cv mod [0v - 10v]
  2: db: cv mod [0v - 10v]
  3: env : cv mod [0v - 10v]
  4: type: cv mod [0v - 10v]
*/

#define FILTER_INCOUNT          2
#define FILTER_OUTCOUNT         1
#define FILTER_CONTROLCOUNT     5

#define FILTER_IN_PORT_AUD      0
#define FILTER_IN_PORT_FREQ     1

#define FILTER_OUT_PORT_AUD     0

#define FILTER_CONTROL_FREQ     0
#define FILTER_CONTROL_Q        1
#define FILTER_CONTROL_DB       2
#define FILTER_CONTROL_ENV      3
#define FILTER_CONTROL_TYPE     4

#define FILTER_MIDI_INCOUNT    0
#define FILTER_MIDI_OUTCOUNT    0
#define FILTER_MIDI_CONTROLCOUNT    0

///////////
// TYPES //
///////////

typedef struct Filter
{
  Module module;

  // input ports
  R4 * inputPorts[FILTER_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * FILTER_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * FILTER_OUTCOUNT];

  R4 controlsCurr[FILTER_CONTROLCOUNT];
  R4 controlsPrev[FILTER_CONTROLCOUNT];

  // input ports
  MIDISataStream inputMIDIPorts[FILTER_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[FILTER_MIDI_OUTCOUNT];
  MIDIData outputMIDIPortsCurr[FILTER_MIDI_OUTCOUNT];

  MIDIData midiControlsCurr[FILTER_MIDI_CONTROLCOUNT];
  MIDIData midiControlsPrev[FILTER_MIDI_CONTROLCOUNT];


  float b0, b1, b2, a1, a2;
  float x1, x2;  // past inputs
  float y1, y2;  // past outputs

} Filter;


Module * Filter_init(char* name);

#endif