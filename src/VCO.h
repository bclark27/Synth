#ifndef VCO_H_
#define VCO_H_

#include "comm/Common.h"
#include "Oscillator.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  0: freq: pitch cv range [0v - 5v]
  1: pulse width: cv mod range [-10v - 10v]

  Outputs:
  0: sin wave: audio volt range
  1: saw wave: audio volt range
  2: sqr wave: audio volt range
  3: tri wave: audio volt range

  Controls:
  0: freq
  1: pulse width
*/

#define VCO_INCOUNT         2
#define VCO_OUTCOUNT        4
#define VCO_CONTROLCOUNT    2

#define VCO_IN_PORT_FREQ    0
#define VCO_IN_PORT_PW      1

#define VCO_OUT_PORT_SIN    0
#define VCO_OUT_PORT_SAW    1
#define VCO_OUT_PORT_SQR    2
#define VCO_OUT_PORT_TRI    3

#define VCO_CONTROL_FREQ    0
#define VCO_CONTROL_PW      1

///////////
// TYPES //
///////////

typedef struct VCO
{
  Module module;

  // input ports
  R4 * inputPorts[VCO_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * VCO_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * VCO_OUTCOUNT];

  R4 controlsCurr[VCO_CONTROLCOUNT];
  R4 controlsPrev[VCO_CONTROLCOUNT];

  Oscillator oscSin;
  Oscillator oscSqr;
  Oscillator oscSaw;
  Oscillator oscTri;
  Oscillator oscWht;

} VCO;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * VCO_init(void);

#endif
