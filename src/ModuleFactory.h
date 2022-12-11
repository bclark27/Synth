#ifndef MODULE_FACTORY_H_
#define MODULE_FACTORY_H_

#include "comm/Common.h"
#include "Module.h"

// all known modules:
#include "VCO.h"
#include "Mixer.h"

///////////
// TYPES //
///////////

typedef enum ModuleType
{
  ModuleType_VCO,
  ModuleType_Mixer,
} ModuleType;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * ModuleFactory_createModule(ModuleType type);
void ModuleFactory_destroyModule(Module * module);

#endif
