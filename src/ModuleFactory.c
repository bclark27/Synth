#include "ModuleFactory.h"


//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


Module * ModuleFactory_createModule(ModuleType type, char * name)
{
  Module * mod = NULL;
  switch (type)
  {
    case ModuleType_OutputModule:
    mod = OutputModule_init(name);
    break;

    case ModuleType_Clock:
    mod = Clock_init(name);
    break;

    case ModuleType_ClockMult:
    mod = ClockMult_init(name);
    break;

    case ModuleType_VCO:
    mod = VCO_init(name);
    break;

    case ModuleType_Mixer:
    mod = Mixer_init(name);
    break;

    case ModuleType_ADSR:
    mod = ADSR_init(name);
    break;

    case ModuleType_Sequencer:
    mod = Sequencer_init(name);
    break;

    case ModuleType_Attenuverter:
    mod = Attenuverter_init(name);
    break;

    case ModuleType_Filter:
    mod = Filter_init(name);
    break;

    case ModuleType_MidiInput:
    mod = MidiInput_init(name);
    break;

    case ModuleType_PolyKeys:
    mod = PolyKeys_init(name);
    break;

    case ModuleType_LFO:
    mod = LFO_init(name);
    break;
  }

  return mod;
}

void ModuleFactory_destroyModule(Module * module)
{
  module->freeModule(module);
}
