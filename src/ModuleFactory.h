#ifndef MODULE_FACTORY_H_
#define MODULE_FACTORY_H_

#include "comm/Common.h"
#include "Module.h"

// all known modules:
#include "ADSR.h"
#include "Attenuator.h"
#include "Clock.h"
#include "ClockMult.h"
#include "Mixer.h"
#include "OutputModule.h"
#include "Sequencer.h"
#include "VCO.h"

///////////
// TYPES //
///////////

typedef enum ModuleType
{
  ModuleType_ADSR,
  ModuleType_Attenuator,
  ModuleType_Clock,
  ModuleType_ClockMult,
  ModuleType_Mixer,
  ModuleType_OutputModule,
  ModuleType_Sequencer,
  ModuleType_VCO,
} ModuleType;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * ModuleFactory_createModule(ModuleType type);
void ModuleFactory_destroyModule(Module * module);

#endif
