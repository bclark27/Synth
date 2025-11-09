#ifndef FILTER_H_
#define FILTER_H_

#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  0: audio: audio range [-5v - 5v]
  1: freq: cv mod [-10v - 10v]

  Outputs:
  0: audio out: audio range [-5v - 5v]

  Controls:
  0: frequency: cv mod [-10v - 10v]
  1: Q resonance: cv mod [-10v - 10v]
  2: db fitler type: cv mod [-10v - 10v]
*/

#define FITLER_INCOUNT          2
#define FITLER_OUTCOUNT         1
#define FITLER_CONTROLCOUNT     5

#define FITLER_IN_PORT_AUD      0
#define FITLER_IN_PORT_FREQ     1

#define FITLER_OUT_PORT_AUD     0

#define FITLER_CONTROL_FREQ     0
#define FITLER_CONTROL_Q        1
#define FITLER_CONTROL_DB       2
#define FITLER_CONTROL_ENV      3
#define FITLER_CONTROL_TYPE     4

///////////
// TYPES //
///////////

typedef struct Filter
{
  Module module;

  // input ports
  R4 * inputPorts[FITLER_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * FITLER_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * FITLER_OUTCOUNT];

  R4 controlsCurr[FITLER_CONTROLCOUNT];
  R4 controlsPrev[FITLER_CONTROLCOUNT];

  float b0, b1, b2, a1, a2;
  float x1, x2;  // past inputs
  float y1, y2;  // past outputs

} Filter;


Module * Filter_init(char* name);

#endif