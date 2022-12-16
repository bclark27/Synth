#include "VCO.h"

#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define IN_PORT_ADDR(mod, port)           (((VCO*)(mod))->inputPorts[port]);

#define PREV_PORT_ADDR(mod, port)         (((VCO*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)         (((VCO*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_PORT_FREQ(vco)                 ((vco)->inputPorts[VCO_IN_PORT_FREQ])
#define IN_PORT_PW(vco)                   ((vco)->inputPorts[VCO_IN_PORT_PW])

#define OUT_PORT_SIN(vco)                 (CURR_PORT_ADDR(vco, VCO_OUT_PORT_SIN))
#define OUT_PORT_SAW(vco)                 (CURR_PORT_ADDR(vco, VCO_OUT_PORT_SAW))
#define OUT_PORT_SQR(vco)                 (CURR_PORT_ADDR(vco, VCO_OUT_PORT_SQR))
#define OUT_PORT_TRI(vco)                 (CURR_PORT_ADDR(vco, VCO_OUT_PORT_TRI))

#define GET_CONTROL_CURR_FREQ(vco)        ((vco)->controlsCurr[VCO_CONTROL_FREQ])
#define GET_CONTROL_PREV_FREQ(vco)        ((vco)->controlsPrev[VCO_CONTROL_FREQ])
#define GET_CONTROL_CURR_PW(vco)          ((vco)->controlsCurr[VCO_CONTROL_PW])
#define GET_CONTROL_PREV_PW(vco)          ((vco)->controlsPrev[VCO_CONTROL_PW])

#define SET_CONTROL_CURR_FREQ(vco, freq)  ((vco)->controlsCurr[VCO_CONTROL_FREQ] = (freq))
#define SET_CONTROL_PREV_FREQ(vco, freq)  ((vco)->controlsPrev[VCO_CONTROL_FREQ] = (freq))
#define SET_CONTROL_CURR_PW(vco, pw)      ((vco)->controlsCurr[VCO_CONTROL_PW] = (pw))
#define SET_CONTROL_PREV_PW(vco, pw)      ((vco)->controlsPrev[VCO_CONTROL_PW] = (pw))

#define CONTROL_PUSH_TO_PREV(vco)         for (U4 i = 0; i < VCO_CONTROLCOUNT; i++) {(vco)->controlsPrev[i] = (vco)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_vco(void * modPtr);
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

