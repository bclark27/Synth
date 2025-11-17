#ifndef MODULE_FACTORY_H_
#define MODULE_FACTORY_H_

#include "comm/Common.h"
#include "Module.h"

// all known modules:
#include "ADSR.h"
#include "Attenuverter.h"
#include "Clock.h"
#include "ClockMult.h"
#include "Mixer.h"
#include "OutputModule.h"
#include "Sequencer.h"
#include "VCO.h"
#include "Filter.h"
#include "MidiInput.h"
#include "PolyKeys.h"
#include "LFO.h"
#include "Noise.h"
#include "SampleHold.h"

///////////
// TYPES //
///////////

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * ModuleFactory_createModule(ModuleType type, char * name);
void ModuleFactory_destroyModule(Module * module);

#endif
