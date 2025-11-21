#ifndef MODULE_H_
#define MODULE_H_

#include "../comm/Common.h"
#include "../AudioSettings.h"
#include "../MIDI.h"
#include "../VoltUtils.h"

///////////////
//  DEFINES  //
///////////////

#define MAX_RACK_SIZE   200
#define MAX_CONN_COUNT  1000
#define MAX_MODULE_NAME_LEN 32
#define MAX_PORT_COUNT 64

///////////
// TYPES //
///////////

typedef enum ModuleType
{
  ModuleType_ADSR=0,
  ModuleType_Attenuverter,
  ModuleType_Clock,
  ModuleType_ClockMult,
  ModuleType_Mixer,
  ModuleType_OutputModule,
  ModuleType_Sequencer,
  ModuleType_VCO,
  ModuleType_Filter,
  ModuleType_MidiInput,
  ModuleType_PolyKeys,
  ModuleType_LFO,
  ModuleType_Noise,
  ModuleType_SampleHold,
  ModuleType_Slew,
  ModuleType_QuantizeCv,
  ModuleType_GateExtender,

  ModuleType_COUNT,
} ModuleType;

typedef enum ModulePortType
{
  ModulePortType_None=0,
  ModulePortType_VoltStream,
  ModulePortType_VoltControl,
  ModulePortType_MIDIStream,
  ModulePortType_MIDIControl,
  ModulePortType_Count,
} ModulePortType;

typedef R4 Volt;
typedef Volt* VoltStream;
typedef MIDIData* MIDISataStream;

typedef U2 ModularPortID;
typedef U2 ModularID;

typedef struct Module
{
  ModuleType type;
  void (*freeModule)(void*);
  void (*updateState)(void*);
  void (*pushCurrToPrev)(void*);
  void* (*getOutputAddr)(void*,ModularPortID);
  void* (*getInputAddr)(void*,ModularPortID);
  ModulePortType (*getInputType)(void*,ModularPortID);
  ModulePortType (*getOutputType)(void*,ModularPortID);
  ModulePortType (*getControlType)(void*,ModularPortID);
  U4 (*getInCount)(void*);
  U4 (*getOutCount)(void*);
  U4 (*getContolCount)(void*);
  void (*setControlVal)(void*,ModularPortID,void*);
  void (*getControlVal)(void*,ModularPortID,void*); // caller should provide a buffer large enough
  void (*linkToInput)(void*,ModularPortID,void*);

  char* name;
  int inPortNamesCount;
  char** inPortNames;
  int outPortNamesCount;
  char** outPortNames;
  int controlNamesCount;
  char** controlNames;
} Module;

extern const char * const ModuleTypeNames[ModuleType_COUNT];

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

ModuleType Module_GetModuleTypeByName(char* name, bool* found);
ModularPortID Module_GetInPortId(Module* mod, char* name, bool* found);
ModularPortID Module_GetOutPortId(Module* mod, char* name, bool* found);
ModularPortID Module_GetControlId(Module* mod, char* name, bool* found);
void Module_RemoveAllIncomingConnections(Module* mod);

#endif
