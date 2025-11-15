#ifndef LFO_H_
#define LFO_H_

#include "comm/Common.h"
#include "Oscillator.h"
#include "Module.h"


#define LFO_INCOUNT 3
#define LFO_OUTCOUNT 2
#define LFO_CONTROLCOUNT 5
#define LFO_IN_PORT_FREQ	0
#define LFO_IN_PORT_CLK	1
#define LFO_IN_PORT_PW	2

#define LFO_OUT_PORT_SIG	0
#define LFO_OUT_PORT_CLK	1

#define LFO_CONTROL_FREQ	0
#define LFO_CONTROL_PW	1
#define LFO_CONTROL_MIN	2
#define LFO_CONTROL_MAX	3
#define LFO_CONTROL_WAVE	4

#define LFO_MIDI_INCOUNT 1
#define LFO_MIDI_OUTCOUNT 1

#define LFO_IN_MIDI_PORT_MIDI_asdasd	0

#define LFO_OUT_MIDI_PORT_MIDI_okok	0


///////////
// TYPES //
///////////

typedef struct LFO
{
  Module module;

  // input ports
  R4 * inputPorts[LFO_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * LFO_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * LFO_OUTCOUNT];

  R4 controlsCurr[LFO_CONTROLCOUNT];
  R4 controlsPrev[LFO_CONTROLCOUNT];

  // input ports
  MIDIData* inputMIDIPorts[LFO_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[LFO_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  MIDIData outputMIDIPortsCurr[LFO_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];

  Oscillator osc;

  long long lastClockAge;
  R4 currentClockFreq;
  R4 lastSampleVoltage;
  bool lastUpdateHadNoClockPort;

} LFO;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * LFO_init(char* name);
void LFO_initInPlace(LFO * lfo, char* name);

#endif