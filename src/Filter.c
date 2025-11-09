#include "Filter.h"

#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define IN_PORT_ADDR(mod, port)           (((Filter*)(mod))->inputPorts[port]);

#define PREV_PORT_ADDR(mod, port)         (((Filter*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)         (((Filter*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_PORT_AUD(flt)                 ((flt)->inputPorts[FITLER_IN_PORT_AUD])
#define IN_PORT_FREQ(flt)                 ((flt)->inputPorts[FITLER_IN_PORT_FREQ])
#define IN_PORT_Q(flt)                 ((flt)->inputPorts[FITLER_IN_PORT_Q])

#define OUT_PORT_AUD(flt)                (CURR_PORT_ADDR(flt, FITLER_OUT_PORT_AUD))

#define GET_CONTROL_CURR_FREQ(flt)        ((flt)->controlsCurr[FITLER_CONTROL_FREQ])
#define GET_CONTROL_PREV_FREQ(flt)        ((flt)->controlsPrev[FITLER_CONTROL_FREQ])
#define GET_CONTROL_CURR_Q(flt)        ((flt)->controlsCurr[FITLER_CONTROL_Q])
#define GET_CONTROL_PREV_Q(flt)        ((flt)->controlsPrev[FITLER_CONTROL_Q])
#define GET_CONTROL_CURR_DB(flt)        ((flt)->controlsCurr[FITLER_CONTROL_DB])
#define GET_CONTROL_PREV_DB(flt)        ((flt)->controlsPrev[FITLER_CONTROL_DB])

#define SET_CONTROL_CURR_FREQ(flt, v)     ((flt)->controlsCurr[FITLER_CONTROL_FREQ] = (v))
#define SET_CONTROL_PREV_FREQ(flt, v)     ((flt)->controlsPrev[FITLER_CONTROL_FREQ] = (v))
#define SET_CONTROL_CURR_Q(flt, v)     ((flt)->controlsCurr[FITLER_CONTROL_Q] = (v))
#define SET_CONTROL_PREV_Q(flt, v)     ((flt)->controlsPrev[FITLER_CONTROL_Q] = (v))
#define SET_CONTROL_CURR_DB(flt, v)     ((flt)->controlsCurr[FITLER_CONTROL_DB] = (v))
#define SET_CONTROL_PREV_DB(flt, v)     ((flt)->controlsPrev[FITLER_CONTROL_DB] = (v))

#define CONTROL_PUSH_TO_PREV(vco)         for (U4 i = 0; i < FITLER_CONTROLCOUNT; i++) {(vco)->controlsPrev[i] = (vco)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static char * inPortNames[FITLER_INCOUNT] = {
  "Audio",
  "Freq",
  "Q",
};

static char * outPortNames[FITLER_OUTCOUNT] = {
  "Audio",
};

static char * controlNames[FITLER_CONTROLCOUNT] = {
  "Freq",
  "Q",
  "DB",
};

static void free_attn(void * modPtr);
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

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static Module vtable = {
  .type = ModuleType_Filter,
  .freeModule = free_attn,
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
  .inPortNames = inPortNames,
  .inPortNamesCount = ARRAY_LEN(inPortNames),
  .outPortNames = outPortNames,
  .outPortNamesCount = ARRAY_LEN(outPortNames),
  .controlNames = controlNames,
  .controlNamesCount = ARRAY_LEN(controlNames),
};

#define DEFAULT_CONTROL_FREQ   0
#define DEFAULT_CONTROL_Q   0
#define DEFAULT_CONTROL_DB   0

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * Filter_init(char* name)
{
  Filter * flt = calloc(1, sizeof(Filter));

  // set vtable
  flt->module = vtable;
  flt->module.name = name;

  //set controls
  SET_CONTROL_CURR_FREQ(flt, DEFAULT_CONTROL_FREQ);
  SET_CONTROL_CURR_Q(flt, DEFAULT_CONTROL_Q);
  SET_CONTROL_CURR_DB(flt, DEFAULT_CONTROL_DB);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(flt);

  return (Module*)flt;
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
  Filter * flt = (Filter *)modPtr;

  if (!IN_PORT_AUD(flt))
  {
    memset(OUT_PORT_AUD(flt), 0, sizeof(R4) * MODULE_BUFFER_SIZE);
    return;
  }
}

static void pushCurrToPrev(void * modPtr)
{
  Filter * flt = (Filter *)modPtr;
  memcpy(flt->outputPortsPrev, flt->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * FITLER_OUTCOUNT);
}

static R4 * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port >= FITLER_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static R4 * getInputAddr(void * modPtr, ModularPortID port)
{
  if (port >= FITLER_INCOUNT) return NULL;

  return IN_PORT_ADDR(modPtr, port);
}

static U4 getInCount(void * modPtr)
{
  return FITLER_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return FITLER_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return FITLER_CONTROLCOUNT;
}

static void setControlVal(void * modPtr, ModularPortID id, R4 val)
{
  if (id >= FITLER_CONTROLCOUNT) return;

  Filter * flt = (Filter *)modPtr;
  flt->controlsCurr[id] = val;
}

static R4 getControlVal(void * modPtr, ModularPortID id)
{
  if (id >= FITLER_CONTROLCOUNT) return 0;

  Filter * flt = (Filter *)modPtr;
  return flt->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, R4 * readAddr)
{
  if (port >= FITLER_INCOUNT) return;

  Filter * flt = (Filter *)modPtr;
  flt->inputPorts[port] = readAddr;
}
