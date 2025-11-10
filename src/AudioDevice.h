#ifndef AUDIO_DEVICE_H_
#define AUDIO_DEVICE_H_

#include "soundio/soundio.h"

///////////////
//  DEFINES  //
///////////////

///////////
// TYPES //
///////////

typedef struct AudioDevice
{
    struct SoundIo *soundio;
    struct SoundIoDevice* device;
    struct SoundIoOutStream *outstream;
} AudioDevice;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void AudioDevice_init(void);
void AudioDevice_LoopForever(void);

#endif