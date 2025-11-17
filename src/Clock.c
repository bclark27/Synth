#include "Clock.h"

#include "AudioSettings.h"
#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define IN_PORT_ADDR(mod, port)           (((Clock*)(mod))->inputPorts[port]);

#define PREV_PORT_ADDR(mod, port)         (((Clock*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)         (((Clock*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define OUT_PORT_CLOCK(clk)               (CURR_PORT_ADDR(clk, CLOCK_OUT_PORT_CLOCK))

#define GET_CONTROL_CURR_CLOCK(clk)       ((clk)->controlsCurr[CLOCK_CONTROL_RATE])
#define GET_CONTROL_PREV_CLOCK(clk)       ((clk)->controlsPrev[CLOCK_CONTROL_RATE])

#define SET_CONTROL_CURR_CLOCK(clk, rate) ((clk)->controlsCurr[CLOCK_CONTROL_RATE] = (rate))
#define SET_CONTROL_PREV_CLOCK(clk, rate) ((clk)->controlsPrev[CLOCK_CONTROL_RATE] = (rate))

#define CONTROL_PUSH_TO_PREV(vco)         for (U4 i = 0; i < CLOCK_CONTROLCOUNT; i++) {(vco)->controlsPrev[i] = (vco)->controlsCurr[i];} for (U4 i = 0; i < CLOCK_MIDI_CONTROLCOUNT; i++) {(vco)->midiControlsPrev[i] = (vco)->midiControlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_clock(void * modPtr);
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

static char * inPortNames[CLOCK_INCOUNT] = {
};

static char * outPortNames[CLOCK_OUTCOUNT] = {
  "Clock",
};

static char * controlNames[CLOCK_CONTROLCOUNT] = {
  "Freq",
};

static Module vtable = {
  .type = ModuleType_Clock,
  .freeModule = free_clock,
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

#define DEFAULT_CONTROL_RATE    1

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * Clock_init(char* name)
{
  Clock * clk = calloc(1, sizeof(Clock));

  // set module
  clk->module = vtable;
  clk->module.name = name;

  // set curr control val
  SET_CONTROL_CURR_CLOCK(clk, DEFAULT_CONTROL_RATE);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(clk);

  // set private vars
  clk->phase = 0;

  return (Module*)clk;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_clock(void * modPtr)
{
  Clock * clk = (Clock*)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(clk);
}

static void updateState(void * modPtr)
{
  Clock * clk = (Clock*)modPtr;

  R4 step = GET_CONTROL_CURR_CLOCK(clk) / SAMPLE_RATE;
  for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
  {
    // calculate the step
    clk->phase += step;
    if (clk->phase >= 1)
    {
      clk->phase = 0;
      OUT_PORT_CLOCK(clk)[i] = VOLTSTD_GATE_HIGH;
    }
    else
    {
      OUT_PORT_CLOCK(clk)[i] = VOLTSTD_GATE_LOW;
    }
  }

}

static void pushCurrToPrev(void * modPtr)
{
  Clock * clk = (Clock *)modPtr;
  memcpy(clk->outputPortsPrev, clk->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * CLOCK_OUTCOUNT);
  memcpy(clk->outputMIDIPortsPrev, clk->outputMIDIPortsCurr, sizeof(MIDIData) * CLOCK_MIDI_OUTCOUNT);
  CONTROL_PUSH_TO_PREV(clk);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port >= CLOCK_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
  if (port >= CLOCK_INCOUNT) return NULL;

  return IN_PORT_ADDR(modPtr, port);
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < CLOCK_INCOUNT) return ModulePortType_VoltStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < CLOCK_OUTCOUNT) return ModulePortType_VoltStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < CLOCK_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}


static U4 getInCount(void * modPtr)
{
  return CLOCK_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return CLOCK_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return CLOCK_CONTROLCOUNT;
}


static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
  if (id >= CLOCK_CONTROLCOUNT) return;

  Clock * vco = (Clock *)modPtr;
  memcpy(&vco->controlsCurr[id], val, sizeof(Volt));
}

static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id >= CLOCK_CONTROLCOUNT) return;

  Clock * vco = (Clock *)modPtr;
  *((Volt*)ret) = vco->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
  if (port >= CLOCK_INCOUNT) return;

  Clock * clk = (Clock *)modPtr;
  clk->inputPorts[port] = readAddr;
}
