#include "ModuleFactory.h"

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * ModuleFactory_createModule(ModuleType type)
{
  Module * mod = NULL;
  switch (type)
  {
    case ModuleType_VCO:
    mod = VCO_init();
    break;

    case ModuleType_Mixer:
    mod = Mixer_init();
    break;
  }

  return mod;
}

void ModuleFactory_destroyModule(Module * module)
{
  module->freeModule(module);
}
