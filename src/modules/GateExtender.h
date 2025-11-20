#ifndef GATEEXTENDER_H_
#define GATEEXTENDER_H_

#include "Module.h"


#define GATEEXTENDER_INCOUNT 2
#define GATEEXTENDER_OUTCOUNT 1
#define GATEEXTENDER_CONTROLCOUNT 1
#define GATEEXTENDER_IN_PORT_IN	0
#define GATEEXTENDER_IN_PORT_LENGTH	1

#define GATEEXTENDER_OUT_PORT_OUT	0

#define GATEEXTENDER_CONTROL_LENGTH	0

#define GATEEXTENDER_MIDI_INCOUNT 0
#define GATEEXTENDER_MIDI_OUTCOUNT 0




///////////
// TYPES //
///////////

typedef struct GateExtender
{
  Module module;

  // input ports
  R4 * inputPorts[GATEEXTENDER_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * GATEEXTENDER_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * GATEEXTENDER_OUTCOUNT];

  R4 controlsCurr[GATEEXTENDER_CONTROLCOUNT];
  R4 controlsPrev[GATEEXTENDER_CONTROLCOUNT];

  // input ports
  MIDIData* inputMIDIPorts[GATEEXTENDER_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[GATEEXTENDER_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  MIDIData outputMIDIPortsCurr[GATEEXTENDER_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];

  R4 lastInputVoltage;
  unsigned long long samplesSinceLastTrigger;
  bool gateHighNext;

} GateExtender;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * GateExtender_init(char* name);
void GateExtender_initInPlace(GateExtender * gateextender, char* name);

#endif
