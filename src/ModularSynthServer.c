#include "ModularSynthServer.h"


/////////////
//  TYPES  //
/////////////

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

void OnSynthRequest(MessageType t, void* d, MessageSize s);
void handleSummaryReq(ControllerMessage_ReqGetSummary* req);
void fillInModuleSummary(ControllerCommon_ModuleConfig* config, ModularID id);

////////////////////
//  PRIVATE VARS  //
////////////////////

static int controllerCounter = 0;
static bool initDone = false;
static ModularSynthServer instance;
static ModularSynthServer* server = &instance;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void ModularSynthServer_init()
{
    if (initDone) return;

    server = (ModularSynthServer*)calloc(1, sizeof(ModularSynthServer));
    IPC_StartService(SYNTH_NAME);
}

void ModularSynthServer_connectToController(char* name)
{
    IPC_ConnectToService(name, OnSynthRequest);
    
    int newId = controllerCounter++;
    ControllerMessage_SetControllerID msg = {
        .header = {
            .controllerId = -1,
        },
        .id = newId,
    };

    sleep(1);

    IPC_PostMessage(ControllerMessageType_SetControllerID, &msg, sizeof(msg));
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

void OnSynthRequest(MessageType t, void* d, MessageSize s)
{
    switch (t)
    {
        case ControllerMessageType_ReqGetSummary:
        {
            handleSummaryReq((ControllerMessage_ReqGetSummary*)d);
        }
        default:
        {
            break;
        }
    }
}

void handleSummaryReq(ControllerMessage_ReqGetSummary* req)
{
    switch (req->fullSummaryReq)
    {
        case true:
        {
            ModularSynth_readLock(true);
            
            int len;
            ModularID ids[MAX_RACK_SIZE];
            ModularSynth_GetAllModulrIDs(ids, &len);
            
            int totalSize = sizeof(ControllerMessage_RespGetFullSummary) + sizeof(ControllerCommon_ModuleConfig) * len;
            char data[totalSize];
            
            ControllerMessage_RespGetFullSummary* sum = (ControllerMessage_RespGetFullSummary*)data;
            ControllerCommon_ModuleConfig* configs = (ControllerCommon_ModuleConfig*)(((char*)data) + sizeof(ControllerMessage_RespGetFullSummary));
            sum->length = len;
            sum->header.controllerId = req->header.controllerId;

            for (int i = 0; i < len; i++)
            {
                fillInModuleSummary(&configs[i], ids[i]);
            }

            ModularSynth_readLock(false);

            IPC_PostMessage(ControllerMessageType_RespGetFullSummary, data, totalSize);
            break;
        }
        default:
        {
            ModularSynth_readLock(true);
            
            ModularID id = req->modId;
            ControllerMessage_RespGetModuleSummary sum;
            fillInModuleSummary(&sum.module, id);
            sum.header.controllerId = req->header.controllerId;
            ModularSynth_readLock(false);

            IPC_PostMessage(ControllerMessageType_RespGetModuleSummary, &sum, sizeof(sum));
            break;
        }
    }
}

void fillInModuleSummary(ControllerCommon_ModuleConfig* config, ModularID id)
{
    config->id = id;
    config->type = ModularSynth_GetModuleType(id);
    ModularSynth_CopyModuleName(id, config->name);
}