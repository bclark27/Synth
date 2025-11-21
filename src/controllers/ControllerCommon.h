#ifndef CONTROLLER_COMMON_H_
#define CONTROLLER_COMMON_H_

#include "../modules/Module.h"

///////////////
//  DEFINES  //
///////////////

#define PUSH_CONTROLLER_NAME "Controller"
#define SYNTH_NAME "Synth"

#define MESSAGE_IS_CONTROLLER_TYPE(type) ((type) < ControllerMessageType_Count && (type) >= ControllerMessageType_SetControllerID)
#define MESSAGE_SENT_TO_ME(msg, myId) (((__ControllerMessageHeader*)(msg))->controllerId == (myId) || ((__ControllerMessageHeader*)(msg))->controllerId <= -1)

///////////
// TYPES //
///////////



typedef struct {
    ModularID module;
    ModularPortID port;
} ControllerCommon_ConnectionInfo;

typedef struct {
    ModulePortType type;
    char name[MAX_MODULE_NAME_LEN];
} ControllerCommon_ControlInfo;

typedef struct {
    ModulePortType type;
    char name[MAX_MODULE_NAME_LEN];
    bool hasConnection;
    ControllerCommon_ConnectionInfo connection;
} ControllerCommon_InPortInfo;

typedef struct {
    ModulePortType type;
    char name[MAX_MODULE_NAME_LEN];
} ControllerCommon_OutPortInfo;

typedef struct {
    ModuleType type;
    ModularID id;
    char name[MAX_MODULE_NAME_LEN];

    int outPortCount;
    int inPortCount;
    int controlCount;

    ControllerCommon_OutPortInfo outPorts[MAX_PORT_COUNT];
    ControllerCommon_InPortInfo inPorts[MAX_PORT_COUNT];
    ControllerCommon_ControlInfo controls[MAX_PORT_COUNT];
} ControllerCommon_ModuleConfig;


/*
    all messages will go here
*/

typedef enum ControllerMessageType
{
    ControllerMessageType_SetControllerID = 0x80,
    ControllerMessageType_ReqGetSummary,
    ControllerMessageType_RespGetFullSummary,
    ControllerMessageType_RespGetModuleSummary,
    ControllerMessageType_ReqSetControlValue,
    ControllerMessageType_RespControlValue,

    ControllerMessageType_Count,
} ControllerMessageType;

typedef struct __ControllerMessageHeader
{
    int controllerId;
} __ControllerMessageHeader;

typedef struct ControllerMessage_SetControllerID
{
    __ControllerMessageHeader header;

    int id;
} ControllerMessage_SetControllerID;

typedef struct ControllerMessage_ReqGetSummary
{
    __ControllerMessageHeader header;

    ModularID modId;
    bool fullSummaryReq;
} ControllerMessage_ReqGetSummary;

typedef struct ControllerMessage_RespGetFullSummary
{
    __ControllerMessageHeader header;

    int length;
    /*
        payload of N ControllerCommon_ModuleConfig structs is right after this guy
    */
} ControllerMessage_RespGetFullSummary;

typedef struct ControllerMessage_RespGetModuleSummary
{
    __ControllerMessageHeader header;

    ControllerCommon_ModuleConfig module;
} ControllerMessage_RespGetModuleSummary;

typedef struct ControllerMessage_RespControlValue
{
    __ControllerMessageHeader header;

    ModularID module;
    ModularPortID controlId;
    ModulePortType type;
    union
    {
        R4 controlVolt;
    } data;
} ControllerMessage_RespControlValue;

typedef struct ControllerMessage_ReqSetControlValue
{
    __ControllerMessageHeader header;

    ModularID module;
    ModularPortID controlId;
    ModulePortType type;
    union
    {
        R4 controlVolt;
        MIDIData midi;
    } data;
} ControllerMessage_ReqSetControlValue;

#endif