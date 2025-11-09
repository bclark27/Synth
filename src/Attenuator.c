#include "Attenuator.h"

#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define IN_PORT_ADDR(mod, port)           (((Attenuator*)(mod))->inputPorts[port]);

#define PREV_PORT_ADDR(mod, port)         (((Attenuator*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)         (((Attenuator*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_PORT_VOL(attn)                 ((attn)->inputPorts[ATTN_IN_PORT_VOL])
#define IN_PORT_AUD(attn)                 ((attn)->inputPorts[ATTN_IN_PORT_AUD])

#define OUT_PORT_AUD(attn)                (CURR_PORT_ADDR(attn, ATTN_OUT_PORT_AUD))

#define GET_CONTROL_CURR_VOL(attn)        ((attn)->controlsCurr[ATTN_CONTROL_VOL])
#define GET_CONTROL_PREV_VOL(attn)        ((attn)->controlsPrev[ATTN_CONTROL_VOL])

#define SET_CONTROL_CURR_VOL(attn, v)     ((attn)->controlsCurr[ATTN_CONTROL_VOL] = (v))
#define SET_CONTROL_PREV_VOL(attn, v)     ((attn)->controlsPrev[ATTN_CONTROL_VOL] = (v))

#define CONTROL_PUSH_TO_PREV(vco)         for (U4 i = 0; i < ATTN_CONTROLCOUNT; i++) {(vco)->controlsPrev[i] = (vco)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static char * inPortNames[ATTN_INCOUNT] = {
  "Vol",
  "Audio",
};

static char * outPortNames[ATTN_OUTCOUNT] = {
  "Audio",
};

static char * controlNames[ATTN_CONTROLCOUNT] = {
  "Vol",
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
  .type = ModuleType_Attenuator,
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

#define DEFAULT_CONTROL_VOL   0//VOLTSTD_MOD_CV_MIN

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * Attenuator_init(char* name)
{
  Attenuator * attn = calloc(1, sizeof(Attenuator));

  // set vtable
  attn->module = vtable;
  attn->module.name = name;

  //set controls
  SET_CONTROL_CURR_VOL(attn, DEFAULT_CONTROL_VOL);

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
  Attenuator * attn = (Attenuator *)modPtr;

  if (!IN_PORT_AUD(attn))
  {
    memset(OUT_PORT_AUD(attn), 0, sizeof(R4) * MODULE_BUFFER_SIZE);
    return;
  }

  for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
  {

    // get the input voltage
    R4 inVolts = IN_PORT_VOL(attn) ? IN_PORT_VOL(attn)[i] : 0;

    // interp the control input
    R4 controlVolts = INTERP(GET_CONTROL_PREV_VOL(attn), GET_CONTROL_CURR_VOL(attn), MODULE_BUFFER_SIZE, i);

    // convert voltage into attn multiplier
    R4 attnMult = VoltUtils_voltDbToAtten(inVolts + controlVolts);

    // out = mult * inputSig
    OUT_PORT_AUD(attn)[i] = IN_PORT_AUD(attn)[i] * attnMult;

  }
}

static void pushCurrToPrev(void * modPtr)
{
  Attenuator * attn = (Attenuator *)modPtr;
  memcpy(attn->outputPortsPrev, attn->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * ATTN_OUTCOUNT);
  CONTROL_PUSH_TO_PREV(attn);
}

static R4 * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port >= ATTN_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static R4 * getInputAddr(void * modPtr, ModularPortID port)
{
  if (port >= ATTN_INCOUNT) return NULL;

  return IN_PORT_ADDR(modPtr, port);
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

static void setControlVal(void * modPtr, ModularPortID id, R4 val)
{
  if (id >= ATTN_CONTROLCOUNT) return;

  Attenuator * attn = (Attenuator *)modPtr;
  attn->controlsCurr[id] = val;
}

static R4 getControlVal(void * modPtr, ModularPortID id)
{
  if (id >= ATTN_CONTROLCOUNT) return 0;

  Attenuator * attn = (Attenuator *)modPtr;
  return attn->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, R4 * readAddr)
{
  if (port >= ATTN_INCOUNT) return;

  Attenuator * attn = (Attenuator *)modPtr;
  attn->inputPorts[port] = readAddr;
}
