#ifndef PUSH_SCREEN_H_
#define PUSH_SCREEN_H_

#include "../../ControllerCommon.h"
#include "../../../comm/IPC.h"

///////////
// TYPES //
///////////

typedef enum PushScreenType
{
    PushScreenType_MainOverview,
    PushScreenType_Count,
} PushScreenType;

typedef struct PushScreen
{
    PushScreenType type;
    void (*freeScreen)(void*);
    void (*configChanged)(void*, ModularID);
    void (*mouted)(void*);
    void (*unmounted)(void*);
    void (*onPushEvent)(void*,void*,MessageType);
} PushScreen;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////


#endif