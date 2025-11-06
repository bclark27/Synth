#ifndef ADSR_H_
#define ADSR_H_

#include "comm/Common.h"
#include "Module.h"

///////////////
//  DEFINES  //
///////////////

/*
  Inputs:
  0: gate in: gate range [0v, 5v]
  1: attack: mod cv range [-10v, 10v]
  2: decay: mod cv range [-10v, 10v]
  3: sustain: mod cv range [-10v, 10v]
  4: release: mod cv range [-10v, 10v]

  Outputs:
  0: envelope: mod cv range [-10v, 10v]

  Controls:
  0: attack
  1: decay
  2: sustain
  3: release
*/

#define ADSR_INCOUNT            6
#define ADSR_OUTCOUNT           1
#define ADSR_CONTROLCOUNT       4

#define ADSR_IN_PORT_GATE       0
#define ADSR_IN_PORT_TRIG       1
#define ADSR_IN_PORT_A          2
#define ADSR_IN_PORT_D          3
#define ADSR_IN_PORT_S          4
#define ADSR_IN_PORT_R          5

#define ADSR_OUT_PORT_ENV       0

#define ADSR_CONTROL_A          0
#define ADSR_CONTROL_D          1
#define ADSR_CONTROL_S          2
#define ADSR_CONTROL_R          3

///////////
// TYPES //
///////////

typedef enum ADSR_section
{
  ADSR_ASection,
  ADSR_DSection,
  ADSR_SSection,
  ADSR_RSection,

} ADSR_section;

typedef struct ADSR
{
  Module module;

  // input ports
  R4 * inputPorts[ADSR_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * ADSR_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * ADSR_OUTCOUNT];

  R4 controlsCurr[ADSR_CONTROLCOUNT];
  R4 controlsPrev[ADSR_CONTROLCOUNT];

  ADSR_section section;
  R4 timeSinceSectionStart;
  R4 prevSampleValue;
  R4 releaseStartVal;
  R4 prevADSRStop;
  R4 prevADSRVal;

  bool isHeld;
  bool envelopeActive;

} ADSR;

Module * ADSR_init(char* name);

#endif
