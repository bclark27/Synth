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
void ModularSynth_free();

R4 * ModularSynth_getLeftChannel();
R4 * ModularSynth_getRightChannel();
void ModularSynth_update();

ModularID ModularSynth_addModule(ModuleType type, char * name);
ModularID ModularSynth_addModuleByName(char* type, char * name);
bool ModularSynth_removeModule(ModularID id);
bool ModularSynth_removeModuleByName(char* name);
bool ModularSynth_addConnection(ModularID srcId, ModularPortID srcPort, ModularID destId, ModularPortID destPort);
bool ModularSynth_addConnectionByName(char* srcModuleName, char* srcPortName, char* destModuleName, char* destPortName);
void ModularSynth_removeConnection(ModularID destId, ModularPortID destPort);
void ModularSynth_removeConnectionByName(char* destModuleName, char* destPortName);
bool ModularSynth_setControl(ModularID id, ModularPortID controlID, R4 val);
bool ModularSynth_setControlByName(char * name, char * controlName, R4 val);
bool ModularSynth_readConfig(char * fname);
bool ModularSynth_exportConfig(char * fname);
char* ModularSynth_PrintFullModuleInfo(ModularID id);

#endif
