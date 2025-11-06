#include "ModularSynth.h"

#include "AudioSettings.h"
#include "ModuleFactory.h"

//////////////
// DEFINES  //
//////////////

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static Module * getModuleByName(ModularSynth * synth, char * name);
static ModularID getModuleIdByName(ModularSynth * synth, char * name, bool* found);
static Module * getModuleById(ModularSynth * synth, ModularID id);
static bool connectionLogEq(ModuleConnection a, ModuleConnection b);
static bool connectionExists(ModularSynth * synth, ModuleConnection connection);
static void logConnection(ModularSynth * synth, ModuleConnection connection);
static void removeConnectionLog(ModularSynth * synth, ModuleConnection connection);
static void getAllInPortConnections(ModularSynth * synth, ModularID id, ModuleConnection * connections, U4 * connectionsCount);
static void getAllOutPortConnections(ModularSynth * synth, ModularID id, ModularPortID port, ModuleConnection * connections, U4 * connectionsCount);
static ModuleConnection getConnectionToDestInPort(ModularSynth * synth, ModularID id, ModularPortID port, bool* found);
static bool addConnectionHelper(ModularSynth * synth, Module * srcMod, ModularID srcId, ModularPortID srcPort, Module * destMod, ModularID destId, ModularPortID destPort);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static const char * outModuleName = "OUT_MOD";

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

ModularSynth * ModularSynth_init(void)
{
  ModularSynth * synth = calloc(1, sizeof(ModularSynth));

  synth->modulesCount = 1;
  synth->moduleIDtoIdx[OUT_MODULE_ID] = OUT_MODULE_IDX;
  memset(synth->moduleIDAvailability, 1, MAX_RACK_SIZE * sizeof(U1));
  synth->moduleIDAvailability[OUT_MODULE_ID] = 0;

  Module * outMod = ModuleFactory_createModule(ModuleType_OutputModule, strdup(outModuleName));
  synth->modules[OUT_MODULE_IDX] = outMod;

  // get output modules out buffers
  synth->outModuleLeft = outMod->getOutputAddr(outMod, OUTPUT_OUT_PORT_LEFT);
  synth->outModuleRight = outMod->getOutputAddr(outMod, OUTPUT_OUT_PORT_RIGHT);

  return synth;
}

void ModularSynth_free(ModularSynth * synth)
{
  for (U4 i = 0; i < MAX_RACK_SIZE; i++)
  {
    Module * module = getModuleById(synth, i);
    if (module)
    {
      module->freeModule(module);
    }
  }

  free(synth);
}

R4 * ModularSynth_getLeftChannel(ModularSynth * synth)
{
  return synth->outputBufferLeft;
}

R4 * ModularSynth_getRightChannel(ModularSynth * synth)
{
  return synth->outputBufferRight;
}

void ModularSynth_update(ModularSynth * synth)
{
  // update all states, then push to out

  Module * module;
  U4 modulesCount = synth->modulesCount;
  R4 * currOutputPtrLeft = synth->outputBufferLeft;
  R4 * currOutputPtrRight = synth->outputBufferRight;


  for (U4 i = 0; i < MODULE_BUFS_PER_STREAM_BUF; i++)
  {
    // update the modules
    for (U4 m = 0; m < modulesCount; m++)
    {
      module = synth->modules[m];
      module->updateState(module);
    }

    // push updates
    for (U4 m = 0; m < modulesCount; m++)
    {
      module = synth->modules[m];
      module->pushCurrToPrev(module);
    }

    // output module is always idx 0 of modules[]. cpy from output
    // to the left and right output channels
    memcpy(currOutputPtrLeft, synth->outModuleLeft, MODULE_BUFFER_SIZE * sizeof(R4));
    memcpy(currOutputPtrRight, synth->outModuleLeft, MODULE_BUFFER_SIZE * sizeof(R4));

    // update the ptrs to the next spot in the out buffer
    currOutputPtrLeft += MODULE_BUFFER_SIZE;
    currOutputPtrRight += MODULE_BUFFER_SIZE;
  }
}

