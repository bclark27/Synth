#ifndef MODULE_FACTORY_H_
#define MODULE_FACTORY_H_

#include "comm/Common.h"
#include "modules/Module.h"

// all known modules:
#include "modules/ADSR.h"
#include "modules/Attenuverter.h"
#include "modules/Clock.h"
#include "modules/ClockMult.h"
#include "modules/Mixer.h"
#include "modules/OutputModule.h"
#include "modules/Sequencer.h"
#include "modules/VCO.h"
#include "modules/Filter.h"
#include "modules/MidiInput.h"
#include "modules/PolyKeys.h"
#include "modules/LFO.h"
#include "modules/Noise.h"
#include "modules/SampleHold.h"
#include "modules/Slew.h"
#include "modules/QuantizeCv.h"
#include "modules/GateExtender.h"

///////////
// TYPES //
///////////

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * ModuleFactory_createModule(ModuleType type, char * name);
void ModuleFactory_destroyModule(Module * module);

#endif
