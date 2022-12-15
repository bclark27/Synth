#include "OutputModule.h"

#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define IN_PORT_ADDR(mod, port)           (((OutputModule*)(mod))->inputPorts[port]);

#define PREV_PORT_ADDR(out, port)         (((OutputModule*)(out))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(out, port)         (((OutputModule*)(out))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_PORT_LEFT(out)                 ((out)->inputPorts[OUTPUT_IN_PORT_LEFT])
#define IN_PORT_RIGHT(out)                ((out)->inputPorts[OUTPUT_IN_PORT_RIGHT])

#define OUT_PORT_LEFT(out)                (CURR_PORT_ADDR(out, OUTPUT_OUT_PORT_LEFT))
#define OUT_PORT_RIGHT(out)               (CURR_PORT_ADDR(out, OUTPUT_OUT_PORT_RIGHT))

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_outputModule(void * modPtr);
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
  .freeModule = free_outputModule,
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


//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * OutputModule_init(void)
{
  OutputModule * out = calloc(1, sizeof(OutputModule));

  out->module = vtable;

  return (Module*) out;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_outputModule(void * modPtr)
{
  OutputModule * out = (OutputModule *)modPtr;
  free(out);
}

static void updateState(void * modPtr)
{
  OutputModule * out = (OutputModule *)modPtr;

  R4 * leftIn = IN_PORT_LEFT(out);
  R4 * rightIn = IN_PORT_RIGHT(out);

  R4 * leftOut = OUT_PORT_LEFT(out);
  R4 * rightOut = OUT_PORT_RIGHT(out);

  // split into ugly if and for loop for faster execution

  if (leftIn)
  {
    for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
      leftOut[i] = MAX(MIN(leftIn[i], VOLTSTD_AUD_MAX), VOLTSTD_AUD_MIN) / VOLTSTD_AUD_MAX;
    }
  }
  else
  {
    memset(leftOut, MODULE_BUFFER_SIZE, sizeof(R4));
  }

  if (rightIn)
  {
    for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
      rightOut[i] = MAX(MIN(rightIn[i], VOLTSTD_AUD_MAX), VOLTSTD_AUD_MIN) / VOLTSTD_AUD_MAX;
    }
  }
  else
  {
    memset(rightOut, MODULE_BUFFER_SIZE, sizeof(R4));
  }
}

static void pushCurrToPrev(void * modPtr)
{
  OutputModule * out = (OutputModule *)modPtr;
  memcpy(out->outputPortsPrev, out->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * OUTPUT_OUTCOUNT);
}

static R4 * getOutputAddr(void * modPtr, U4 port)
{
  if (port >= OUTPUT_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static R4 * getInputAddr(void * modPtr, U4 port)
{
  if (port >= OUTPUT_INCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static U4 getInCount(void * modPtr)
{
  return OUTPUT_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return OUTPUT_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return OUTPUT_CONTROLCOUNT;
}

static void setControlVal(void * modPtr, U4 id, R4 val)
{
  if (id >= OUTPUT_CONTROLCOUNT) return;

  OutputModule * out = (OutputModule *)modPtr;
  out->controlsCurr[id] = val;
}

static R4 getControlVal(void * modPtr, U4 id)
{
  if (id >= OUTPUT_CONTROLCOUNT) return 0;

  OutputModule * out = (OutputModule *)modPtr;
  return out->controlsCurr[id];
}

static void linkToInput(void * modPtr, U4 port, R4 * readAddr)
{
  if (port >= OUTPUT_INCOUNT) return;

  OutputModule * out = (OutputModule *)modPtr;
  out->inputPorts[port] = readAddr;
}