ModularID ModularSynth_addModule(ModularSynth * synth, ModuleType type, char * name)
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

  // add ptr to the end of the list
  U2 idx = synth->modulesCount;
  synth->modules[idx] = module;

  // make sure the len is now +1
  synth->modulesCount += 1;

  // take that idx, and sotre in the id->idx table
  synth->moduleIDtoIdx[id] = idx;

  // mark the id as unavailable
  synth->moduleIDAvailability[id] = 0;

  printf("Adding Module\n\tName: %s\n", module->name);

  return id;
}

bool ModularSynth_removeModule(ModularSynth * synth, ModularID id)
{
  // exclude output module from deletion
  if (id == ModuleType_OutputModule) return 0;

  // exclude out of bound id
  if (id >= MAX_RACK_SIZE) return 0;

  // check to see it is an unavailable id
  // if the id is available, there is nothing to delete
  if (synth->moduleIDAvailability[id]) return 0;

  // get the idx
  U2 idx = synth->moduleIDtoIdx[id];

  // check the ptr is NULL
  if (synth->modules[idx] == NULL) return 0;

  // disconnect all other modules from this guys outputs
  // if other module was connected, set its input to NULL or a black space

  Module * delMod = synth->modules[idx];

  for (U2 i = 0; i < synth->modulesCount; i++)
  {
    if (i == idx) continue;

    Module * thisMod = synth->modules[i];
    for (U2 delModOutPort = 0; delModOutPort < delMod->getOutCount(delMod); delModOutPort++)
    {
      for (U2 thisModInPort = 0; thisModInPort < thisMod->getOutCount(thisMod); thisModInPort++)
      {
        if (thisMod->getInputAddr(thisMod, thisModInPort) == delMod->getOutputAddr(delMod, delModOutPort))
        {
          thisMod->linkToInput(thisMod, thisModInPort, NULL);
        }
      }
    }

  }
  
  // TODO: need to go in here and remove the book keeping about the connections

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
  for (U4 i = idx; i < synth->modulesCount - 1; i++)
  {
    synth->modules[i] = synth->modules[i + 1];
  }

  // mark the id as available
  synth->moduleIDAvailability[id] = 1;

  // decrement the total count
  synth->modulesCount--;

  return 1;
}

bool ModularSynth_addConnection(ModularSynth * synth, ModularID srcId, ModularPortID srcPort, ModularID destId, ModularPortID destPort)
{
  // exclude output module as a source
  if (srcId == OUT_MODULE_ID) return 0;

  // exclude out of bound id
  if (srcId >= MAX_RACK_SIZE || destId >= MAX_RACK_SIZE) return 0;

  // check that we have enough space to add a connection
  if (synth->portConnectionsCount >= MAX_CONN_COUNT) return 0;

  // cheeck that the modules exist
  Module * srcMod = getModuleById(synth, srcId);
  Module * destMod = getModuleById(synth, destId);

  if (!srcMod || !destMod) return 0;

  return addConnectionHelper(synth, srcMod, srcId, srcPort, destMod, destId, destPort);
}

bool ModularSynth_addConnectionByName(ModularSynth * synth, char* srcModuleName, char* srcPortName, char* destModuleName, char* destPortName)
{
  if (!srcModuleName || !srcPortName || !destModuleName || !destPortName) return 0;

  bool srcFound;
  bool destFound;
  ModularID srcId = getModuleIdByName(synth, srcModuleName, &srcFound);
  ModularID destId = getModuleIdByName(synth, destModuleName, &destFound);
  if (!srcFound || !destFound) return 0;
  
  Module * srcMod = getModuleById(synth, srcId);
  Module * destMod = getModuleById(synth, destId);
  if (!srcMod || !destMod) return 0;
  
  ModularPortID srcPort = Module_GetOutPortId(srcMod, srcPortName, &srcFound);
  ModularPortID destPort = Module_GetInPortId(destMod, destPortName, &destFound);
  if (!srcFound || !destFound) return 0;

  return addConnectionHelper(synth, srcMod, srcId, srcPort, destMod, destId, destPort);
}

