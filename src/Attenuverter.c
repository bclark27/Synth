#include "Attenuverter.h"

#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define IN_PORT_ADDR(mod, port)           (((Attenuverter*)(mod))->inputPorts[port]);

#define PREV_PORT_ADDR(mod, port)         (((Attenuverter*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)         (((Attenuverter*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_PORT_ATTN(attn)                 ((attn)->inputPorts[ATTN_IN_PORT_ATTN])
#define IN_PORT_SIG(attn)                 ((attn)->inputPorts[ATTN_IN_PORT_SIG])

#define OUT_PORT_SIG(attn)                (CURR_PORT_ADDR(attn, ATTN_OUT_PORT_SIG))

#define GET_CONTROL_CURR_ATTN(attn)        ((attn)->controlsCurr[ATTN_CONTROL_ATTN])
#define GET_CONTROL_PREV_ATTN(attn)        ((attn)->controlsPrev[ATTN_CONTROL_ATTN])

#define SET_CONTROL_CURR_ATTN(attn, v)     ((attn)->controlsCurr[ATTN_CONTROL_ATTN] = (v))
#define SET_CONTROL_PREV_ATTN(attn, v)     ((attn)->controlsPrev[ATTN_CONTROL_ATTN] = (v))

#define CONTROL_PUSH_TO_PREV(vco)         for (U4 i = 0; i < ATTN_CONTROLCOUNT; i++) {(vco)->controlsPrev[i] = (vco)->controlsCurr[i];} for (U4 i = 0; i < ATTN_MIDI_CONTROLCOUNT; i++) {(vco)->midiControlsPrev[i] = (vco)->midiControlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static char * inPortNames[ATTN_INCOUNT] = {
  "Attn",
  "Sig",
};

static char * outPortNames[ATTN_OUTCOUNT] = {
  "Sig",
};

static char * controlNames[ATTN_CONTROLCOUNT] = {
  "Attn",
};

static void free_attn(void * modPtr);
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

static Module vtable = {
  .type = ModuleType_Attenuverter,
  .freeModule = free_attn,
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

#define DEFAULT_CONTROL_ATTN   VOLTSTD_MOD_CV_MAX

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * Attenuverter_init(char* name)
{
  Attenuverter * attn = calloc(1, sizeof(Attenuverter));

  // set vtable
  attn->module = vtable;
  attn->module.name = name;

  //set controls
  SET_CONTROL_CURR_ATTN(attn, DEFAULT_CONTROL_ATTN);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(attn);

  return (Module*)attn;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_attn(void * modPtr)
{
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(modPtr);
}

static void updateState(void * modPtr)
{
  Attenuverter * attn = (Attenuverter *)modPtr;

  if (!IN_PORT_SIG(attn))
  {
    memset(OUT_PORT_SIG(attn), 0, sizeof(R4) * MODULE_BUFFER_SIZE);
    return;
  }

  for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
  {

    // get the input voltage
    R4 inVolts = CLAMP(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, IN_PORT_ATTN(attn) ? IN_PORT_ATTN(attn)[i] : VOLTSTD_MOD_CV_MAX); // [-10v, +10v]

    // interp the control input
    R4 controlVolts = CLAMP(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, INTERP(GET_CONTROL_PREV_ATTN(attn), GET_CONTROL_CURR_ATTN(attn), MODULE_BUFFER_SIZE, i)); //[-10v, +10v]
    R4 controlAsMultiplier = MAP(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, -1.f, 1.f, controlVolts); // [-1, 1]

    // convert voltage into attn multiplier
    R4 attnMult = VoltUtils_voltDbToAttenuverterMult(inVolts * controlAsMultiplier);

    // out = mult * inputSig
    OUT_PORT_SIG(attn)[i] = IN_PORT_SIG(attn)[i] * attnMult;
  }
}

static void pushCurrToPrev(void * modPtr)
{
  Attenuverter * attn = (Attenuverter *)modPtr;
  memcpy(attn->outputPortsPrev, attn->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * ATTN_OUTCOUNT);
  memcpy(attn->outputMIDIPortsPrev, attn->outputMIDIPortsCurr, sizeof(MIDIData) * ATTN_MIDI_OUTCOUNT);
  CONTROL_PUSH_TO_PREV(attn);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port >= ATTN_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
  if (port >= ATTN_INCOUNT) return NULL;

  return IN_PORT_ADDR(modPtr, port);
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < ATTN_INCOUNT) return ModulePortType_VoltStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < ATTN_OUTCOUNT) return ModulePortType_VoltStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < ATTN_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}


static U4 getInCount(void * modPtr)
{
  return ATTN_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return ATTN_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return ATTN_CONTROLCOUNT;
}


static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
  if (id >= ATTN_CONTROLCOUNT) return;

  Attenuverter * vco = (Attenuverter *)modPtr;
  memcpy(&vco->controlsCurr[id], val, sizeof(Volt));
}

static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id >= ATTN_CONTROLCOUNT) return;

  Attenuverter * vco = (Attenuverter *)modPtr;
  *((Volt*)ret) = vco->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
  if (port >= ATTN_INCOUNT) return;

  Attenuverter * attn = (Attenuverter *)modPtr;
  attn->inputPorts[port] = readAddr;
}
