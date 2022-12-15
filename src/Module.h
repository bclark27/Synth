#ifndef MODULE_H_
#define MODULE_H_

#include "comm/Common.h"
#include "AudioSettings.h"

///////////
// TYPES //
///////////

typedef struct Module
{
  void (*freeModule)(void*);
  void (*updateState)(void*);
  void (*pushCurrToPrev)(void*);
  R4* (*getOutputAddr)(void*,U4);
  R4* (*getInputAddr)(void*,U4);
  U4 (*getInCount)(void*);
  U4 (*getOutCount)(void*);
  U4 (*getContolCount)(void*);
  void (*setControlVal)(void*,U4,R4);
  R4 (*getControlVal)(void*,U4);
  void (*linkToInput)(void*,U4,R4*);
} Module;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

#endif