void ModularSynth_removeConnection(ModularSynth * synth, ModularID destId, ModularPortID destPort)
{
  // exclude out of bound id
  if (destId >= MAX_RACK_SIZE) return;

  // check that we have enough space to add a connection
  if (synth->portConnectionsCount >= MAX_CONN_COUNT) return;

  // cheeck that the modules exist
  Module * destMod = getModuleById(synth, destId);

  if (!destMod) return;

  // check that port numbers are valid
  if (destPort >= destMod->getInCount(destMod)) return;

  // they exist and ports are good! disconnect the dest so it does not point to the other port anymore
  destMod->linkToInput(destMod, destPort, NULL);

  // find what it was connected to
  bool found;
  ModuleConnection connection = getConnectionToDestInPort(synth, destId, destPort, &found);
  if (found)
  {
    removeConnectionLog(synth, connection);
  }
}

bool ModularSynth_setControl(ModularSynth * synth, ModularID id, ModularPortID controlID, R4 val)
{
  Module * mod = getModuleById(synth, id);

  if (!mod) return 0;

  mod->setControlVal(mod, controlID, val);
  return 1;
}

char* ModularSynth_PrintFullModuleInfo(ModularSynth * synth, ModularID id)
{
  Module * mod = getModuleById(synth, id);

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
      ModuleConnection connection = getConnectionToDestInPort(synth, id, i, &found);
      if (found)
      {
        Module * otherMod = getModuleById(synth, connection.srcMod);
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
      getAllOutPortConnections(synth, id, i, connections, &count);
      for (int k = 0; k < count; k++)
      {
        ModuleConnection connection = connections[k];
        Module * otherMod = getModuleById(synth, connection.destMod);
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

static Module * getModuleByName(ModularSynth * synth, char * name)
{
  for (int i = 0; i < synth->modulesCount; i++)
  {
    if (strcmp(synth->modules[i]->name, name) == 0) return synth->modules[i];
  }

  return NULL;
}

static ModularID getModuleIdByName(ModularSynth * synth, char * name, bool* found)
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

static Module * getModuleById(ModularSynth * synth, ModularID id)
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

static bool connectionExists(ModularSynth * synth, ModuleConnection connection)
{
  for (int i = 0; i < synth->portConnectionsCount; i++)
  {
    if (connectionLogEq(synth->portConnections[i], connection)) return 1;
  }
  return 0;
}

static void logConnection(ModularSynth * synth, ModuleConnection connection)
{
  if (connectionExists(synth, connection)) return;
  if (synth->portConnectionsCount >= MAX_CONN_COUNT) return;

  synth->portConnections[synth->portConnectionsCount] = connection;
  synth->portConnectionsCount++;
}

static void removeConnectionLog(ModularSynth * synth, ModuleConnection connection)
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

static void getAllInPortConnections(ModularSynth * synth, ModularID id, ModuleConnection * connections, U4 * connectionsCount)
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

static void getAllOutPortConnections(ModularSynth * synth, ModularID id, ModularPortID port, ModuleConnection * connections, U4 * connectionsCount)
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

static ModuleConnection getConnectionToDestInPort(ModularSynth * synth, ModularID id, ModularPortID port, bool* found)
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

static bool addConnectionHelper(ModularSynth * synth, Module * srcMod, ModularID srcId, ModularPortID srcPort, Module * destMod, ModularID destId, ModularPortID destPort)
{
  // check that port numbers are valid
  if (srcPort >= srcMod->getOutCount(srcMod)) return 0;
  if (destPort >= destMod->getInCount(destMod)) return 0;
  
  // check that the destination is not hooked up to something already
  bool found;
  ModuleConnection connection = getConnectionToDestInPort(synth, destId, destPort, &found);
  if (found) return 0;

  // they exist and ports are good! link them
  destMod->linkToInput(destMod, destPort, srcMod->getOutputAddr(srcMod, srcPort));

  logConnection(synth, (ModuleConnection)
    { 
      .srcPort = srcPort, 
      .destPort = destPort, 
      .srcMod = srcId, 
      .destMod = destId
    });

  return 1;
}