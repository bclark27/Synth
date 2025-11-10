#ifndef CLOCK_H_
#define CLOCK_H_

#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  None

  Outputs:
  0: clock out: gate range [0v - 5v]

  Controls:
  0: rate
*/

#define CLOCK_INCOUNT           0
#define CLOCK_OUTCOUNT          1
#define CLOCK_CONTROLCOUNT      1

#define CLOCK_OUT_PORT_CLOCK    0

#define CLOCK_CONTROL_RATE      0

#define CLK_MIDI_INCOUNT    0
#define CLK_MIDI_OUTCOUNT    0
#define CLK_MIDI_CONTROLCOUNT    0

///////////
// TYPES //
///////////

typedef struct Clock
{
  Module module;

  // input ports
  R4 * inputPorts[CLOCK_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * CLOCK_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * CLOCK_OUTCOUNT];

  R4 controlsCurr[CLOCK_CONTROLCOUNT];
  R4 controlsPrev[CLOCK_CONTROLCOUNT];

  // input ports
  MIDISataStream inputMIDIPorts[CLK_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[CLK_MIDI_OUTCOUNT];
  MIDIData outputMIDIPortsCurr[CLK_MIDI_OUTCOUNT];

  MIDIData midiControlsCurr[CLK_MIDI_CONTROLCOUNT];
  MIDIData midiControlsPrev[CLK_MIDI_CONTROLCOUNT];


  R4 phase;

} Clock;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * Clock_init(char* name);

#endif
