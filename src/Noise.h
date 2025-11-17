#ifndef NOISE_H_
#define NOISE_H_

#include "comm/Common.h"
#include "Module.h"


#define NOISE_INCOUNT 1
#define NOISE_OUTCOUNT 3
#define NOISE_CONTROLCOUNT 4
#define NOISE_IN_PORT_CLK	0

#define NOISE_OUT_PORT_WHITE	0
#define NOISE_OUT_PORT_COLORED	1
#define NOISE_OUT_PORT_RANDOM	2

#define NOISE_CONTROL_NOISE_LEVEL	0
#define NOISE_CONTROL_COLORED_LOW	1
#define NOISE_CONTROL_COLORED_HIGH	2
#define NOISE_CONTROL_RANDOM_LEVEL	3

#define NOISE_MIDI_INCOUNT 0
#define NOISE_MIDI_OUTCOUNT 0




///////////
// TYPES //
///////////

typedef struct Noise
{
  Module module;

  // input ports
  R4 * inputPorts[NOISE_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * NOISE_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * NOISE_OUTCOUNT];

  R4 controlsCurr[NOISE_CONTROLCOUNT];
  R4 controlsPrev[NOISE_CONTROLCOUNT];

  // input ports
  MIDIData* inputMIDIPorts[NOISE_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[NOISE_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  MIDIData outputMIDIPortsCurr[NOISE_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];

  uint32_t noiseState;
  R4 lastClockInSampleVoltage;
  unsigned long long int currentClockSampleLength;
  unsigned long long int currentClockSampleCount;
  R4 currentRandomLevel;
} Noise;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * Noise_init(char* name);
void Noise_initInPlace(Noise * noise, char* name);

#endif