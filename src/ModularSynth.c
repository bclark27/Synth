#include "ModularSynth.h"

#include "AudioSettings.h"
#include "ModuleFactory.h"

//////////////
// DEFINES  //
//////////////

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static Module * getModuleById(ModularSynth * synth, ModularID id);

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

  // cheeck that the modules exist
  Module * srcMod = getModuleById(synth, srcId);
  Module * destMod = getModuleById(synth, destId);

  if (!srcMod || !destMod) return 0;

  // check that port numbers are valid
  if (srcPort >= srcMod->getOutCount(srcMod)) return 0;
  if (destPort >= destMod->getInCount(destMod)) return 0;

  // they exist and ports are good! link them
  destMod->linkToInput(destMod, destPort, srcMod->getOutputAddr(srcMod, srcPort));

  return 1;
}

bool ModularSynth_removeConnection(ModularSynth * synth, ModularID srcId, ModularPortID srcPort, ModularID destId, ModularPortID destPort)
{

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
  for (int i = 0; i < mod->inPortNamesCount; i++) {
      strcat(buffer, "    ");
      strcat(buffer, mod->inPortNames[i]);
      strcat(buffer, "\n");
  }

  strcat(buffer, "Out Ports:\n");
  for (int i = 0; i < mod->outPortNamesCount; i++) {
      strcat(buffer, "    ");
      strcat(buffer, mod->outPortNames[i]);
      strcat(buffer, "\n");
  }

  strcat(buffer, "Control Names:\n");
  for (int i = 0; i < mod->controlNamesCount; i++) {
      strcat(buffer, "    ");
      strcat(buffer, mod->controlNames[i]);
      strcat(buffer, "\n");
  }

  return buffer; // caller must free()
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static Module * getModuleById(ModularSynth * synth, ModularID id)
{
  // check id bounds
  if (id >= MAX_RACK_SIZE) return NULL;

  // check if module does not exist
  if (synth->moduleIDAvailability[id]) return NULL;

  return synth->modules[synth->moduleIDtoIdx[id]];
}
