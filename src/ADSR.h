#ifndef ADSR_H_
#define ADSR_H_

#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  0: gate in: gate range [0v, 5v]
  1: attack: mod cv range [0v, 10v]
  2: decay: mod cv range [0v, 10v]
  3: sustain: mod cv range [0v, 10v]
  4: release: mod cv range [0v, 10v]

  Outputs:
  0: envelope: mod cv range [0v, 10v] will flip upside down if attenuverted
  1: envelope: mod cv range [-10v, 0v] will flip upside down if attenuverted

  Controls:
  0: attack [0v, +10v]
  1: decay [0v, +10v]
  2: sustain [0v, +10v]
  3: release [0v, +10v]
  4: attnuvert [-10v, +10v]
*/

#define ADSR_INCOUNT            7
#define ADSR_OUTCOUNT           2
#define ADSR_CONTROLCOUNT       5

#define ADSR_IN_PORT_GATE       0
#define ADSR_IN_PORT_TRIG       1
#define ADSR_IN_PORT_A          2
#define ADSR_IN_PORT_D          3
#define ADSR_IN_PORT_S          4
#define ADSR_IN_PORT_R          5
#define ADSR_IN_PORT_ATTN       6

#define ADSR_OUT_PORT_ENV       0
#define ADSR_OUT_PORT_ENVINV       1

#define ADSR_CONTROL_A          0
#define ADSR_CONTROL_D          1
#define ADSR_CONTROL_S          2
#define ADSR_CONTROL_R          3
#define ADSR_CONTROL_ATTN       4

#define ADSR_MIDI_INCOUNT    0
#define ADSR_MIDI_OUTCOUNT    0
#define ADSR_MIDI_CONTROLCOUNT    0

///////////
// TYPES //
///////////

typedef enum ADSR_section
{
  ADSR_ASection,
  ADSR_DSection,
  ADSR_SSection,
  ADSR_RSection,

} ADSR_section;

typedef struct ADSR
{
  Module module;

  // input ports
  R4 * inputPorts[ADSR_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * ADSR_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * ADSR_OUTCOUNT];

  R4 controlsCurr[ADSR_CONTROLCOUNT];
  R4 controlsPrev[ADSR_CONTROLCOUNT];

  // input ports
  MIDISataStream inputMIDIPorts[ADSR_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[ADSR_MIDI_OUTCOUNT];
  MIDIData outputMIDIPortsCurr[ADSR_MIDI_OUTCOUNT];

  MIDIData midiControlsCurr[ADSR_MIDI_CONTROLCOUNT];
  MIDIData midiControlsPrev[ADSR_MIDI_CONTROLCOUNT];


  ADSR_section section;
  R4 timeSinceSectionStart;
  R4 prevSampleValue;
  R4 releaseStartVal;
  R4 prevADSRStop;
  R4 prevADSRVal;

  bool isHeld;
  bool envelopeActive;

} ADSR;

Module * ADSR_init(char* name);
void ADSR_initInPlace(ADSR* adsr, char* name);

#endif
