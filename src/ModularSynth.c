#include "ModularSynth.h"

#include "AudioSettings.h"
#include "ModuleFactory.h"
#include <pthread.h>
#include <stdatomic.h>
#include <xmmintrin.h>
#include <immintrin.h>

//////////////
// DEFINES  //
//////////////

#define UPDATE_MOD_PHASE  0
#define PUSH_BUFFERS_PHASE  1
#define SHUTDOWN_PHASE  2

/////////////
//  TYPES  //
/////////////


/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static Module * getModuleByName(char * name);
static ModularID getModuleIdByIdx(int idx);
static ModularID getModuleIdByName(char * name, bool* found);
static Module * getModuleById(ModularID id);
static bool connectionLogEq(ModuleConnection a, ModuleConnection b);
static bool connectionExists(ModuleConnection connection);
static void logConnection(ModuleConnection connection);
static void removeConnectionLog(ModuleConnection connection);
static void getAllInPortConnections(ModularID id, ModuleConnection * connections, U4 * connectionsCount);
static void getAllOutPortConnections(ModularID id, ModularPortID port, ModuleConnection * connections, U4 * connectionsCount);
static ModuleConnection getConnectionToDestInPort(ModularID id, ModularPortID port, bool* found);
static bool addConnectionHelper(Module * srcMod, ModularID srcId, ModularPortID srcPort, Module * destMod, ModularID destId, ModularPortID destPort);
void* updateWorkerThread(void *arg);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static ModularSynth synthInst;
static ModularSynth* synth = &synthInst;

static FullConfig config;
static const char * outModuleName = "__OUTPUT__";

static bool initDone = false;

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////



void ModularSynth_init(void)
{
  if (initDone) return;
  initDone = true;

  _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
  _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

  memset(synth, 0, sizeof(ModularSynth));

  synth->modulesCount = 1;
  synth->moduleIDtoIdx[OUT_MODULE_ID] = OUT_MODULE_IDX;
  memset(synth->moduleIDAvailability, 1, MAX_RACK_SIZE * sizeof(U1));
  synth->moduleIDAvailability[OUT_MODULE_ID] = 0;

  Module * outMod = ModuleFactory_createModule(ModuleType_OutputModule, strdup(outModuleName));
  synth->modules[OUT_MODULE_IDX] = outMod;

  // get output modules out buffers
  synth->outModuleLeft = outMod->getOutputAddr(outMod, OUTPUT_OUT_PORT_LEFT);
  synth->outModuleRight = outMod->getOutputAddr(outMod, OUTPUT_OUT_PORT_RIGHT);

  memset(&synth->threadpool, 0, sizeof(SynthThreadPool));
  synth->threadpool.running = 1;
  pthread_barrier_init(&synth->threadpool.barrier, NULL, MAX_SYNTH_THREADS + 1);
  pthread_mutex_init(&synth->threadpool.mutex, NULL);
  for (intptr_t i = 0; i < MAX_SYNTH_THREADS; i++)
  {
    pthread_create(&synth->threadpool.threads[i], NULL, updateWorkerThread, (void*)i);
  }
}

void ModularSynth_free()
{
  atomic_store(&synth->threadpool.phase, SHUTDOWN_PHASE);
  atomic_store(&synth->threadpool.running, true);
  pthread_barrier_wait(&synth->threadpool.barrier);
  pthread_barrier_wait(&synth->threadpool.barrier);
  pthread_barrier_destroy(&synth->threadpool.barrier);

  for (U4 i = 0; i < synth->modulesCount; i++)
  {
    Module * module = synth->modules[i];
    if (module)
    {
      module->freeModule(module);
    }
  }

  initDone = false;
}

R4 * ModularSynth_getLeftChannel()
{
  return synth->outputBufferLeft;
}

R4 * ModularSynth_getRightChannel()
{
  return synth->outputBufferRight;
}

