#ifndef MODULE_FACTORY_H_
#define MODULE_FACTORY_H_

#include "comm/Common.h"
#include "Module.h"

// all known modules:
#include "OutputModule.h"
#include "Clock.h"
#include "ClockMult.h"
#include "VCO.h"
#include "Mixer.h"
#include "ADSR.h"
#include "Sequencer.h"

///////////
// TYPES //
///////////

typedef enum ModuleType
{
  ModuleType_OutputModule,
  ModuleType_Clock,
  ModuleType_ClockMult,
  ModuleType_VCO,
  ModuleType_Mixer,
  ModuleType_ADSR,
  ModuleType_Sequencer,
} ModuleType;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * ModuleFactory_createModule(ModuleType type);
void ModuleFactory_destroyModule(Module * module);

#endif
