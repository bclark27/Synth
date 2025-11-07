#ifndef MODULE_H_
#define MODULE_H_

#include "comm/Common.h"
#include "AudioSettings.h"

///////////
// TYPES //
///////////

typedef enum ModuleType
{
  ModuleType_ADSR=0,
  ModuleType_Attenuator,
  ModuleType_Clock,
  ModuleType_ClockMult,
  ModuleType_Mixer,
  ModuleType_OutputModule,
  ModuleType_Sequencer,
  ModuleType_VCO,

  ModuleType_COUNT,
} ModuleType;

typedef U2 ModularPortID;

typedef struct Module
{
  ModuleType type;
  void (*freeModule)(void*);
  void (*updateState)(void*);
  void (*pushCurrToPrev)(void*);
  R4* (*getOutputAddr)(void*,ModularPortID);
  R4* (*getInputAddr)(void*,ModularPortID);
  U4 (*getInCount)(void*);
  U4 (*getOutCount)(void*);
  U4 (*getContolCount)(void*);
  void (*setControlVal)(void*,ModularPortID,R4);
  R4 (*getControlVal)(void*,ModularPortID);
  void (*linkToInput)(void*,ModularPortID,R4*);

  char* name;
  int inPortNamesCount;
  char** inPortNames;
  int outPortNamesCount;
  char** outPortNames;
  int controlNamesCount;
  char** controlNames;
} Module;

extern const char * const ModuleTypeNames[ModuleType_COUNT];

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

ModuleType Module_GetModuleTypeByName(char* name, bool* found);
ModularPortID Module_GetInPortId(Module* mod, char* name, bool* found);
ModularPortID Module_GetOutPortId(Module* mod, char* name, bool* found);
ModularPortID Module_GetControlId(Module* mod, char* name, bool* found);

#endif
