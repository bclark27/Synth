#include "ClockMult.h"

#include "VoltUtils.h"

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
static R4 * getOutputAddr(void * modPtr, ModularPortID port);
static R4 * getInputAddr(void * modPtr, ModularPortID port);
static U4 getInCount(void * modPtr);
static U4 getOutCount(void * modPtr);
static U4 getControlCount(void * modPtr);
static void setControlVal(void * modPtr, ModularPortID id, R4 val);
static R4 getControlVal(void * modPtr, ModularPortID id);
static void linkToInput(void * modPtr, ModularPortID port, R4 * readAddr);

static R4 divideHelper(ClockMult * clkMult, ModularPortID id);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static Module vtable = {
  .freeModule = free_clkMult,
  .updateState = updateState,
  .pushCurrToPrev = pushCurrToPrev,
  .getOutputAddr = getOutputAddr,
  .getInputAddr = getInputAddr,
  .getInCount = getInCount,
  .getOutCount = getOutCount,
  .getContolCount = getControlCount,
  .setControlVal = setControlVal,
  .getControlVal = getControlVal,
  .linkToInput = linkToInput,
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
}

static R4 * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port >= CLKMULT_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static R4 * getInputAddr(void * modPtr, ModularPortID port)
{
  if (port >= CLKMULT_INCOUNT) return NULL;

  return IN_PORT_ADDR(modPtr, port);
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

static void setControlVal(void * modPtr, ModularPortID id, R4 val)
{
  if (id >= CLKMULT_CONTROLCOUNT) return;

  ClockMult * clkMult = (ClockMult *)modPtr;
  clkMult->controlsCurr[id] = val;
}

static R4 getControlVal(void * modPtr, ModularPortID id)
{
  if (id >= CLKMULT_CONTROLCOUNT) return 0;

  ClockMult * clkMult = (ClockMult *)modPtr;
  return clkMult->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, R4 * readAddr)
{
  if (port >= CLKMULT_INCOUNT) return;

  ClockMult * clkMult = (ClockMult *)modPtr;
  clkMult->inputPorts[port] = readAddr;
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
