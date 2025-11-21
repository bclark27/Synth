#ifndef PUSH_CONTROLLER_H_
#define PUSH_CONTROLLER_H_

#include "../ControllerCommon.h"
#include "PushSynthState.h"

///////////////
//  DEFINES  //
///////////////

///////////
// TYPES //
///////////

typedef struct PushControllerState
{
    int id;
    PushSynthState* state;
} PushControllerState;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void PushController_run(void);

#endif