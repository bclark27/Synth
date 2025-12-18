#ifndef SAMPLER_H_
#define SAMPLER_H_

#include "Module.h"

#include "ADSR.h"

#define SAMPLER_MAX_LOOP_SECONDS 10
#define SAMPLER_MAX_RECORD_SAMPLES (SAMPLER_MAX_LOOP_SECONDS * SAMPLE_RATE)

#define SAMPLER_INCOUNT 8
#define SAMPLER_OUTCOUNT 1
#define SAMPLER_CONTROLCOUNT 11
#define SAMPLER_BYTE_CONTROLCOUNT 1
#define SAMPLER_MIDI_CONTROLCOUNT 0
#define SAMPLER_IN_PORT_Pitch	0
#define SAMPLER_IN_PORT_Gate	1
#define SAMPLER_IN_PORT_Attack	2
#define SAMPLER_IN_PORT_Decay	3
#define SAMPLER_IN_PORT_Sustain	4
#define SAMPLER_IN_PORT_Release	5
#define SAMPLER_IN_PORT_RecordGate	6
#define SAMPLER_IN_PORT_RecordAudio	7

#define SAMPLER_OUT_PORT_Audio	0

#define SAMPLER_CONTROL_Pitch	0
#define SAMPLER_CONTROL_Attack	1
#define SAMPLER_CONTROL_Decay	2
#define SAMPLER_CONTROL_Sustain	3
#define SAMPLER_CONTROL_Release	4
#define SAMPLER_CONTROL_Source	5
#define SAMPLER_CONTROL_PlaybackMode	6
#define SAMPLER_CONTROL_PlaybackStart	7
#define SAMPLER_CONTROL_LoopStart	8
#define SAMPLER_CONTROL_LoopEnd	9
#define SAMPLER_CONTROL_CrossFade	10

#define SAMPLER_BYTE_CONTROL_File	0


#define SAMPLER_MIDI_INCOUNT 1
#define SAMPLER_MIDI_OUTCOUNT 0

#define SAMPLER_IN_MIDI_PORT_Midi	0



///////////
// TYPES //
///////////

typedef struct Sampler
{
  Module module;

  // input ports
  R4 * inputPorts[SAMPLER_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * SAMPLER_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * SAMPLER_OUTCOUNT];

  R4 controlsCurr[SAMPLER_CONTROLCOUNT];
  R4 controlsPrev[SAMPLER_CONTROLCOUNT];

  // input ports
  MIDIData* inputMIDIPorts[SAMPLER_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[SAMPLER_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  MIDIData outputMIDIPortsCurr[SAMPLER_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  
  atomic_uint midiRingRead[SAMPLER_MIDI_CONTROLCOUNT];
  atomic_uint midiRingWrite[SAMPLER_MIDI_CONTROLCOUNT];
  MIDIData midiControlsRingBuffer[SAMPLER_MIDI_CONTROLCOUNT * MIDI_STREAM_BUFFER_SIZE];

  // byte array type storage
  atomic_bool byteArrayLock[SAMPLER_BYTE_CONTROLCOUNT];
  volatile bool byteArrayDirty[SAMPLER_BYTE_CONTROLCOUNT];
  volatile unsigned int byteArrayLen[SAMPLER_BYTE_CONTROLCOUNT];
  char* byteArrayControlStorage[SAMPLER_BYTE_CONTROLCOUNT];

  // audio data from the actual file
  struct {
    float* data;       // interleaved if stereo
    int frames;        // number of samples per channel
    int channels;      // 1 = mono, 2 = stereo
    int sampleRate;
  } audioBuffer;

  // recorded audio data from listening to input ports
  R4 portRecordedAudio[SAMPLER_MAX_RECORD_SAMPLES];
  U4 portRecordedAudioLength;
  U4 portRecordedAudioStartHead;
  
  // loop and audio playback indexes. can be common to both file and internal audio recording
  U4 playbackStartIdx;
  U4 playbackEndIdx;
  U4 playbackCurrIdx;

  ADSR adsr;

  R4 lastRecordGateValue;
  R4 lastPlaybackGateValue;
} Sampler;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * Sampler_init(char* name);
void Sampler_initInPlace(Sampler * sampler, char* name);

#endif


