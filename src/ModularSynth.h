#ifndef MODULAR_SYNTH_H_
#define MODULAR_SYNTH_H_

#include "Module.h"
#include "ModuleFactory.h"

//////////////
// DEFINES  //
//////////////

#define MAX_RACK_SIZE 1000

/////////////
//  TYPES  //
/////////////

typedef U2 ModularID;

typedef struct ModularSynth
{
  Module * modules[MAX_RACK_SIZE];
  U2 moduleIDtoIdx[MAX_RACK_SIZE];
  bool moduleIDAvailability[MAX_RACK_SIZE]; // 1=available, 0=not
  U2 moduleCount;

  R4 outputBufferLeft[STREAM_BUFFER_SIZE];
  R4 outputBufferRight[STREAM_BUFFER_SIZE];

  R4 * outModuleLeft;
  R4 * outModuleRight;

} ModularSynth;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

ModularSynth * ModularSynth_init(void);
void ModularSynth_free(ModularSynth * synth);

R4 * ModularSynth_getLeftChannel(ModularSynth * synth);
R4 * ModularSynth_getRightChannel(ModularSynth * synth);
void ModularSynth_update(ModularSynth * synth);

ModularID ModularSynth_addModule(ModularSynth * synth, ModuleType type);
bool ModularSynth_removeModule(ModularSynth * synth, ModularID id);
bool ModularSynth_addConnection(ModularSynth * synth, ModularID srcId, U4 srcPort, ModularID destId, U4 destPort);
bool ModularSynth_removeConnection(ModularSynth * synth);
bool ModularSynth_setControl(ModularSynth * synth, ModularID id, U4 controlID, R4 val);

#endif
