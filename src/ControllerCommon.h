#ifndef CONTROLLER_COMMON_H_
#define CONTROLLER_COMMON_H_

#include "Module.h"

///////////////
//  DEFINES  //
///////////////

#define PUSH_CONTROLLER_NAME "Controller"
#define SYNTH_NAME "Synth"

///////////
// TYPES //
///////////

/*
    all messages will go here
*/

typedef enum ControllerDataRequestType
{
    ControllerDataRequestType_GetSummary,
} ControllerDataRequestType;

typedef enum ControllerMessageType
{
    ControllerMessageType_SetControllerID = 0x80,
    ControllerMessageType_ReqModuleData,
    ControllerMessageType_RespGetSummary,
} ControllerMessageType;

typedef struct __ControllerMessageHeader
{
    int id;
} __ControllerMessageHeader;

typedef struct ControllerMessage_SetControllerID
{
    __ControllerMessageHeader header;
    int id;
} ControllerMessage_SetControllerID;

typedef struct ControllerMessage_ReqModuleData
{
    __ControllerMessageHeader header;
    ControllerDataRequestType type;
    ModularID modId;
    ModularPortID portId;

} ControllerMessage_ReqModuleData;

typedef struct ControllerMessage_RespGetSummary
{
    __ControllerMessageHeader header;
    int length;
    ModularID ids[MAX_RACK_SIZE];
    ModuleType types[MAX_RACK_SIZE];
    char names[MAX_RACK_SIZE][MAX_MODULE_NAME_LEN];

} ControllerMessage_RespGetSummary;

#endif