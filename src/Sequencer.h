#ifndef SEQUENCER_H_
#define SEQUENCER_H_


#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  0: clock in: gate range [0v - 5v]

  Outputs:
  0: gate out: gate range [0v - 5v]
  1: trigger out: gate range [0v - 5v]
  2: pitch out: cv pitch range [0v - 5v]

  Controls:
  0: note0 on/off
  1: mote0 pitch
  .
  .
  .
  30: note15 on/off
  31: mote15 pitch

  32: note len
  33: seq len
  34: random chance

*/

#define SEQ_INCOUNT           1
#define SEQ_OUTCOUNT          3
// control count moved to bottom

#define SEQ_IN_PORT_CLKIN     0

#define SEQ_OUT_PORT_GATE     0
#define SEQ_OUT_PORT_TRIG     1
#define SEQ_OUT_PORT_PITCH    2

#define SEQ_NOTE_COUNT_TOTAL      16
#define SEQ_CONTROL_PER_NOTE      2
#define SEQ_OTHER_CONTROL_TOTAL   3

#define SEQ_CONTROL_NOTE_ON(n)    ((n) * SEQ_CONTROL_PER_NOTE)
#define SEQ_CONTROL_NOTE_PITCH(n) ((n) * SEQ_CONTROL_PER_NOTE + 1)

#define SEQ_CONTROL_NOTE_LEN  (SEQ_NOTE_COUNT_TOTAL * SEQ_CONTROL_PER_NOTE + 0)
#define SEQ_CONTROL_SEQ_LEN   (SEQ_NOTE_COUNT_TOTAL * SEQ_CONTROL_PER_NOTE + 1)
#define SEQ_CONTROL_RAND      (SEQ_NOTE_COUNT_TOTAL * SEQ_CONTROL_PER_NOTE + 2)

#define SEQ_CONTROLCOUNT      (SEQ_NOTE_COUNT_TOTAL * SEQ_CONTROL_PER_NOTE + SEQ_OTHER_CONTROL_TOTAL)

///////////
// TYPES //
///////////

typedef struct Sequencer
{
  Module module;

  // input ports
  R4 * inputPorts[SEQ_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * SEQ_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * SEQ_OUTCOUNT];

  R4 controlsCurr[SEQ_CONTROLCOUNT];
  R4 controlsPrev[SEQ_CONTROLCOUNT];

  U4 currentStepNum;
  R4 gateOpenTime;
  bool clockIsHigh;
  bool gateHigh;
} Sequencer;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * Sequencer_init(char* name);

#endif
