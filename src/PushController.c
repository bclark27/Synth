#include "PushController.h"

#include "push/OutputMessageBuilder.h"
#include "push/PushEventManager.h"
#include "push/PushManager.h"
#include "push/PushUsbDriver.h"


/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void onSynthMessage(MessageType t, void* d, MessageSize s);
// static void CloseBtnListener(void * sub, void * args);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

// static char stop = 0;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void PushController_run(void)
{
    PushManager_Init();
    int s = PushManager_InitServer(PUSH_CONTROLLER_NAME);
    printf("PushController server status: %d\n", s);

    printf("Connecting to: %s\n", SYNTH_NAME);
    
    s = -1;
    while (s != CC_SUCCESS)
    {
        //s = IPC_ConnectToService(SYNTH_NAME, onSynthMessage);
        s = PushManager_ReceiveCommandsFromService(SYNTH_NAME);
        printf("connection status: %d\n", s);

        if (s != CC_SUCCESS)
        sleep(1);
    }

    printf("cycling push\n");

    // char btnHandler;
    // pushEventManager_subscribeToNewButtonPackets(&btnHandler, CloseBtnListener);

    while (1)
    {
        PushManager_Cycle();
    }

    // pushEventManager_unsubscribeToNewButtonPackets(&btnHandler);
    // PushManager_Free();
}

// priv
static void onSynthMessage(MessageType t, void* d, MessageSize s)
{
    // if (t == TESTING_PKT)
    //     printf("Push Controller %d\n", ((TestingPacket*)d)->test);
}

// static void CloseBtnListener(void * sub, void * args)
// {
//   AbletonPkt_button * pkt = args;
//   if (pkt->btnId == 3)
//   {
//     stop = 1;
//   }

//   SetBtnOn(pkt->btnId);
// }