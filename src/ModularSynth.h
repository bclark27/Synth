#ifndef MODULAR_SYNTH_H_
#define MODULAR_SYNTH_H_

#include "Module.h"
#include "ModuleFactory.h"
#include "ConfigParser.h"

//////////////
// DEFINES  //
//////////////

/////////////
//  TYPES  //
/////////////

typedef U2 ModularID;

typedef struct ModuleConnection
{
  ModularPortID srcPort;
  ModularPortID destPort;
  ModularID srcMod;
  ModularID destMod;
} ModuleConnection;

typedef struct ModularSynth
{
  U2 modulesCount;
  Module * modules[MAX_RACK_SIZE];
  U2 moduleIDtoIdx[MAX_RACK_SIZE];
  bool moduleIDAvailability[MAX_RACK_SIZE]; // 1=available, 0=not

  U2 portConnectionsCount;
  ModuleConnection portConnections[MAX_CONN_COUNT];

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

ModularID ModularSynth_addModule(ModularSynth * synth, ModuleType type, char * name);
ModularID ModularSynth_addModuleByName(ModularSynth * synth, char* type, char * name);
bool ModularSynth_removeModule(ModularSynth * synth, ModularID id);
bool ModularSynth_removeModuleByName(ModularSynth * synth, char* name);
bool ModularSynth_addConnection(ModularSynth * synth, ModularID srcId, ModularPortID srcPort, ModularID destId, ModularPortID destPort);
bool ModularSynth_addConnectionByName(ModularSynth * synth, char* srcModuleName, char* srcPortName, char* destModuleName, char* destPortName);
void ModularSynth_removeConnection(ModularSynth * synth, ModularID destId, ModularPortID destPort);
void ModularSynth_removeConnectionByName(ModularSynth * synth, char* destModuleName, char* destPortName);
bool ModularSynth_setControl(ModularSynth * synth, ModularID id, ModularPortID controlID, R4 val);
bool ModularSynth_setControlByName(ModularSynth * synth, char * name, char * controlName, R4 val);
bool ModularSynth_readConfig(ModularSynth * synth, char * fname);
bool ModularSynth_exportConfig(ModularSynth * synth, char * fname);
char* ModularSynth_PrintFullModuleInfo(ModularSynth * synth, ModularID id);

#endif
