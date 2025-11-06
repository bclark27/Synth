#ifndef ATTENUATOR_H_
#define ATTENUATOR_H_

#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  0: volume: cv mod [-10v - 10v]
  1: audio: audio range [-5v - 5v]

  Outputs:
  0: audio out: audio range [-5v - 5v]

  Controls:
  0: volume
*/

#define ATTN_INCOUNT        2
#define ATTN_OUTCOUNT       1
#define ATTN_CONTROLCOUNT   1

#define ATTN_IN_PORT_VOL    0
#define ATTN_IN_PORT_AUD    1

#define ATTN_OUT_PORT_AUD   0

#define ATTN_CONTROL_VOL    0

///////////
// TYPES //
///////////

typedef struct Attenuator
{
  Module module;

  // input ports
  R4 * inputPorts[ATTN_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * ATTN_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * ATTN_OUTCOUNT];

  R4 controlsCurr[ATTN_CONTROLCOUNT];
  R4 controlsPrev[ATTN_CONTROLCOUNT];

} Attenuator;


Module * Attenuator_init(char* name);

#endif
