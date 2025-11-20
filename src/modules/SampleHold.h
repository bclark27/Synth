#ifndef SAMPLEHOLD_H_
#define SAMPLEHOLD_H_

#include "Module.h"


#define SAMPLEHOLD_INCOUNT 9
#define SAMPLEHOLD_OUTCOUNT 4
#define SAMPLEHOLD_CONTROLCOUNT 0
#define SAMPLEHOLD_IN_PORT_TRIGGER	0
#define SAMPLEHOLD_IN_PORT_IN0	1
#define SAMPLEHOLD_IN_PORT_IN1	2
#define SAMPLEHOLD_IN_PORT_IN2	3
#define SAMPLEHOLD_IN_PORT_IN3	4
#define SAMPLEHOLD_IN_PORT_TRIGGER0	5
#define SAMPLEHOLD_IN_PORT_TRIGGER1	6
#define SAMPLEHOLD_IN_PORT_TRIGGER2	7
#define SAMPLEHOLD_IN_PORT_TRIGGER3	8

#define SAMPLEHOLD_OUT_PORT_OUT0	0
#define SAMPLEHOLD_OUT_PORT_OUT1	1
#define SAMPLEHOLD_OUT_PORT_OUT2	2
#define SAMPLEHOLD_OUT_PORT_OUT3	3


#define SAMPLEHOLD_MIDI_INCOUNT 0
#define SAMPLEHOLD_MIDI_OUTCOUNT 0




///////////
// TYPES //
///////////

typedef struct SampleHold
{
  Module module;

  // input ports
  R4 * inputPorts[SAMPLEHOLD_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * SAMPLEHOLD_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * SAMPLEHOLD_OUTCOUNT];

  R4 controlsCurr[SAMPLEHOLD_CONTROLCOUNT];
  R4 controlsPrev[SAMPLEHOLD_CONTROLCOUNT];

  // input ports
  MIDIData* inputMIDIPorts[SAMPLEHOLD_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[SAMPLEHOLD_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  MIDIData outputMIDIPortsCurr[SAMPLEHOLD_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];

  R4 lastTriggerVoltage[4];
  R4 holding[4];
} SampleHold;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * SampleHold_init(char* name);
void SampleHold_initInPlace(SampleHold * samplehold, char* name);

#endif