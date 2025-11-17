#ifndef SLEW_H_
#define SLEW_H_

#include "comm/Common.h"
#include "Module.h"


#define SLEW_INCOUNT 8
#define SLEW_OUTCOUNT 4
#define SLEW_CONTROLCOUNT 4
#define SLEW_IN_PORT_IN0	0
#define SLEW_IN_PORT_IN1	1
#define SLEW_IN_PORT_IN2	2
#define SLEW_IN_PORT_IN3	3
#define SLEW_IN_PORT_SLEW0	4
#define SLEW_IN_PORT_SLEW1	5
#define SLEW_IN_PORT_SLEW2	6
#define SLEW_IN_PORT_SLEW3	7

#define SLEW_OUT_PORT_OUT0	0
#define SLEW_OUT_PORT_OUT1	1
#define SLEW_OUT_PORT_OUT2	2
#define SLEW_OUT_PORT_OUT3	3

#define SLEW_CONTROL_SLEW0	0
#define SLEW_CONTROL_SLEW1	1
#define SLEW_CONTROL_SLEW2	2
#define SLEW_CONTROL_SLEW3	3

#define SLEW_MIDI_INCOUNT 0
#define SLEW_MIDI_OUTCOUNT 0




///////////
// TYPES //
///////////

typedef struct Slew
{
  Module module;

  // input ports
  R4 * inputPorts[SLEW_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * SLEW_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * SLEW_OUTCOUNT];

  R4 controlsCurr[SLEW_CONTROLCOUNT];
  R4 controlsPrev[SLEW_CONTROLCOUNT];

  // input ports
  MIDIData* inputMIDIPorts[SLEW_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[SLEW_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  MIDIData outputMIDIPortsCurr[SLEW_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];

  R4 prevSlew[4];
} Slew;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * Slew_init(char* name);
void Slew_initInPlace(Slew * slew, char* name);

#endif