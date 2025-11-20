#ifndef PUSH_CONTROLLER_H_
#define PUSH_CONTROLLER_H_

#include "ControllerCommon.h"

///////////////
//  DEFINES  //
///////////////

///////////
// TYPES //
///////////

typedef struct PushControllerState
{
    int id;
} PushControllerState;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void PushController_run(void);

#endif