#ifndef PUSH_SCREEN_H_
#define PUSH_SCREEN_H_

#include "../../ControllerCommon.h"
#include "../../../comm/IPC.h"
#include "../PushSynthState.h"
#include "../../../push/OutputMessageBuilder.h"

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
    void (*configChanged)(void*,void*,PushSynthStateChangeType);
    void (*mouted)(void*);
    void (*unmounted)(void*);
    void (*onPushEvent)(void*,void*,MessageType);
    bool screenIsVisible;
    pushStateObject pushState;
} PushScreen;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////


#endif