void ModularSynth_update()
{
// update all states, then push to out

  Module * module;
  U4 modulesCount = synth->modulesCount;
  R4 * currOutputPtrLeft = synth->outputBufferLeft;
  R4 * currOutputPtrRight = synth->outputBufferRight;
  

  for (U4 i = 0; i < MODULE_BUFS_PER_STREAM_BUF; i++)
  {
#ifdef MULTITHREAD_UPDATE_LOOP
    atomic_store(&synth->threadpool.moduleCount, synth->modulesCount);
    atomic_store(&synth->threadpool.phase, UPDATE_MOD_PHASE);
    pthread_barrier_wait(&synth->threadpool.barrier);
    pthread_barrier_wait(&synth->threadpool.barrier);

    atomic_store(&synth->threadpool.phase, PUSH_BUFFERS_PHASE);
    pthread_barrier_wait(&synth->threadpool.barrier);
    pthread_barrier_wait(&synth->threadpool.barrier);
#endif
#ifndef MULTITHREAD_UPDATE_LOOP
    for (int m = 0; m < modulesCount; m++)
    {
      module = synth->modules[m];
      module->updateState(module);
    }
    for (int m = 0; m < modulesCount; m++) 
    {
      module = synth->modules[m];
      module->pushCurrToPrev(module);
    }
#endif
    // output module is always idx 0 of modules[]. cpy from output
    // to the left and right output channels
    memcpy(currOutputPtrLeft, synth->outModuleLeft, MODULE_BUFFER_SIZE * sizeof(R4));
    memcpy(currOutputPtrRight, synth->outModuleRight, MODULE_BUFFER_SIZE * sizeof(R4));

    // update the ptrs to the next spot in the out buffer
    currOutputPtrLeft += MODULE_BUFFER_SIZE;
    currOutputPtrRight += MODULE_BUFFER_SIZE;
  }

}

ModularID ModularSynth_addModule(ModuleType type, char * name)
{
  // exclude all output module types
  if (type == ModuleType_OutputModule) return 0;

  // get a valid id (if non exist ret 0)
  ModularID id = 0;
  for (U4 i = 0; i < MAX_RACK_SIZE; i++)
  {
    if (synth->moduleIDAvailability[i])
    {
      id = i;
      break;
    }
  }

  if (!id) return 0;

  // create the module
  Module * module = ModuleFactory_createModule(type, name);
  if (!module) return -1;

  // add ptr to the end of the list
  U2 idx = synth->modulesCount;
  synth->modules[idx] = module;

  // make sure the len is now +1
  synth->modulesCount += 1;

  // take that idx, and sotre in the id->idx table
  synth->moduleIDtoIdx[id] = idx;

  // mark the id as unavailable
  synth->moduleIDAvailability[id] = 0;

  return id;
}


ModularID ModularSynth_addModuleByName(char* type, char * name)
{
  if (!type || !name) return -1;
  bool found;
  ModuleType typeId = Module_GetModuleTypeByName(type, &found);
  return ModularSynth_addModule(typeId, name);
}

bool ModularSynth_removeModule(ModularID id)
{
  Module * delMod = getModuleById(id);
  if (!delMod || delMod->type == ModuleType_OutputModule) return 0;
  
  int idx = 0;
  for (int i = 0; i < synth->modulesCount; i++)
  {
    if (synth->modules[i] == delMod)
    {
      idx = i;
      break;
    }
  }
  
  // disconnect all other modules from this guys outputs
  // if other module was connected, set its input to NULL or a black space

  // loop the connections in reverse so that deleting them does not affect us
  int loopCount = synth->portConnectionsCount;
  for (int i = loopCount - 1; i >= 0; i--)
  {
    // check if this connection has the mod involved
    ModuleConnection* thisConnection = &synth->portConnections[i];

    if (thisConnection->destMod == id || thisConnection->srcMod == id)
    {
      // if the destination was some other module (thedest module is pointing into the deleting module) tell that other module to connect to null
      if (thisConnection->destMod != id)
      {
        Module* otherModule = getModuleById(thisConnection->destMod);
        if (otherModule)
        {
          otherModule->linkToInput(otherModule, thisConnection->destPort, NULL);
        }
      }

      ARRAY_SHIFT(synth->portConnections, ModuleConnection, i + 1, synth->portConnectionsCount - 1, i);
      synth->portConnectionsCount--;
    }
  }

  
  // free the module
  ModuleFactory_destroyModule(delMod);
  
  // for all idx in the id->idx list that are graeter than idx, -=1
  for (U4 i = 0; i < MAX_RACK_SIZE; i++)
  {
    if (synth->moduleIDtoIdx[i] > idx)
    {
      synth->moduleIDtoIdx[i]--;
    }
  }
  
  // shift all the modules in the module list above idx to down one idx
  // decrement the total count
  for (U4 i = idx; i < synth->modulesCount - 1; i++)
  {
    synth->modules[i] = synth->modules[i + 1];
  }

  synth->modulesCount--;

  // mark the id as available
  synth->moduleIDAvailability[id] = 1;

  return 1;
}

