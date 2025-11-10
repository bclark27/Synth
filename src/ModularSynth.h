#ifndef MODULAR_SYNTH_H_
#define MODULAR_SYNTH_H_

#include "Module.h"
#include "ModuleFactory.h"
#include "ConfigParser.h"
#include <pthread.h>
#include <stdatomic.h>

//////////////
// DEFINES  //
//////////////

#undef MULTITHREAD_UPDATE_LOOP
#define MAX_SYNTH_THREADS 1

/////////////
//  TYPES  //
/////////////

typedef struct {
  pthread_t threads[MAX_SYNTH_THREADS];
  pthread_barrier_t barrier;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int running;
  int phase;
  U4 moduleCount;
} SynthThreadPool;

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

  SynthThreadPool threadpool;

} ModularSynth;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

void ModularSynth_init(void);
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
bool ModularSynth_setControl(ModularID id, ModularPortID controlID, void* val);
bool ModularSynth_setControlByName(char * name, char * controlName, void* val);
void ModularSynth_getControlByName(char * name, char * controlName, void* ret);
ModulePortType ModularSynth_getControlTypeByName(char * name, char * controlName);
bool ModularSynth_readConfig(char * fname);
bool ModularSynth_exportConfig(char * fname);
char* ModularSynth_PrintFullModuleInfo(ModularID id);

#endif
