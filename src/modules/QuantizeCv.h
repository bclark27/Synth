#ifndef QUANTIZE_CV_H_
#define QUANTIZE_CV_H_

#include "Module.h"


#define QUANTIZE_CV_INCOUNT 4
#define QUANTIZE_CV_OUTCOUNT 2
#define QUANTIZE_CV_CONTROLCOUNT 2
#define QUANTIZE_CV_IN_PORT_IN	0
#define QUANTIZE_CV_IN_PORT_SCALE	1
#define QUANTIZE_CV_IN_PORT_TRIGGER	2
#define QUANTIZE_CV_IN_PORT_TRANSPOSE	3

#define QUANTIZE_CV_OUT_PORT_OUT	0
#define QUANTIZE_CV_OUT_PORT_TRIGGER	1

#define QUANTIZE_CV_CONTROL_SCALE	0
#define QUANTIZE_CV_CONTROL_TRANSPOSE	1

#define QUANTIZE_CV_MIDI_INCOUNT 0
#define QUANTIZE_CV_MIDI_OUTCOUNT 0




///////////
// TYPES //
///////////

typedef enum QuantizeScale
{
  QuantizeScale_Octave,
  QuantizeScale_Chromatic,
  QuantizeScale_Major,
  QuantizeScale_Minor,
  QuantizeScale_MajorPenta,
  QuantizeScale_MainorPenta,
  QuantizeScale_InSen,
  QuantizeScale_Dorian,

  QuantizeScale_Count,
} QuantizeScale;

typedef struct QuantizeCv
{
  Module module;

  // input ports
  R4 * inputPorts[QUANTIZE_CV_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * QUANTIZE_CV_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * QUANTIZE_CV_OUTCOUNT];

  R4 controlsCurr[QUANTIZE_CV_CONTROLCOUNT];
  R4 controlsPrev[QUANTIZE_CV_CONTROLCOUNT];

  // input ports
  MIDIData* inputMIDIPorts[QUANTIZE_CV_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[QUANTIZE_CV_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  MIDIData outputMIDIPortsCurr[QUANTIZE_CV_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];

  R4 lastQuantize;
  R4 lastTriggerInputVolt;

} QuantizeCv;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * QuantizeCv_init(char* name);
void QuantizeCv_initInPlace(QuantizeCv * quantize, char* name);

#endif