bool ModularSynth_removeModuleByName(char* name)
{
  if (!name) return 0;
  bool found;
  ModularID id = getModuleIdByName(name, &found);
  
  if (!found) return 0;

  return ModularSynth_removeModule(id);
}

bool ModularSynth_addConnection(ModularID srcId, ModularPortID srcPort, ModularID destId, ModularPortID destPort)
{
  // exclude output module as a source
  if (srcId == OUT_MODULE_ID) return 0;

  // exclude out of bound id
  if (srcId >= MAX_RACK_SIZE || destId >= MAX_RACK_SIZE) return 0;

  // check that we have enough space to add a connection
  if (synth->portConnectionsCount >= MAX_CONN_COUNT) return 0;

  // cheeck that the modules exist
  Module * srcMod = getModuleById(srcId);
  Module * destMod = getModuleById(destId);

  if (!srcMod || !destMod) return 0;

  return addConnectionHelper(srcMod, srcId, srcPort, destMod, destId, destPort);
}

bool ModularSynth_addConnectionByName(char* srcModuleName, char* srcPortName, char* destModuleName, char* destPortName)
{
  if (!srcModuleName || !srcPortName || !destModuleName || !destPortName) return 0;

  bool srcFound;
  bool destFound;
  ModularID srcId = getModuleIdByName(srcModuleName, &srcFound);
  ModularID destId = getModuleIdByName(destModuleName, &destFound);
  if (!srcFound || !destFound) return 0;
  
  Module * srcMod = getModuleById(srcId);
  Module * destMod = getModuleById(destId);
  if (!srcMod || !destMod) return 0;
  
  ModularPortID srcPort = Module_GetOutPortId(srcMod, srcPortName, &srcFound);
  ModularPortID destPort = Module_GetInPortId(destMod, destPortName, &destFound);
  if (!srcFound || !destFound) return 0;


  return addConnectionHelper(srcMod, srcId, srcPort, destMod, destId, destPort);
}

void ModularSynth_removeConnection(ModularID destId, ModularPortID destPort)
{
  // exclude out of bound id
  if (destId >= MAX_RACK_SIZE) return;

  // check that we have enough space to add a connection
  if (synth->portConnectionsCount >= MAX_CONN_COUNT) return;

  // cheeck that the modules exist
  Module * destMod = getModuleById(destId);

  if (!destMod) return;

  // check that port numbers are valid
  if (destPort >= destMod->getInCount(destMod)) return;

  // they exist and ports are good! disconnect the dest so it does not point to the other port anymore
  destMod->linkToInput(destMod, destPort, NULL);

  // find what it was connected to
  bool found;
  ModuleConnection connection = getConnectionToDestInPort(destId, destPort, &found);
  if (found)
  {
    removeConnectionLog(connection);
  }
}

void ModularSynth_removeConnectionByName(char* destModuleName, char* destPortName)
{
  if (!destModuleName || !destPortName) return;

  bool found;
  ModularID id = getModuleIdByName(destModuleName, &found);
  if (!found) return;

  Module* mod = getModuleById(id);
  ModularPortID port = Module_GetInPortId(mod, destPortName, &found);
  if (!found) return;

  ModularSynth_removeConnection(id, port);
}

bool ModularSynth_setControl(ModularID id, ModularPortID controlID, void* val)
{
  Module * mod = getModuleById(id);

  if (!mod) return 0;

  mod->setControlVal(mod, controlID, val);
  return 1;
}

bool ModularSynth_setControlByName(char * name, char * controlName, void* val)
{
  if (!name || !controlName) return 0;

  Module * mod = getModuleByName(name);
  if (!mod) return 0;
  
  bool found;
  ModularPortID controlID = Module_GetControlId(mod, controlName, &found);
  if (!found) return 0;
  
  mod->setControlVal(mod, controlID, val);
  return 1;
}

void ModularSynth_getControlByName(char * name, char * controlName, void* ret)
{
  if (!name || !controlName || !ret) return;

  Module * mod = getModuleByName(name);
  if (!mod) return;

  bool found;
  ModularPortID controlID = Module_GetControlId(mod, controlName, &found);
  if (!found) return;

  mod->getControlVal(mod, controlID, ret);
}

ModulePortType ModularSynth_getControlTypeByName(char * name, char * controlName)
{
  if (!name || !controlName) return 0;

  Module * mod = getModuleByName(name);
  if (!mod) return 0;

  bool found;
  ModularPortID controlID = Module_GetControlId(mod, controlName, &found);
  if (!found) return 0;

  return mod->getControlType(mod, controlID);
}

