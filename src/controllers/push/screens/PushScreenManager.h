#ifndef PUSH_SCREEN_MANAGER_H_
#define PUSH_SCREEN_MANAGER_H_

#include "PushScreen.h"
#include "../PushSynthState.h"

#include "MainOverviewScreen.h"

///////////
// TYPES //
///////////

typedef struct PushScreenManager
{
    PushScreen* currentScreen;
    PushScreen* screens[PushScreenType_Count];
    PushSynthState* state;
} PushScreenManager;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void PushScreenManager_init();
void PushScreenManager_navigate(PushScreenType dest);
void PushScreenManager_notify_configChanged(PushSynthStateChange event);
void PushScreenManager_notify_pushEvent(void* event, MessageType type);

#endif