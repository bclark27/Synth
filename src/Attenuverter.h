#ifndef ATTENUVERTER_H_
#define ATTENUVERTER_H_

#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  0: attenuvert: cv mod [-10v - 10v]
  1: sig: [-?v - ?v]

  Outputs:
  0: sig out: [-?v - ?v]

  Controls:
  0: attn [-10v, +10v]
*/

#define ATTN_INCOUNT        2
#define ATTN_OUTCOUNT       1
#define ATTN_CONTROLCOUNT   2

#define ATTN_IN_PORT_ATTN    0
#define ATTN_IN_PORT_SIG    1

#define ATTN_OUT_PORT_SIG   0

#define ATTN_CONTROL_ATTN    0
#define ATTN_CONTROL_DCOFF    1

#define ATTN_MIDI_INCOUNT    0
#define ATTN_MIDI_OUTCOUNT    0
#define ATTN_MIDI_CONTROLCOUNT    0

///////////
// TYPES //
///////////

typedef struct Attenuverter
{
  Module module;

  // input ports
  R4 * inputPorts[ATTN_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * ATTN_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * ATTN_OUTCOUNT];

  R4 controlsCurr[ATTN_CONTROLCOUNT];
  R4 controlsPrev[ATTN_CONTROLCOUNT];

  // input ports
  MIDISataStream inputMIDIPorts[ATTN_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[ATTN_MIDI_OUTCOUNT];
  MIDIData outputMIDIPortsCurr[ATTN_MIDI_OUTCOUNT];

  MIDIData midiControlsCurr[ATTN_MIDI_CONTROLCOUNT];
  MIDIData midiControlsPrev[ATTN_MIDI_CONTROLCOUNT];


} Attenuverter;


Module * Attenuverter_init(char* name);
void Attenuverter_initInPlace(Attenuverter* attn, char* name);

#endif
