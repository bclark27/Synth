#ifndef OUTPUTMODULE_H_
#define OUTPUTMODULE_H_

#include "comm/Common.h"

///////////
// TYPES //
///////////

typedef struct OutputModule
{
  U4 sampleRate;
  U4 streamBufferSize;
} OutputModule;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void OutputModule_init(U4 sampleRate, U4 streamBufferSize);


#endif
