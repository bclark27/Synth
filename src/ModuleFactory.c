#include "ModuleFactory.h"

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * ModuleFactory_createModule(ModuleType type)
{
  Module * mod = NULL;
  switch (type)
  {
    case ModuleType_OutputModule:
    mod = OutputModule_init();
    break;

    case ModuleType_Clock:
    mod = Clock_init();
    break;

    case ModuleType_ClockMult:
    mod = ClockMult_init();
    break;

    case ModuleType_VCO:
    mod = VCO_init();
    break;

    case ModuleType_Mixer:
    mod = Mixer_init();
    break;

    case ModuleType_ADSR:
    mod = ADSR_init();
    break;

    case ModuleType_Sequencer:
    mod = Sequencer_init();
    break;

    case ModuleType_Attenuator:
    mod = Attenuator_init();
    break;
  }

  return mod;
}

void ModuleFactory_destroyModule(Module * module)
{
  module->freeModule(module);
}
