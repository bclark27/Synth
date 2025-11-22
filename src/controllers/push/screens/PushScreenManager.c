#include "PushScreenManager.h"

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool initDone = false;
static PushScreenManager instance;
static PushScreenManager* manager = &instance;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void PushScreenManager_init()
{
    if (initDone) return;
    initDone = true;

    memset(manager, 0, sizeof(PushScreenManager));
    manager->state = PushSynthState_getState();

    manager->screens[PushScreenType_MainOverview] = MainOverviewScreen_init();
}

void PushScreenManager_navigate(PushScreenType dest)
{
    if (dest < 0 ||
        dest >= PushScreenType_Count)
        return;

    if (manager->currentScreen != NULL &&
        dest == manager->currentScreen->type)
        return;

    if (manager->currentScreen != NULL)
    {
        manager->currentScreen->unmounted(manager->currentScreen);
        manager->currentScreen->screenIsVisible = false;
    }

    manager->currentScreen = manager->screens[dest];
    manager->currentScreen->screenIsVisible = true;
    manager->currentScreen->mouted(manager->currentScreen);
}

void PushScreenManager_notify_configChanged(void* event, PushSynthStateChangeType type)
{
    if (manager->currentScreen)
    {
        manager->currentScreen->configChanged(manager->currentScreen, event, type);
    }
}

void PushScreenManager_notify_pushEvent(void* event, MessageType type)
{
    if (manager->currentScreen)
    {
        manager->currentScreen->onPushEvent(manager->currentScreen, event, type);
    }
}