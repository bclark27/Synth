#include "PushController.h"

#include "../../push/OutputMessageBuilder.h"
#include "../../push/PushEventManager.h"
#include "../../push/PushManager.h"
#include "../../push/PushUsbDriver.h"
#include "screens/PushScreenManager.h"


/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

void onPad(void * sub, void * args);
void onBtn(void * sub, void * args);
void onKnob(void * sub, void * args);
void onSlider(void * sub, void * args);

static void onSynthMessage(MessageType t, void* d, MessageSize s);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool initDone = false;
static PushControllerState instance;
static PushControllerState* push = &instance;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void PushController_run(void)
{
    if (initDone) exit(0);
    initDone = true;

    memset(push, 0, sizeof(PushControllerState));
    push->id = -1;

    push->state = PushSynthState_getState();

    PushManager_Init();
    int s = PushManager_InitServer(PUSH_CONTROLLER_NAME);
    printf("PushController server status: %d\n", s);

    printf("Connecting to: %s\n", SYNTH_NAME);
    
    s = -1;
    while (s != CC_SUCCESS)
    {
        s = IPC_ConnectToService(SYNTH_NAME, onSynthMessage);
        //s = PushManager_ReceiveCommandsFromService(SYNTH_NAME);
        printf("connection status: %d\n", s);

        if (s != CC_SUCCESS)
        sleep(1);
    }

    PushScreenManager_init();
    PushScreenManager_navigate(PushScreenType_MainOverview);

    printf("cycling push\n");

    pushEventManager_subscribeToNewPadPackets(push, onPad);
    pushEventManager_subscribeToNewButtonPackets(push, onBtn);
    pushEventManager_subscribeToNewKnobPackets(push, onKnob);
    pushEventManager_subscribeToNewSliderPackets(push, onSlider);

    // char btnHandler;
    // pushEventManager_subscribeToNewButtonPackets(&btnHandler, CloseBtnListener);

    while (1)
    {
        PushManager_Cycle();
    }

    pushEventManager_unsubscribeToNewPadPackets(push);
    pushEventManager_unsubscribeToNewButtonPackets(push);
    pushEventManager_unsubscribeToNewKnobPackets(push);
    pushEventManager_unsubscribeToNewSliderPackets(push);

    // pushEventManager_unsubscribeToNewButtonPackets(&btnHandler);
    // PushManager_Free();

}

void onPad(void * sub, void * args)
{
    PushScreenManager_notify_pushEvent(args, MSG_TYPE_ABL_PAD);
}

void onBtn(void * sub, void * args)
{
    PushScreenManager_notify_pushEvent(args, MSG_TYPE_ABL_BUTTON);
}

void onKnob(void * sub, void * args)
{
    PushScreenManager_notify_pushEvent(args, MSG_TYPE_ABL_KNOB);
}

void onSlider(void * sub, void * args)
{
    PushScreenManager_notify_pushEvent(args, MSG_TYPE_ABL_SLIDER);
}

// priv
static void onSynthMessage(MessageType t, void* d, MessageSize s)
{
    if (!MESSAGE_IS_CONTROLLER_TYPE(t) || !MESSAGE_SENT_TO_ME(d, push->id)) return;

    switch (t)
    {
        case ControllerMessageType_SetControllerID:
        {
            if (push->id >= 0) break;

            push->id = ((ControllerMessage_SetControllerID*)d)->id;

            ControllerMessage_ReqGetSummary req = {
                .header = {
                    .controllerId = push->id,
                },
                .fullSummaryReq = true,
            };
            IPC_PostMessage(ControllerMessageType_ReqGetSummary, &req, sizeof(req));
            break;
        }
        case ControllerMessageType_RespGetFullSummary:
        {
            ControllerMessage_RespGetFullSummary* res = (ControllerMessage_RespGetFullSummary*)d;
            ControllerCommon_ModuleConfig* configs = (ControllerCommon_ModuleConfig*)(((char*)res) + sizeof(ControllerMessage_RespGetFullSummary));
            for (int i = 0; i < res->length; i++)
            {
                ControllerCommon_ModuleConfig* config = configs + i;
                PushSynthState_updateSynthModuleConfig(config);
                PushScreenManager_notify_configChanged(config->id);
                
            }
            break;
        }
        case ControllerMessageType_RespGetModuleSummary:
        {
            ControllerMessage_RespGetModuleSummary* res = (ControllerMessage_RespGetModuleSummary*)d;
            ControllerCommon_ModuleConfig* config = &res->module;
            PushSynthState_updateSynthModuleConfig(config);
            PushScreenManager_notify_configChanged(config->id);
            break;
        }
        default:
        {
            break;
        }
    }
}
