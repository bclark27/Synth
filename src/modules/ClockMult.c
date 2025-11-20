#include "ClockMult.h"

//////////////
// DEFINES  //
//////////////

#define IN_PORT_ADDR(mod, port)           (((ClockMult*)(mod))->inputPorts[port]);

#define PREV_PORT_ADDR(mod, port)         (((ClockMult*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)         (((ClockMult*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_PORT_CLKIN(clkMult)            ((clkMult)->inputPorts[CLKMULT_IN_PORT_CLKIN])

#define OUT_PORT_FULL(clkMult)            (CURR_PORT_ADDR(clkMult, CLKMULT_OUT_PORT_FULL))
#define OUT_PORT_HALF(clkMult)            (CURR_PORT_ADDR(clkMult, CLKMULT_OUT_PORT_HALF))
#define OUT_PORT_QURT(clkMult)            (CURR_PORT_ADDR(clkMult, CLKMULT_OUT_PORT_QURT))
#define OUT_PORT_EGHT(clkMult)            (CURR_PORT_ADDR(clkMult, CLKMULT_OUT_PORT_EGHT))
#define OUT_PORT_SIXT(clkMult)            (CURR_PORT_ADDR(clkMult, CLKMULT_OUT_PORT_SIXT))

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_clkMult(void * modPtr);
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

static R4 divideHelper(ClockMult * clkMult, ModularPortID id);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static char * inPortNames[CLKMULT_INCOUNT] = {
  "Clock",
};

static char * outPortNames[CLKMULT_OUTCOUNT] = {
  "Whole",
  "Half",
  "Quarter",
  "Eighth",
  "Sixtenth",
};

static char * controlNames[CLKMULT_CONTROLCOUNT] = {
};

static Module vtable = {
  .type = ModuleType_ClockMult,
  .freeModule = free_clkMult,
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

static const U4 halfsMode[CLKMULT_OUTCOUNT] = {1, 2, 4, 8, 16};

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * ClockMult_init(char* name)
{
  ClockMult * clkMult = calloc(1, sizeof(ClockMult));

  // set vtable
  clkMult->module = vtable;
  clkMult->module.name = name;

  // set priv vars
  clkMult->currCount = 0;
  clkMult->lastHighToHighCount = SAMPLE_RATE;

  clkMult->inClkIsHigh = 0;

  memcpy(clkMult->divisors, halfsMode, CLKMULT_OUTCOUNT * sizeof(U4));

  return (Module*)clkMult;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////


static void free_clkMult(void * modPtr)
{
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(modPtr);
}

static void updateState(void * modPtr)
{
  ClockMult * clkMult = (ClockMult *)modPtr;

  if (!IN_PORT_CLKIN(clkMult)) return;

  for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
  {
    if (!clkMult->inClkIsHigh && IN_PORT_CLKIN(clkMult)[i] > VOLTSTD_GATE_HIGH_THRESH)
    {
      clkMult->lastHighToHighCount = clkMult->currCount;
      clkMult->currCount = 0;
      clkMult->inClkIsHigh = 1;

      memset(clkMult->dividedCounts, 0, sizeof(U4) * CLKMULT_OUTCOUNT);
    }
    else
    {
      clkMult->inClkIsHigh = 0;
    }

    // if (clkMult->currCount == 0) { OUT_PORT_FULL(clkMult)[i] = VOLTSTD_GATE_HIGH; }
    // else { OUT_PORT_FULL(clkMult)[i] = VOLTSTD_GATE_LOW; }

    OUT_PORT_FULL(clkMult)[i] = divideHelper(clkMult, CLKMULT_OUT_PORT_FULL);
    OUT_PORT_HALF(clkMult)[i] = divideHelper(clkMult, CLKMULT_OUT_PORT_HALF);
    OUT_PORT_QURT(clkMult)[i] = divideHelper(clkMult, CLKMULT_OUT_PORT_QURT);
    OUT_PORT_EGHT(clkMult)[i] = divideHelper(clkMult, CLKMULT_OUT_PORT_EGHT);
    OUT_PORT_SIXT(clkMult)[i] = divideHelper(clkMult, CLKMULT_OUT_PORT_SIXT);

    clkMult->currCount++;
  }
}


static void pushCurrToPrev(void * modPtr)
{
  ClockMult * clkMult = (ClockMult *)modPtr;
  memcpy(clkMult->outputPortsPrev, clkMult->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * CLKMULT_OUTCOUNT);
  memcpy(clkMult->outputMIDIPortsPrev, clkMult->outputMIDIPortsCurr, sizeof(MIDIData) * CLKMULT_MIDI_OUTCOUNT);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port >= CLKMULT_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
  if (port >= CLKMULT_INCOUNT) return NULL;

  return IN_PORT_ADDR(modPtr, port);
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < CLKMULT_INCOUNT) return ModulePortType_VoltStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < CLKMULT_OUTCOUNT) return ModulePortType_VoltStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < CLKMULT_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}


static U4 getInCount(void * modPtr)
{
  return CLKMULT_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return CLKMULT_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return CLKMULT_CONTROLCOUNT;
}

static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
  if (id >= CLKMULT_CONTROLCOUNT) return;

  ClockMult * vco = (ClockMult *)modPtr;
  memcpy(&vco->controlsCurr[id], val, sizeof(Volt));
}

static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id >= CLKMULT_CONTROLCOUNT) return;

  ClockMult * vco = (ClockMult *)modPtr;
  *((Volt*)ret) = vco->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
  if (port >= CLKMULT_INCOUNT) return;

  ClockMult * clk = (ClockMult *)modPtr;
  clk->inputPorts[port] = readAddr;
}


static R4 divideHelper(ClockMult * clkMult, ModularPortID id)
{
  if (clkMult->currCount % (clkMult->lastHighToHighCount / clkMult->divisors[id]) == 0 &&
        clkMult->dividedCounts[id] < clkMult->divisors[id])
  {
    clkMult->dividedCounts[id]++;
    return VOLTSTD_GATE_HIGH;
  }
  else
  {
    return VOLTSTD_GATE_LOW;
  }
}
