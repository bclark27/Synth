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

#define OUT_PORT_AUD(vco)                 (CURR_PORT_ADDR(vco, VCO_OUT_PORT_AUD))

#define GET_CONTROL_CURR_FREQ(vco)        ((vco)->controlsCurr[VCO_CONTROL_FREQ])
#define GET_CONTROL_PREV_FREQ(vco)        ((vco)->controlsPrev[VCO_CONTROL_FREQ])
#define GET_CONTROL_CURR_PW(vco)          ((vco)->controlsCurr[VCO_CONTROL_PW])
#define GET_CONTROL_PREV_PW(vco)          ((vco)->controlsPrev[VCO_CONTROL_PW])
#define GET_CONTROL_CURR_WAVE(vco)          ((vco)->controlsCurr[VCO_CONTROL_WAVE])
#define GET_CONTROL_PREV_WAVE(vco)          ((vco)->controlsPrev[VCO_CONTROL_WAVE])
#define GET_CONTROL_CURR_UNI(vco)          ((vco)->controlsCurr[VCO_CONTROL_UNI])
#define GET_CONTROL_PREV_UNI(vco)          ((vco)->controlsPrev[VCO_CONTROL_UNI])
#define GET_CONTROL_CURR_DET(vco)          ((vco)->controlsCurr[VCO_CONTROL_DET])
#define GET_CONTROL_PREV_DET(vco)          ((vco)->controlsPrev[VCO_CONTROL_DET])

#define SET_CONTROL_CURR_FREQ(vco, freq)  ((vco)->controlsCurr[VCO_CONTROL_FREQ] = (freq))
#define SET_CONTROL_PREV_FREQ(vco, freq)  ((vco)->controlsPrev[VCO_CONTROL_FREQ] = (freq))
#define SET_CONTROL_CURR_PW(vco, pw)      ((vco)->controlsCurr[VCO_CONTROL_PW] = (pw))
#define SET_CONTROL_PREV_PW(vco, pw)      ((vco)->controlsPrev[VCO_CONTROL_PW] = (pw))
#define SET_CONTROL_CURR_WAVE(vco, w)      ((vco)->controlsCurr[VCO_CONTROL_WAVE] = (w))
#define SET_CONTROL_PREV_WAVE(vco, w)      ((vco)->controlsPrev[VCO_CONTROL_WAVE] = (w))
#define SET_CONTROL_CURR_UNI(vco, u)      ((vco)->controlsCurr[VCO_CONTROL_UNI] = (u))
#define SET_CONTROL_PREV_UNI(vco, u)      ((vco)->controlsPrev[VCO_CONTROL_UNI] = (u))
#define SET_CONTROL_CURR_DET(vco, d)      ((vco)->controlsCurr[VCO_CONTROL_DET] = (d))
#define SET_CONTROL_PREV_DET(vco, d)      ((vco)->controlsPrev[VCO_CONTROL_DET] = (d))

#define CONTROL_PUSH_TO_PREV(vco)         for (U4 i = 0; i < VCO_CONTROLCOUNT; i++) {(vco)->controlsPrev[i] = (vco)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_vco(void * modPtr);
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

static void createStrideTable(VCO * vco, R4 * table);
static void createPwTable(VCO * vco, R4 * table);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static char * inPortNames[VCO_INCOUNT] = {
  "Freq",
  "PW",
};

static char * outPortNames[VCO_OUTCOUNT] = {
  "Aud",
};

static char * controlNames[VCO_CONTROLCOUNT] = {
  "Freq",
  "PW",
  "Waveform",
  "Unison",
  "Detune",
};

static Module vtable = {
  .type = ModuleType_VCO,
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
  .inPortNames = inPortNames,
  .inPortNamesCount = ARRAY_LEN(inPortNames),
  .outPortNames = outPortNames,
  .outPortNamesCount = ARRAY_LEN(outPortNames),
  .controlNames = controlNames,
  .controlNamesCount = ARRAY_LEN(controlNames),
};

#define DEFAULT_CONTROL_FREQ    0
#define DEFAULT_CONTROL_PW      0.5f
#define DEFAULT_CONTROL_WAVE    0
#define DEFAULT_CONTROL_UNI     1
#define DEFAULT_CONTROL_DET     1.03

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * VCO_init(char* name)
{
  VCO * vco = calloc(1, sizeof(VCO));

  // set vtable
  vco->module = vtable;

  // set name of module
  vco->module.name = name;

  // set all control values
  SET_CONTROL_CURR_FREQ(vco, DEFAULT_CONTROL_FREQ);
  SET_CONTROL_CURR_PW(vco, DEFAULT_CONTROL_PW);
  SET_CONTROL_CURR_WAVE(vco, DEFAULT_CONTROL_WAVE);
  SET_CONTROL_CURR_UNI(vco, DEFAULT_CONTROL_UNI);
  SET_CONTROL_CURR_DET(vco, DEFAULT_CONTROL_DET);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(vco);

  // init other structs
  Oscillator_initInPlace(&vco->osc, Waveform_sin);

  return (Module*)vco;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_vco(void * modPtr)
{
  VCO * vco = (VCO *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(vco);
}

static void updateState(void * modPtr)
{
  VCO * vco = (VCO *)modPtr;

  R4 strideTable[MODULE_BUFFER_SIZE];
  R4 pwTable[MODULE_BUFFER_SIZE];

  createStrideTable(vco, strideTable);
  createPwTable(vco, pwTable);

  // now lets change the wave form based on the control
  Waveform wave = (Waveform)(int)MAX(MIN(GET_CONTROL_PREV_WAVE(vco) * 4, 3), 0);
  vco->osc.waveform = wave;

  U1 unison = MAX(MIN((int)GET_CONTROL_PREV_UNI(vco), MAX_UNISON), 1);
  Oscillator_sampleWithStrideAndPWTable(&vco->osc, OUT_PORT_AUD(vco), MODULE_BUFFER_SIZE, strideTable, pwTable, unison, GET_CONTROL_PREV_DET(vco));

  // push curr to prev
  CONTROL_PUSH_TO_PREV(vco);
}

static void pushCurrToPrev(void * modPtr)
{
  VCO * vco = (VCO *)modPtr;
  memcpy(vco->outputPortsPrev, vco->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * VCO_OUTCOUNT);
}

static R4 * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port >= VCO_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static R4 * getInputAddr(void * modPtr, ModularPortID port)
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

static void setControlVal(void * modPtr, ModularPortID id, R4 val)
{
  if (id >= VCO_CONTROLCOUNT) return;

  VCO * vco = (VCO *)modPtr;
  vco->controlsCurr[id] = val;
}

static R4 getControlVal(void * modPtr, ModularPortID id)
{
  if (id >= VCO_CONTROLCOUNT) return 0;

  VCO * vco = (VCO *)modPtr;
  return vco->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, R4 * readAddr)
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
