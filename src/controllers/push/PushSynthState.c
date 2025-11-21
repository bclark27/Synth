#include "PushSynthState.h"

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static ControllerCommon_ModuleConfig* getFreeConfigSpot();

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool initDone = false;
static PushSynthState instance;
static PushSynthState* state = &instance;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

PushSynthState* PushSynthState_getState()
{
    if (!initDone)
    {
        memset(state, 0, sizeof(PushSynthState));
        initDone = true;
    }
    return state;
}

void PushSynthState_updateSynthModuleConfig(ControllerCommon_ModuleConfig* config)
{
    // try to find if this is a repeat of the same module id somewhere
    ControllerCommon_ModuleConfig* spot = PushSynthState_getModuleConfigById(config->id);
    if (spot == NULL)
    {
        spot = getFreeConfigSpot(state);
    }

    if (spot == NULL)
    {
        return;
    }

    *spot = *config;
}

ControllerCommon_ModuleConfig* PushSynthState_getModuleConfigById(ModularID id)
{
    for (int i = 0; i < MAX_RACK_SIZE; i++)
    {
        if (state->synthModuleConfigInUse[i] && state->synthModuleConfigs[i].id == id) return &state->synthModuleConfigs[i];
    }

    return NULL;
}

//////////////////
//  PRIV FUNCS  //
//////////////////

static ControllerCommon_ModuleConfig* getFreeConfigSpot()
{
    for (int i = 0; i < MAX_RACK_SIZE; i++)
    {
        if (!state->synthModuleConfigInUse[i]) return &state->synthModuleConfigs[i];
    }

    return NULL;
}