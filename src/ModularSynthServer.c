#include "ModularSynthServer.h"


/////////////
//  TYPES  //
/////////////

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

void OnSynthRequest(MessageType t, void* d, MessageSize s);
void handleDataRequest(ControllerMessage_ReqModuleData* req);

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
            .id = newId,
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
        case ControllerMessageType_ReqModuleData:
        {
            handleDataRequest((ControllerMessage_ReqModuleData*)d);
        }
        default:
        {
            break;
        }
    }
}

void handleDataRequest(ControllerMessage_ReqModuleData* req)
{
    switch (req->type)
    {
        case ControllerDataRequestType_GetSummary:
        {
            ControllerMessage_RespGetSummary sum;

            ModularSynth_readLock(true);

            int len;
            ModularSynth_GetAllModulrIDs(sum.ids, &len);
            sum.length = len;

            for (int i = 0; i < len; i++)
            {
                sum.types[i] = ModularSynth_GetModuleType(sum.ids[i]);
                ModularSynth_CopyModuleName(sum.ids[i], sum.names[i]);
            }

            ModularSynth_readLock(false);

            IPC_PostMessage(ControllerMessageType_RespGetSummary, &sum, sizeof(sum));
            break;
        }
        default:
        {
            break;
        }
    }
}