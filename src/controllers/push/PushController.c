#include "PushController.h"

#include "../../push/OutputMessageBuilder.h"
#include "../../push/PushEventManager.h"
#include "../../push/PushManager.h"
#include "../../push/PushUsbDriver.h"


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
static PushControllerState* push;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void PushController_run(void)
{
    if (initDone) exit(0);
    initDone = true;

    push = calloc(1, sizeof(PushControllerState));

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

// priv
static void onSynthMessage(MessageType t, void* d, MessageSize s)
{
    switch (t)
    {
        case ControllerMessageType_SetControllerID:
        {
            push->id = ((ControllerMessage_SetControllerID*)d)->id;

            ControllerMessage_ReqModuleData req = {
                .header = {
                    .id = push->id,
                },
                .type = ControllerDataRequestType_GetSummary,
            };
            IPC_PostMessage(ControllerMessageType_ReqModuleData, &req, sizeof(req));
            break;
        }
        case ControllerMessageType_RespGetSummary:
        {
            ControllerMessage_RespGetSummary* res = (ControllerMessage_RespGetSummary*)d;
            
            for (int i = 0; i < res->length; i++)
            {
                printf("%s\n", res->names[i]);
            }
            // TODO fill in some push state here
        }
        default:
        {
            break;
        }
    }
}

void onPad(void * sub, void * args)
{
    AbletonPkt_pad* pkt = args;
}

void onBtn(void * sub, void * args)
{
    AbletonPkt_button* pkt = args;
}

void onKnob(void * sub, void * args)
{
    AbletonPkt_knob* pkt = args;
}

void onSlider(void * sub, void * args)
{
    AbletonPkt_slider* pkt = args;
}