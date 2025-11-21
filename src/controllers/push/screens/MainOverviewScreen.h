#ifndef MAIN_OVERVIEW_SCREEN_H_
#define MAIN_OVERVIEW_SCREEN_H_

#include "PushScreen.h"

///////////
// TYPES //
///////////

typedef struct MainOverviewScreen
{
    PushScreen screen;
} MainOverviewScreen;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

PushScreen* MainOverviewScreen_init();

#endif