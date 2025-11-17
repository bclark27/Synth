#ifndef QUANTIZE_H_
#define QUANTIZE_H_

#include "comm/Common.h"
#include "Module.h"


#define QUANTIZE_INCOUNT 4
#define QUANTIZE_OUTCOUNT 2
#define QUANTIZE_CONTROLCOUNT 2
#define QUANTIZE_IN_PORT_IN	0
#define QUANTIZE_IN_PORT_SCALE	1
#define QUANTIZE_IN_PORT_TRIGGER	2
#define QUANTIZE_IN_PORT_TRANSPOSE	3

#define QUANTIZE_OUT_PORT_OUT	0
#define QUANTIZE_OUT_PORT_TRIGGER	1

#define QUANTIZE_CONTROL_SCALE	0
#define QUANTIZE_CONTROL_TRANSPOSE	1

#define QUANTIZE_MIDI_INCOUNT 0
#define QUANTIZE_MIDI_OUTCOUNT 0




///////////
// TYPES //
///////////

typedef enum QuantizeScale
{
    QuantizeScale_Chromatic,
    QuantizeScale_Major,
    QuantizeScale_Minor,
    QuantizeScale_MajorPenta,
    QuantizeScale_MainorPenta,
    QuantizeScale_InSen,
    QuantizeScale_Dorian,

    QuantizeScale_Count,
} QuantizeScale;

typedef struct Quantize
{
  Module module;

  // input ports
  R4 * inputPorts[QUANTIZE_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * QUANTIZE_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * QUANTIZE_OUTCOUNT];

  R4 controlsCurr[QUANTIZE_CONTROLCOUNT];
  R4 controlsPrev[QUANTIZE_CONTROLCOUNT];

  // input ports
  MIDIData* inputMIDIPorts[QUANTIZE_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[QUANTIZE_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  MIDIData outputMIDIPortsCurr[QUANTIZE_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];

} Quantize;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * Quantize_init(char* name);
void Quantize_initInPlace(Quantize * quantize, char* name);

#endif
