#include "Mixer.h"

#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define IN_PORT_ADDR(mod, port)           (((Mixer*)(mod))->inputPorts[port]);

#define PREV_PORT_ADDR(mod, port)         (((Mixer*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)         (((Mixer*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_PORT_AUDIO0(mix)               ((mix)->inputPorts[MIXER_IN_PORT_AUDIO0])
#define IN_PORT_AUDIO1(mix)               ((mix)->inputPorts[MIXER_IN_PORT_AUDIO1])
#define IN_PORT_AUDIO2(mix)               ((mix)->inputPorts[MIXER_IN_PORT_AUDIO2])
#define IN_PORT_AUDIO3(mix)               ((mix)->inputPorts[MIXER_IN_PORT_AUDIO3])
#define IN_PORT_VOL(mix)                  ((mix)->inputPorts[MIXER_IN_PORT_VOL])

#define OUT_PORT_SUM(mix)                 (CURR_PORT_ADDR(mix, MIXER_OUT_PORT_SUM))

#define GET_CONTROL_CURR_VOL(mix)         ((mix)->controlsCurr[MIXER_CONTROL_VOL])
#define GET_CONTROL_PREV_VOL(mix)         ((mix)->controlsPrev[MIXER_CONTROL_VOL])

#define SET_CONTROL_CURR_VOL(mix, vol)    ((mix)->controlsCurr[MIXER_CONTROL_VOL] = (vol))
#define SET_CONTROL_PREV_VOL(mix, vol)    ((mix)->controlsPrev[MIXER_CONTROL_VOL] = (vol))

#define CONTROL_PUSH_TO_PREV(vco)         for (U4 i = 0; i < MIXER_CONTROLCOUNT; i++) {(vco)->controlsPrev[i] = (vco)->controlsCurr[i];} for (U4 i = 0; i < MIXER_MIDI_CONTROLCOUNT; i++) {(vco)->midiControlsPrev[i] = (vco)->midiControlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_mixer(void * modPtr);
static void updateState(void * modPtr);
static void pushCurrToPrev(void * modPtr);
static void * getOutputAddr(void * modPtr, ModularPortID port);
static void * getInputAddr(void * modPtr, ModularPortID port);
static ModulePortType getInputType(void * modPtr, ModularPortID port);
static ModulePortType getOutputType(void * modPtr, ModularPortID port);
static ModulePortType getControlType(void * modPtr, ModularPortID port);
static U4 getInCount(void * modPtr);
static U4 getOutCount(void * modPtr);
static U4 getControlCount(void * modPtr);
static void setControlVal(void * modPtr, ModularPortID id, void* val);
static void getControlVal(void * modPtr, ModularPortID id, void* ret);
static void linkToInput(void * modPtr, ModularPortID port, void * readAddr);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static char * inPortNames[MIXER_INCOUNT] = {
  "Input0",
  "Input1",
  "Input2",
  "Input3",
  "Volume",
};

static char * outPortNames[MIXER_OUTCOUNT] = {
  "Mix",
};

static char * controlNames[MIXER_CONTROLCOUNT] = {
  "Volume",
};

static Module vtable = {
  .type = ModuleType_Mixer,
  .freeModule = free_mixer,
  .updateState = updateState,
  .pushCurrToPrev = pushCurrToPrev,
  .getOutputAddr = getOutputAddr,
  .getInputAddr = getInputAddr,
  .getInputType = getInputType,
  .getOutputType = getOutputType,
  .getControlType = getControlType,
  .getInCount = getInCount,
  .getOutCount = getOutCount,
  .getContolCount = getControlCount,
  .setControlVal = setControlVal,
  .getControlVal = getControlVal,
  .linkToInput = linkToInput,
  .inPortNames = inPortNames,
  .inPortNamesCount = ARRAY_LEN(inPortNames),
  .outPortNames = outPortNames,
  .outPortNamesCount = ARRAY_LEN(outPortNames),
  .controlNames = controlNames,
  .controlNamesCount = ARRAY_LEN(controlNames),
};

#define DEFAULT_CONTROL_VOL   VOLTSTD_MOD_CV_MID

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * Mixer_init(char* name)
{
  Mixer * mix = calloc(1, sizeof(Mixer));

  mix->module = vtable;
  mix->module.name = name;

  // set all control values
  SET_CONTROL_CURR_VOL(mix, DEFAULT_CONTROL_VOL);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(mix);

  return (Module*)mix;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_mixer(void * modPtr)
{
  Mixer * mix = (Mixer *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(mix);
}

static void updateState(void * modPtr)
{
  Mixer * mix = (Mixer *)modPtr;

  R4 * sum = OUT_PORT_SUM(mix);
  for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
  {
    sum[i] = 0;

    sum[i] += IN_PORT_AUDIO0(mix) ? IN_PORT_AUDIO0(mix)[i] : 0;
    sum[i] += IN_PORT_AUDIO1(mix) ? IN_PORT_AUDIO1(mix)[i] : 0;
    sum[i] += IN_PORT_AUDIO2(mix) ? IN_PORT_AUDIO2(mix)[i] : 0;
    sum[i] += IN_PORT_AUDIO3(mix) ? IN_PORT_AUDIO3(mix)[i] : 0;

    R4 rawVolts = 0;
    rawVolts += IN_PORT_VOL(mix) ? IN_PORT_VOL(mix)[i] : 0;
    rawVolts += INTERP(GET_CONTROL_PREV_VOL(mix), GET_CONTROL_CURR_VOL(mix), MODULE_BUFFER_SIZE, i);
    sum[i] *= VoltUtils_voltDbToAmpl(rawVolts);

  }
}

static void pushCurrToPrev(void * modPtr)
{
  Mixer * mix = (Mixer *)modPtr;
  memcpy(mix->outputPortsPrev, mix->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * MIXER_OUTCOUNT);
  memcpy(mix->outputMIDIPortsPrev, mix->outputMIDIPortsCurr, sizeof(MIDIData) * MIXER_MIDI_OUTCOUNT);
  CONTROL_PUSH_TO_PREV(mix);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port >= MIXER_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
  if (port >= MIXER_INCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < MIXER_INCOUNT) return ModulePortType_VoltStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < MIXER_OUTCOUNT) return ModulePortType_VoltStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < MIXER_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}


static U4 getInCount(void * modPtr)
{
  return MIXER_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return MIXER_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return MIXER_CONTROLCOUNT;
}

static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
  if (id >= MIXER_CONTROLCOUNT) return;

  Mixer * vco = (Mixer *)modPtr;
  memcpy(&vco->controlsCurr[id], val, sizeof(Volt));
}

static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id >= MIXER_CONTROLCOUNT) return;

  Mixer * vco = (Mixer *)modPtr;
  *((Volt*)ret) = vco->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
  if (port >= MIXER_INCOUNT) return;

  Mixer * mix = (Mixer *)modPtr;
  mix->inputPorts[port] = readAddr;
}
