#ifndef OUTPUTMODULE_H_
#define OUTPUTMODULE_H_

#include "Module.h"

///////////////
//  DEFINES  //
///////////////

#define OUT_MODULE_IDX  0
#define OUT_MODULE_ID   0

/*
  Inputs:
  0: audioLeft: audio volt range
  1: audioRight: audio volt range

  Outputs:
  0: audioLeft: [-1, 1]
  1: audioRight: [-1, 1]

  Controls:
  None
*/

#define OUTPUT_INCOUNT        2
#define OUTPUT_OUTCOUNT       2
#define OUTPUT_CONTROLCOUNT   0

#define OUTPUT_IN_PORT_LEFT   0
#define OUTPUT_IN_PORT_RIGHT  1

#define OUTPUT_OUT_PORT_LEFT  0
#define OUTPUT_OUT_PORT_RIGHT 1

#define OUTPUT_MIDI_INCOUNT    0
#define OUTPUT_MIDI_OUTCOUNT    0
#define OUTPUT_MIDI_CONTROLCOUNT    0


///////////
// TYPES //
///////////

typedef struct OutputModule
{
  Module module;

  // input ports
  R4 * inputPorts[OUTPUT_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * OUTPUT_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * OUTPUT_OUTCOUNT];

  R4 controlsCurr[OUTPUT_CONTROLCOUNT];
  R4 controlsPrev[OUTPUT_CONTROLCOUNT];

  // input ports
  MIDISataStream inputMIDIPorts[OUTPUT_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[OUTPUT_MIDI_OUTCOUNT];
  MIDIData outputMIDIPortsCurr[OUTPUT_MIDI_OUTCOUNT];

  MIDIData midiControlsCurr[OUTPUT_MIDI_CONTROLCOUNT];
  MIDIData midiControlsPrev[OUTPUT_MIDI_CONTROLCOUNT];

} OutputModule;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * OutputModule_init(char* name);


#endif
