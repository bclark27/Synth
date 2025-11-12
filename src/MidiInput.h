#ifndef MIDI_INPUT_H_
#define MIDI_INPUT_H_


#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  None

  Outputs:
  0: midi messages

  Controls:
  0: input from midi controller
*/

#define MIDIINPUT_INCOUNT           0
#define MIDIINPUT_OUTCOUNT          0
#define MIDIINPUT_CONTROLCOUNT      0

#define MIDIINPUT_MIDI_INCOUNT    0
#define MIDIINPUT_MIDI_OUTCOUNT    1
#define MIDIINPUT_MIDI_CONTROLCOUNT    1

#define MIDIINPUT_MIDI_CONTROL_INPUT    0

#define MIDIINPUT_MIDI_OUTPUT_OUTPUT    0

//#define MIDIINPUT_FIRST_MIDI_OUTPUT_ID  0
//#define MIDIINPUT_FIRST_MIDI_CONTROL_ID  0

///////////
// TYPES //
///////////

typedef struct MidiInput
{
  Module module;

  // input ports
  R4 * inputPorts[MIDIINPUT_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * MIDIINPUT_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * MIDIINPUT_OUTCOUNT];

  R4 controlsCurr[MIDIINPUT_CONTROLCOUNT];
  R4 controlsPrev[MIDIINPUT_CONTROLCOUNT];

  // input ports
  MIDISataStream inputMIDIPorts[MIDIINPUT_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[MIDIINPUT_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  MIDIData outputMIDIPortsCurr[MIDIINPUT_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];

  atomic_uint midiRingRead[MIDIINPUT_MIDI_CONTROLCOUNT];
  atomic_uint midiRingWrite[MIDIINPUT_MIDI_CONTROLCOUNT];
  MIDIData midiControlsRingBuffer[MIDIINPUT_MIDI_CONTROLCOUNT * MIDI_STREAM_BUFFER_SIZE];

  R4 phase;

} MidiInput;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * MidiInput_init(char* name);


#endif