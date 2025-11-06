#ifndef MODULE_H_
#define MODULE_H_

#include "comm/Common.h"
#include "AudioSettings.h"

///////////
// TYPES //
///////////

typedef U2 ModularPortID;

typedef struct Module
{
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

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

#endif