bool ModularSynth_readConfig(char * fname)
{
  // read in the config
  bool success = ConfigParser_Parse(&config, fname);
  if (!success) return 0;

  // ok so first we want to make sure all the modules exist (special exception for output module. it is always where regardless)

  // first step then is to add all the modules that do not exist
  // and or change the ones with the same name to the right type
  bool includedModuleIds[MAX_RACK_SIZE];
  memset(includedModuleIds, 0, sizeof(includedModuleIds));
  for (int i = 0; i < config.moduleCount; i++)
  {
    ModuleConfig* modConfig = &config.modules[i];

    if (strcmp(modConfig->type, ModuleTypeNames[ModuleType_OutputModule]) == 0 ||
        strcmp(modConfig->name, outModuleName) == 0) continue;

    bool existingModFound;
    ModularID existingModuleIdWithSameName = getModuleIdByName(modConfig->name, &existingModFound);

    // if it not exist, add it
    if (!existingModFound)
    {
      includedModuleIds[ModularSynth_addModuleByName(modConfig->type, strdup(modConfig->name))] = 1;
    }
    else
    {
      // else there is one with same name, so maybe we can leave it if same type too
      Module * existingModuleWithSameName = getModuleById(existingModuleIdWithSameName);
      if (strcmp(ModuleTypeNames[existingModuleWithSameName->type], modConfig->type) != 0)
      {
        ModularSynth_removeModuleByName(existingModuleWithSameName->name);
        includedModuleIds[ModularSynth_addModuleByName(modConfig->type, strdup(modConfig->name))] = 1;
      }
      else
      {
        // in here means there is a module with the same name and same type too
        includedModuleIds[existingModuleIdWithSameName] = 1;
      }
    }
  }

  // now delete all the modules that dont exist in the config (except the output)
  for (int i = 0; i < MAX_RACK_SIZE; i++)
  {
    if (!synth->moduleIDAvailability[i] || includedModuleIds[i]) continue;
    
    ModularSynth_removeModule(i);
  }

  // for remaining modules, remove all connections
  for (int i = 0; i < synth->modulesCount; i++)
  {
    Module_RemoveAllIncomingConnections(synth->modules[i]);
  }

  // now just manually delete all the connection logs
  synth->portConnectionsCount = 0;

  // now for all the included modules, connect everything up
  for (int i = 0; i < config.moduleCount; i++)
  {
    ModuleConfig* modConfig = &config.modules[i];
    for (int k = 0; k < modConfig->connectionCount; k++)
    {
      ConnectionInfo* connInfo = &modConfig->connections[k];
      ModularSynth_addConnectionByName(connInfo->otherModule, connInfo->otherOutPort, modConfig->name, connInfo->inPort);
    }

    for (int k = 0; k < modConfig->controlCount; k++)
    {
      ControlInfo* ctrlInfo = &modConfig->controls[k];
      ModularSynth_setControlByName(modConfig->name, ctrlInfo->controlName, &ctrlInfo->value);
    }
  }
}

bool ModularSynth_exportConfig(char * fname)
{
  memset(&config, 0, sizeof(config));

  // first get in the raw number of modules
  config.moduleCount = synth->modulesCount;

  for (int i = 0; i < synth->modulesCount; i++)
  {
    Module * mod = synth->modules[i];
    ModularID id = getModuleIdByIdx(i);
    ModuleConfig* modConfig = &config.modules[i];

    memcpy(modConfig->name, mod->name, strlen(mod->name) + 1);
    memcpy(modConfig->type, ModuleTypeNames[mod->type], strlen(ModuleTypeNames[mod->type]) + 1);

    for (int k = 0; k < synth->portConnectionsCount; k++)
    {
      ModuleConnection* conn = &synth->portConnections[k];
      if (conn->destMod == id)
      {
        ConnectionInfo* connInfo = &modConfig->connections[modConfig->connectionCount];
        modConfig->connectionCount++;

        Module * otherMod = getModuleById(conn->srcMod);
        memcpy(connInfo->inPort, mod->inPortNames[conn->destPort], strlen(mod->inPortNames[conn->destPort]) + 1);
        memcpy(connInfo->otherModule, otherMod->name, strlen(otherMod->name) + 1);
        memcpy(connInfo->otherOutPort, otherMod->outPortNames[conn->srcPort], strlen(otherMod->outPortNames[conn->srcPort]) + 1);
      }
    }

    modConfig->controlCount = mod->controlNamesCount;
    for (int k = 0; k < mod->controlNamesCount; k++)
    {
      ControlInfo* ctrlInfo = &modConfig->controls[k];
      memcpy(ctrlInfo->controlName, mod->controlNames[k], strlen(mod->controlNames[k]) + 1);
      bool found;
      ModularPortID portid = Module_GetControlId(mod, ctrlInfo->controlName, &found);
      if (found && mod->getControlType(mod, portid) == ModulePortType_VoltControl)
      {
        mod->getControlVal(mod, k, &ctrlInfo->value);
      }
    }
  }

  ConfigParser_Write(&config, fname);
}

