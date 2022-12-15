#ifndef MIXER_H_
#define MIXER_H_

#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  0: audio0: audio volt range
  1: audio1: audio volt range
  2: audio2: audio volt range
  3: audio3: audio volt range
  4: volume cv: cv volt range

  Outputs:
  0: combination out: audio volt range

  Controls:
  0: volume
*/


#define MIXER_INCOUNT         5
#define MIXER_OUTCOUNT        1
#define MIXER_CONTROLCOUNT    1

#define MIXER_IN_PORT_AUDIO0  0
#define MIXER_IN_PORT_AUDIO1  1
#define MIXER_IN_PORT_AUDIO2  2
#define MIXER_IN_PORT_AUDIO3  3
#define MIXER_IN_PORT_VOL     4

#define MIXER_OUT_PORT_SUM    0

#define MIXER_CONTROL_VOL     0

///////////
// TYPES //
///////////

typedef struct Mixer
{
  Module module;

  // input ports
  R4 * inputPorts[MIXER_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * MIXER_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * MIXER_OUTCOUNT];

  R4 controlsCurr[MIXER_CONTROLCOUNT];
  R4 controlsPrev[MIXER_CONTROLCOUNT];
} Mixer;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * Mixer_init(void);

#endif
