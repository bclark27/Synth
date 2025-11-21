#ifndef PUSH_SYNTH_STATE_H_
#define PUSH_SYNTH_STATE_H_

#include "../ControllerCommon.h"

///////////////
//  DEFINES  //
///////////////

///////////
// TYPES //
///////////

typedef struct PushSynthState
{
    int id;
    ControllerCommon_ModuleConfig synthModuleConfigs[MAX_RACK_SIZE];
    bool synthModuleConfigInUse[MAX_RACK_SIZE];
} PushSynthState;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

PushSynthState* PushSynthState_getState();
void PushSynthState_updateSynthModuleConfig(ControllerCommon_ModuleConfig* config);
ControllerCommon_ModuleConfig* PushSynthState_getModuleConfigById(ModularID id);

#endif