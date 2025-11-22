#ifndef PUSH_SYNTH_STATE_H_
#define PUSH_SYNTH_STATE_H_

#include "../ControllerCommon.h"

///////////////
//  DEFINES  //
///////////////

///////////
// TYPES //
///////////

typedef enum PushSynthStateChangeType
{
    PushSynthStateChangeType_ModuleAdd,
    PushSynthStateChangeType_ModuleRemove,
    PushSynthStateChangeType_ConnectionAdd,
    PushSynthStateChangeType_ConnectionRemove,
    PushSynthStateChangeType_ControlValueChanged,
    PushSynthStateChangeType_InPortValueChanged,
    PushSynthStateChangeType_OutPortValueChanged,

    PushSynthStateChangeType_Count,
} PushSynthStateChangeType;

typedef struct PushSynthStateChange
{
    PushSynthStateChangeType type;

    union {
        struct {
            ModularID mod;
        } ModuleAdd;

        struct {
            ModularID mod;
        } ModuleRemoved;

        struct {
            ModularID srcMod;
            ModularPortID srcPort;
            ModularID destMod;
            ModularPortID destPort;
        } ConnectionAdded;

        struct {
            ModularID srcMod;
            ModularPortID srcPort;
            ModularID destMod;
            ModularPortID destPort;
        } ConnectionRemoved;

        struct {
            ModularID mod;
            ModularPortID control;
        } ControlValueChanged;

        struct {
            ModularID mod;
            ModularPortID control;
        } InPortValueChanged;

        struct {
            ModularID mod;
            ModularPortID control;
        } OutPortValueChanged;
    };
} PushSynthStateChange;

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