static void createStrideTable(VCO * vco, R4 * table);
static void createPwTable(VCO * vco, R4 * table);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static Module vtable = {
  .freeModule = free_vco,
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

#define DEFAULT_CONTROL_FREQ    0
#define DEFAULT_CONTROL_PW      0.5f

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * VCO_init()
{
  VCO * vco = calloc(1, sizeof(VCO));

  // set vtable
  vco->module = vtable;

  // set all control values
  SET_CONTROL_CURR_FREQ(vco, DEFAULT_CONTROL_FREQ);
  SET_CONTROL_CURR_PW(vco, DEFAULT_CONTROL_PW);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(vco);

  // init other structs
  Oscillator_initInPlace(&vco->oscSin, GET_CONTROL_CURR_FREQ(vco), Waveform_sin);
  Oscillator_initInPlace(&vco->oscSqr, GET_CONTROL_CURR_FREQ(vco), Waveform_sqr);
  Oscillator_initInPlace(&vco->oscSaw, GET_CONTROL_CURR_FREQ(vco), Waveform_saw);
  Oscillator_initInPlace(&vco->oscTri, GET_CONTROL_CURR_FREQ(vco), Waveform_tri);

  return (Module*)vco;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_vco(void * modPtr)
{
  VCO * vco = (VCO *)modPtr;
  free(vco);
}

static void updateState(void * modPtr)
{
  VCO * vco = (VCO *)modPtr;

  R4 strideTable[MODULE_BUFFER_SIZE];
  R4 pwTable[MODULE_BUFFER_SIZE];

  createStrideTable(vco, strideTable);
  createPwTable(vco, pwTable);

  Oscillator_sampleWithStrideAndPWTable(&vco->oscSin, OUT_PORT_SIN(vco), MODULE_BUFFER_SIZE, strideTable, pwTable);
  Oscillator_sampleWithStrideAndPWTable(&vco->oscSaw, OUT_PORT_SAW(vco), MODULE_BUFFER_SIZE, strideTable, pwTable);
  Oscillator_sampleWithStrideAndPWTable(&vco->oscSqr, OUT_PORT_SQR(vco), MODULE_BUFFER_SIZE, strideTable, pwTable);
  Oscillator_sampleWithStrideAndPWTable(&vco->oscTri, OUT_PORT_TRI(vco), MODULE_BUFFER_SIZE, strideTable, pwTable);


  // scale osc output to min and max volt range
  for (U4 i = 0; i < MODULE_BUFFER_SIZE * VCO_OUTCOUNT; i++)
  {
    vco->outputPortsCurr[i] = (VOLTSTD_AUD_RANGE * vco->outputPortsCurr[i]) - VOLTSTD_AUD_MAX;
  }

  // push curr to prev
  CONTROL_PUSH_TO_PREV(vco);
}

static void pushCurrToPrev(void * modPtr)
{
  VCO * vco = (VCO *)modPtr;
  memcpy(vco->outputPortsPrev, vco->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * VCO_OUTCOUNT);
}

static R4 * getOutputAddr(void * modPtr, U4 port)
{
  if (port >= VCO_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static R4 * getInputAddr(void * modPtr, U4 port)
{
  if (port >= VCO_INCOUNT) return NULL;

  return IN_PORT_ADDR(modPtr, port);
}

static U4 getInCount(void * modPtr)
{
  return VCO_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return VCO_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return VCO_CONTROLCOUNT;
}

static void setControlVal(void * modPtr, U4 id, R4 val)
{
  if (id >= VCO_CONTROLCOUNT) return;

  VCO * vco = (VCO *)modPtr;
  vco->controlsCurr[id] = val;
}

static R4 getControlVal(void * modPtr, U4 id)
{
  if (id >= VCO_CONTROLCOUNT) return 0;

  VCO * vco = (VCO *)modPtr;
  return vco->controlsCurr[id];
}

static void linkToInput(void * modPtr, U4 port, R4 * readAddr)
{
  if (port >= VCO_INCOUNT) return;

  VCO * vco = (VCO *)modPtr;
  vco->inputPorts[port] = readAddr;
}

static void createStrideTable(VCO * vco, R4 * table)
{
  if (IN_PORT_FREQ(vco))
  {
    for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
      R4 freqControlVolts = INTERP(GET_CONTROL_PREV_FREQ(vco), GET_CONTROL_CURR_FREQ(vco), MODULE_BUFFER_SIZE, i);
      R4 totalFreqVolts = IN_PORT_FREQ(vco)[i] + freqControlVolts;
      table[i] = VoltUtils_voltToFreq(totalFreqVolts);
      table[i] /= SAMPLE_RATE;
    }
  }
  else
  {
    for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
      R4 freqControlVolts = INTERP(GET_CONTROL_PREV_FREQ(vco), GET_CONTROL_CURR_FREQ(vco), MODULE_BUFFER_SIZE, i);
      table[i] = VoltUtils_voltToFreq(freqControlVolts);
      table[i] /= SAMPLE_RATE;
    }
  }
}

static void createPwTable(VCO * vco, R4 * table)
{
  if (IN_PORT_PW(vco))
  {
    for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
      R4 totalPwVolts = IN_PORT_PW(vco)[i] / VOLTSTD_MOD_CV_MAX + INTERP(GET_CONTROL_PREV_PW(vco), GET_CONTROL_CURR_PW(vco), MODULE_BUFFER_SIZE, i);
      table[i] = MAX(MIN(totalPwVolts, 0.99f), 0.01f);
    }
  }
  else
  {
    for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
      R4 totalPwVolts = INTERP(GET_CONTROL_PREV_PW(vco), GET_CONTROL_CURR_PW(vco), MODULE_BUFFER_SIZE, i);
      table[i] = MAX(MIN(totalPwVolts, 0.99f), 0.01f);
    }
  }
}
