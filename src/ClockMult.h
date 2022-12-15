#ifndef CLOCK_MULT_H_
#define CLOCK_MULT_H_

#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  0: clock in: gate range [0v, 5v]

  Outputs:
  0: full: gate range [0v, 5v]
  1: half: gate range [0v, 5v]
  2: qurt: gate range [0v, 5v]
  3: eght: gate range [0v, 5v]
  4: sixt: gate range [0v, 5v]

  Controls:
  None
*/

#define CLKMULT_INCOUNT           1
#define CLKMULT_OUTCOUNT          5
#define CLKMULT_CONTROLCOUNT      0

#define CLKMULT_IN_PORT_CLKIN     0

#define CLKMULT_OUT_PORT_FULL     0
#define CLKMULT_OUT_PORT_HALF     1
#define CLKMULT_OUT_PORT_QURT     2
#define CLKMULT_OUT_PORT_EGHT     3
#define CLKMULT_OUT_PORT_SIXT     4

///////////
// TYPES //
///////////

typedef struct ClockMult
{
  Module module;

  // input ports
  R4 * inputPorts[CLKMULT_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * CLKMULT_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * CLKMULT_OUTCOUNT];

  R4 controlsCurr[CLKMULT_CONTROLCOUNT];
  R4 controlsPrev[CLKMULT_CONTROLCOUNT];

  U8 currCount;
  U8 lastHighToHighCount;

  U4 divisors[CLKMULT_OUTCOUNT];
  U4 dividedCounts[CLKMULT_OUTCOUNT];

  bool inClkIsHigh;

} ClockMult;

Module * ClockMult_init();

#endif
