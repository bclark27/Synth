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

#define CONTROL_PUSH_TO_PREV(clk)         for (U4 i = 0; i < CLOCK_CONTROLCOUNT; i++) {(clk)->controlsPrev[i] = (clk)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_clock(void * modPtr);
static void updateState(void * modPtr);
static void pushCurrToPrev(void * modPtr);
static R4 * getOutputAddr(void * modPtr, U4 port);
static R4 * getInputAddr(void * modPtr, U4 port);
static U4 getInCount(void * modPtr);
static U4 getOutCount(void * modPtr);
static U4 getControlCount(void * modPtr);
static void setControlVal(void * modPtr, U4 id, R4 val);
static R4 getControlVal(void * modPtr, U4 id);
static void linkToInput(void * modPtr, U4 port, R4 * readAddr);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static Module vtable = {
  .freeModule = free_clock,
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

#define DEFAULT_CONTROL_RATE    1

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * Clock_init()
{
  Clock * clk = calloc(1, sizeof(Clock));

  // set module
  clk->module = vtable;

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
  free(clk);
}

static void updateState(void * modPtr)
{
  Clock * clk = (Clock*)modPtr;

  for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
  {
    // interp the control rate
    R4 thisRate = INTERP(GET_CONTROL_PREV_CLOCK(clk), GET_CONTROL_CURR_CLOCK(clk), MODULE_BUFFER_SIZE, i);

    // calculate the step
    R4 step = thisRate / SAMPLE_RATE;
    clk->phase += step;

    if (clk->phase >= 1)
    {
      clk->phase = 0;
      CURR_PORT_ADDR(clk, CLOCK_OUT_PORT_CLOCK)[i] = VOLTSTD_GATE_HIGH;
    }
    else
    {
      CURR_PORT_ADDR(clk, CLOCK_OUT_PORT_CLOCK)[i] = VOLTSTD_GATE_LOW;
    }
  }

  // push curr to prev
  CONTROL_PUSH_TO_PREV(clk);
}

static void pushCurrToPrev(void * modPtr)
{
  Clock * clk = (Clock *)modPtr;
  memcpy(clk->outputPortsPrev, clk->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * CLOCK_OUTCOUNT);
}

static R4 * getOutputAddr(void * modPtr, U4 port)
{
  if (port >= CLOCK_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static R4 * getInputAddr(void * modPtr, U4 port)
{
  if (port >= CLOCK_INCOUNT) return NULL;

  return IN_PORT_ADDR(modPtr, port);
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

static void setControlVal(void * modPtr, U4 id, R4 val)
{
  if (id >= CLOCK_CONTROLCOUNT) return;

  Clock * clk = (Clock *)modPtr;
  clk->controlsCurr[id] = val;
}

static R4 getControlVal(void * modPtr, U4 id)
{
  if (id >= CLOCK_CONTROLCOUNT) return 0;

  Clock * clk = (Clock *)modPtr;
  return clk->controlsCurr[id];
}

static void linkToInput(void * modPtr, U4 port, R4 * readAddr)
{
  if (port >= CLOCK_INCOUNT) return;

  Clock * clk = (Clock *)modPtr;
  clk->inputPorts[port] = readAddr;
}