char* ModularSynth_PrintFullModuleInfo(ModularID id)
{
  Module * mod = getModuleById(id);
  
  if (!mod) return NULL;
  
  // Estimate buffer size (roughly)
  size_t bufSize = 256;
  bufSize += 64 * (mod->inPortNamesCount + mod->outPortNamesCount + mod->controlNamesCount);
  
  char* buffer = malloc(bufSize);
  if (!buffer) return NULL;
  buffer[0] = '\0';
  
  // Compose
  snprintf(buffer, bufSize, "Name: %s\n", mod->name ? mod->name : "(unnamed)");
  

  strcat(buffer, "In Ports:\n");
  for (ModularPortID i = 0; i < mod->inPortNamesCount; i++) {
      strcat(buffer, "    ");
      strcat(buffer, mod->inPortNames[i]);
      strcat(buffer, " <- ");
      
      bool found;
      ModuleConnection connection = getConnectionToDestInPort(id, i, &found);
      if (found)
      {
        Module * otherMod = getModuleById(connection.srcMod);
        strcat(buffer, otherMod->name);
        strcat(buffer, ".");
        strcat(buffer, otherMod->outPortNames[connection.srcPort]);
      }
      else
      {
        strcat(buffer, "null");
      }
      
      strcat(buffer, "\n");
  }
  
  strcat(buffer, "Out Ports:\n");
  ModuleConnection connections[MAX_CONN_COUNT];
  for (ModularPortID i = 0; i < mod->outPortNamesCount; i++) {
      strcat(buffer, "    ");
      strcat(buffer, mod->outPortNames[i]);
      strcat(buffer, " -> ");

      U4 count;
      getAllOutPortConnections(id, i, connections, &count);
      for (int k = 0; k < count; k++)
      {
        ModuleConnection connection = connections[k];
        Module * otherMod = getModuleById(connection.destMod);
        strcat(buffer, otherMod->name);
        strcat(buffer, ".");
        strcat(buffer, otherMod->inPortNames[connection.destPort]);

        if (k != count - 1)
        {
          strcat(buffer, ", ");
        }
      }

      if (!count)
      {
        strcat(buffer, "null");
      }

      strcat(buffer, "\n");
  }

  strcat(buffer, "Control Names:\n");
  for (ModularPortID i = 0; i < mod->controlNamesCount; i++) {
      strcat(buffer, "    ");
      strcat(buffer, mod->controlNames[i]);
      strcat(buffer, "\n");
  }

  return buffer; // caller must free()
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static Module * getModuleByName(char * name)
{
  for (int i = 0; i < synth->modulesCount; i++)
  {
    if (strcmp(synth->modules[i]->name, name) == 0) return synth->modules[i];
  }

  return NULL;
}

static ModularID getModuleIdByIdx(int idx)
{
  for (int i = 0; i < MAX_RACK_SIZE; i++)
  {
    if (!synth->moduleIDAvailability[i] && synth->moduleIDtoIdx[i] == idx) return i;
  }

  return -1;
}

static ModularID getModuleIdByName(char * name, bool* found)
{
  for (int id = 0; id < MAX_RACK_SIZE; id++)
  {
    if (synth->moduleIDAvailability[id] == 0)
    {
      U2 idx = synth->moduleIDtoIdx[id];
      Module * mod = synth->modules[idx];
      if (strcmp(mod->name, name) == 0)
      {
        *found = 1;
        return id;
      }
    }
  }

  *found = 0;
}

static Module * getModuleById(ModularID id)
{
  // check id bounds
  if (id >= MAX_RACK_SIZE) return NULL;

  // check if module does not exist
  if (synth->moduleIDAvailability[id]) return NULL;

  return synth->modules[synth->moduleIDtoIdx[id]];
}

static bool connectionLogEq(ModuleConnection a, ModuleConnection b)
{
  return a.destMod == b.destMod &&
        a.destPort == b.destPort &&
        a.srcMod   == b.srcMod &&
        a.srcPort  == b.srcPort;
}

static bool connectionExists(ModuleConnection connection)
{
  for (int i = 0; i < synth->portConnectionsCount; i++)
  {
    if (connectionLogEq(synth->portConnections[i], connection)) return 1;
  }
  return 0;
}

static void logConnection(ModuleConnection connection)
{
  if (connectionExists(connection)) return;
  if (synth->portConnectionsCount >= MAX_CONN_COUNT) return;

  synth->portConnections[synth->portConnectionsCount] = connection;
  synth->portConnectionsCount++;
}

static void removeConnectionLog(ModuleConnection connection)
{
  for (int i = 0; i < synth->portConnectionsCount; i++)
  {
    if (connectionLogEq(synth->portConnections[i], connection))
    {
      ARRAY_SHIFT(synth->portConnections, ModuleConnection, i + 1, synth->portConnectionsCount - 1, i);
      synth->portConnectionsCount--;
      return;
    }
  }
}

static void getAllInPortConnections(ModularID id, ModuleConnection * connections, U4 * connectionsCount)
{
  U4 count = 0;
  for (int i = 0; i < synth->portConnectionsCount; i++)
  {
    if (synth->portConnections[i].destMod == id)
    {
      connections[count] = synth->portConnections[i];
      count++;
    }
  }
  *connectionsCount = count;
}

static void getAllOutPortConnections(ModularID id, ModularPortID port, ModuleConnection * connections, U4 * connectionsCount)
{
  U4 count = 0;
  for (int i = 0; i < synth->portConnectionsCount; i++)
  {
    if (synth->portConnections[i].srcMod == id && synth->portConnections[i].srcPort == port)
    {
      connections[count] = synth->portConnections[i];
      count++;
    }
  }
  *connectionsCount = count;
}

static ModuleConnection getConnectionToDestInPort(ModularID id, ModularPortID port, bool* found)
{
  for (int i = 0; i < synth->portConnectionsCount; i++)
  {
    if (synth->portConnections[i].destMod == id && synth->portConnections[i].destPort == port)
    {
      *found = 1;
      return synth->portConnections[i];
    }
  }
  *found = 0;
}

static bool addConnectionHelper(Module * srcMod, ModularID srcId, ModularPortID srcPort, Module * destMod, ModularID destId, ModularPortID destPort)
{
  // check that port numbers are valid
  if (srcPort >= srcMod->getOutCount(srcMod)) return 0;
  if (destPort >= destMod->getInCount(destMod)) return 0;
  
  // check that the destination is not hooked up to something already
  bool found;
  ModuleConnection connection = getConnectionToDestInPort(destId, destPort, &found);
  if (found) return 0;

  ModulePortType destType = destMod->getInputType(destMod, destPort);
  ModulePortType srcType = srcMod->getOutputType(srcMod, srcPort);

  if (destType != srcType || srcType == ModulePortType_None || destType == ModulePortType_None) return 0;

  // they exist and ports are good! link them
  destMod->linkToInput(destMod, destPort, srcMod->getOutputAddr(srcMod, srcPort));

  logConnection((ModuleConnection)
    { 
      .srcPort = srcPort, 
      .destPort = destPort, 
      .srcMod = srcId, 
      .destMod = destId
    });

  return 1;
}

void* updateWorkerThread(void *arg)
{
  int id = (intptr_t)arg;
  while (synth->threadpool.running) 
  {
      pthread_barrier_wait(&synth->threadpool.barrier);  // wait for frame start signal

      if (!synth->threadpool.running) break;

      int phase = atomic_load(&synth->threadpool.phase);
      U4 moduleCount = atomic_load(&synth->threadpool.moduleCount);

      switch (phase)
      {
        case UPDATE_MOD_PHASE:
        {
          for (int m = id; m < moduleCount; m += MAX_SYNTH_THREADS)
          {
            Module *module = synth->modules[m];
            module->updateState(module);
          }
          break;
        }

        case PUSH_BUFFERS_PHASE:
        {
          for (int m = id; m < moduleCount; m += MAX_SYNTH_THREADS) 
          {
            Module *module = synth->modules[m];
            module->pushCurrToPrev(module);
          }
          break;
        }

        case SHUTDOWN_PHASE:
        {
          return NULL;
        }
      }

      pthread_barrier_wait(&synth->threadpool.barrier);  // sync at frame end
  }

  return NULL;
}
