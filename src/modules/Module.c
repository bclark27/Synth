#include "Module.h"

//////////////
// DEFINES  //
//////////////

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

const char * const ModuleTypeNames[ModuleType_COUNT] = {
    "ADSR",
    "Attenuverter",
    "Clock",
    "ClockMult",
    "Mixer",
    "OutputModule",
    "Sequencer",
    "VCO",
    "Filter",
    "MidiInput",
    "PolyKeys",
    "LFO",
    "Noise",
    "SampleHold",
    "Slew",
    "QuantizeCv",
    "GateExtender",
  };
  
/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

ModuleType Module_GetModuleTypeByName(char* name, bool* found)
{
    if (!name)
    {
        *found = 0;
        return 0;
    }

    for (int i = 0; i < ModuleType_COUNT; i++)
    {
        if (strcmp(name, ModuleTypeNames[i]) == 0)
        {
            *found = 1;
            return i;
        }
    }

    *found = 0;
    return 0;
}

ModularPortID Module_GetInPortId(Module* mod, char* name, bool* found)
{
    if (!mod || !name)
    {
        *found = 0;
        return 0;
    }

    for (int i = 0; i < mod->inPortNamesCount; i++)
    {
        if (strcmp(name, mod->inPortNames[i]) == 0)
        {
            *found = 1;
            return i;
        }
    }

    *found = 0;
    return 0;
}

ModularPortID Module_GetOutPortId(Module* mod, char* name, bool* found)
{
    if (!mod || !name)
    {
        *found = 0;
        return 0;
    }

    for (int i = 0; i < mod->outPortNamesCount; i++)
    {
        if (strcmp(name, mod->outPortNames[i]) == 0)
        {
            *found = 1;
            return i;
        }
    }

    *found = 0;
    return 0;
}

ModularPortID Module_GetControlId(Module* mod, char* name, bool* found)
{
    if (!mod || !name)
    {
        *found = 0;
        return 0;
    }

    for (int i = 0; i < mod->controlNamesCount; i++)
    {
        if (strcmp(name, mod->controlNames[i]) == 0)
        {
            *found = 1;
            return i;
        }
    }

    *found = 0;
    return 0;
}

void Module_RemoveAllIncomingConnections(Module* mod)
{
    if (!mod) return;

    for (int i = 0; i < mod->inPortNamesCount; i++)
    {
        mod->linkToInput(mod, i, NULL);
    }
}