#include "MainOverviewScreen.h"

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void freeScreen(void* screen);
static void configChanged(void* screen, void* event, PushSynthStateChangeType type);
static void mouted(void* screen);
static void unmounted(void* screen);
static void onPushEvent(void* screen, void* event, MessageType type);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static PushScreen vtable = {
    .type = PushScreenType_MainOverview,
    .freeScreen = freeScreen,
    .configChanged = configChanged,
    .mouted = mouted,
    .unmounted = unmounted,
    .onPushEvent = onPushEvent,
  };
  
////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

PushScreen* MainOverviewScreen_init()
{
    MainOverviewScreen* mo = (MainOverviewScreen*)calloc(1, sizeof(MainOverviewScreen));

    mo->screen = vtable;

    PushStates_initStateObj(&mo->screen.pushState);

    PushStates_setText(&mo->screen.pushState, 1, 1, "hello", 5);

    return (PushScreen*)mo;
}

//////////////////
//  PRIV FUNCS  //
//////////////////


static void freeScreen(void* screen)
{
    MainOverviewScreen* mo = (MainOverviewScreen*)screen;
}

static void configChanged(void* screen, void* event, PushSynthStateChangeType type)
{
    MainOverviewScreen* mo = (MainOverviewScreen*)screen;
}

static void mouted(void* screen)
{
    MainOverviewScreen* mo = (MainOverviewScreen*)screen;
    outputMessageBuilder_matchStateObj(&mo->screen.pushState);
    outputMessageBuilder_updatePush();
}

static void unmounted(void* screen)
{
    MainOverviewScreen* mo = (MainOverviewScreen*)screen;
}

static void onPushEvent(void* screen, void* event, MessageType type)
{
    switch (type)
    {
        case MSG_TYPE_ABL_PAD:
        {
            AbletonPkt_pad* pad = event;
            printf("PAD\n");
            break;
        }
        case MSG_TYPE_ABL_BUTTON:
        {
            AbletonPkt_button* btn = event;
            printf("BTN\n");
            break;
        }
        case MSG_TYPE_ABL_KNOB:
        {
            AbletonPkt_knob* knob = event;
            printf("KNOB\n");
            break;
        }
        case MSG_TYPE_ABL_SLIDER:
        {
            AbletonPkt_slider* slider = event;
            printf("SLIDER\n");
            break;
        }
        default:
        {
            break;
        }
    }
}