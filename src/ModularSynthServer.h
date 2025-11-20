#ifndef MODULAR_SYNTH_SERVER_H_
#define MODULAR_SYNTH_SERVER_H_

#include "controllers/ControllerCommon.h"
#include "comm/Common.h"
#include "comm/IPC.h"
#include "ModularSynth.h"

///////////
// TYPES //
///////////

typedef struct ModularSynthServer
{

} ModularSynthServer;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void ModularSynthServer_init();
void ModularSynthServer_connectToController(char* name);

#endif