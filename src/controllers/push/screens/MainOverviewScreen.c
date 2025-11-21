#include "MainOverviewScreen.h"

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void freeScreen(void* screen);
static void configChanged(void* screen, ModularID id);
static void mouted(void* screen);
static void unmounted(void* screen);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static PushScreen vtable = {
    .type = PushScreenType_MainOverview,
    .freeScreen = freeScreen,
    .configChanged = configChanged,
    .mouted = mouted,
    .unmounted = unmounted,
  };
  
////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

PushScreen* MainOverviewScreen_init()
{
    MainOverviewScreen* mo = (MainOverviewScreen*)calloc(1, sizeof(MainOverviewScreen));

    mo->screen = vtable;

    return (PushScreen*)mo;
}

//////////////////
//  PRIV FUNCS  //
//////////////////


static void freeScreen(void* screen)
{
    MainOverviewScreen* mo = (MainOverviewScreen*)screen;
}

static void configChanged(void* screen, ModularID id)
{
    MainOverviewScreen* mo = (MainOverviewScreen*)screen;
    printf("udpated\n");
}

static void mouted(void* screen)
{
    MainOverviewScreen* mo = (MainOverviewScreen*)screen;
    printf("ASDASDASDASDASD\n");
}

static void unmounted(void* screen)
{
    MainOverviewScreen* mo = (MainOverviewScreen*)screen;
}