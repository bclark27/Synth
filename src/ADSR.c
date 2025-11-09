#include "ADSR.h"

#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define IN_PORT_ADDR(mod, port)           (((ADSR*)(mod))->inputPorts[port]);

#define PREV_PORT_ADDR(mod, port)         (((ADSR*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)         (((ADSR*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_PORT_GATE(adsr)                ((adsr)->inputPorts[ADSR_IN_PORT_GATE])
#define IN_PORT_TRIG(adsr)                ((adsr)->inputPorts[ADSR_IN_PORT_TRIG])
#define IN_PORT_A(adsr)                   ((adsr)->inputPorts[ADSR_IN_PORT_A])
#define IN_PORT_D(adsr)                   ((adsr)->inputPorts[ADSR_IN_PORT_D])
#define IN_PORT_S(adsr)                   ((adsr)->inputPorts[ADSR_IN_PORT_S])
#define IN_PORT_R(adsr)                   ((adsr)->inputPorts[ADSR_IN_PORT_R])

#define OUT_PORT_ENV(adsr)                (CURR_PORT_ADDR(adsr, ADSR_OUT_PORT_ENV))

#define GET_CONTROL_CURR_A(adsr)          ((adsr)->controlsCurr[ADSR_CONTROL_A])
#define GET_CONTROL_PREV_A(adsr)          ((adsr)->controlsPrev[ADSR_CONTROL_A])
#define GET_CONTROL_CURR_D(adsr)          ((adsr)->controlsCurr[ADSR_CONTROL_D])
#define GET_CONTROL_PREV_D(adsr)          ((adsr)->controlsPrev[ADSR_CONTROL_D])
#define GET_CONTROL_CURR_S(adsr)          ((adsr)->controlsCurr[ADSR_CONTROL_S])
#define GET_CONTROL_PREV_S(adsr)          ((adsr)->controlsPrev[ADSR_CONTROL_S])
#define GET_CONTROL_CURR_R(adsr)          ((adsr)->controlsCurr[ADSR_CONTROL_R])
#define GET_CONTROL_PREV_R(adsr)          ((adsr)->controlsPrev[ADSR_CONTROL_R])

#define SET_CONTROL_CURR_A(adsr, val)     ((adsr)->controlsCurr[ADSR_CONTROL_A] = (val))
#define SET_CONTROL_PREV_A(adsr, val)     ((adsr)->controlsPrev[ADSR_CONTROL_A] = (val))
#define SET_CONTROL_CURR_D(adsr, val)     ((adsr)->controlsCurr[ADSR_CONTROL_D] = (val))
#define SET_CONTROL_PREV_D(adsr, val)     ((adsr)->controlsPrev[ADSR_CONTROL_D] = (val))
#define SET_CONTROL_CURR_S(adsr, val)     ((adsr)->controlsCurr[ADSR_CONTROL_S] = (val))
#define SET_CONTROL_PREV_S(adsr, val)     ((adsr)->controlsPrev[ADSR_CONTROL_S] = (val))
#define SET_CONTROL_CURR_R(adsr, val)     ((adsr)->controlsCurr[ADSR_CONTROL_R] = (val))
#define SET_CONTROL_PREV_R(adsr, val)     ((adsr)->controlsPrev[ADSR_CONTROL_R] = (val))

#define CONTROL_PUSH_TO_PREV(vco)         for (U4 i = 0; i < ADSR_CONTROLCOUNT; i++) {(vco)->controlsPrev[i] = (vco)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_adsr(void * modPtr);
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

static void progressEnvelope(ADSR * adsr, R4 currA, R4 currD, R4 currS, R4 currR);
static R4 sampleEnvelope(ADSR * adsr, R4 currA, R4 currD, R4 currS, R4 currR);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static char * inPortNames[ADSR_INCOUNT] = {
  "Gate",
  "Trigger",
  "Attack",
  "Decay",
  "Sustain",
  "Release",
};

static char * outPortNames[ADSR_OUTCOUNT] = {
  "Envelope",
};

static char * controlNames[ADSR_CONTROLCOUNT] = {
  "Attack",
  "Decay",
  "Sustain",
  "Release",
};

static Module vtable = {
  .type = ModuleType_ADSR,
  .freeModule = free_adsr,
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

#define DEFAULT_CONTROL_A   0.01f
#define DEFAULT_CONTROL_D   0.05f
#define DEFAULT_CONTROL_S   0.1f
#define DEFAULT_CONTROL_R   0.3f

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * ADSR_init(char* name)
{
  ADSR * adsr = calloc(1, sizeof(ADSR));

  // set vtable
  adsr->module = vtable;
  adsr->module.name = name;

  //set controls
  SET_CONTROL_CURR_A(adsr, DEFAULT_CONTROL_A);
  SET_CONTROL_CURR_D(adsr, DEFAULT_CONTROL_D);
  SET_CONTROL_CURR_S(adsr, DEFAULT_CONTROL_S);
  SET_CONTROL_CURR_R(adsr, DEFAULT_CONTROL_R);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(adsr);

  return (Module*)adsr;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_adsr(void * modPtr)
{
  ADSR * adsr = (ADSR *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(adsr);
}

static void updateState(void * modPtr)
{
  ADSR * adsr = (ADSR *)modPtr;

  for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
  {
    R4 currA = INTERP(GET_CONTROL_PREV_A(adsr), GET_CONTROL_CURR_A(adsr), MODULE_BUFFER_SIZE, i);
    R4 currD = INTERP(GET_CONTROL_PREV_D(adsr), GET_CONTROL_CURR_D(adsr), MODULE_BUFFER_SIZE, i);
    R4 currS = INTERP(GET_CONTROL_PREV_S(adsr), GET_CONTROL_CURR_S(adsr), MODULE_BUFFER_SIZE, i);
    R4 currR = INTERP(GET_CONTROL_PREV_R(adsr), GET_CONTROL_CURR_R(adsr), MODULE_BUFFER_SIZE, i);

    currA += IN_PORT_A(adsr) ? IN_PORT_A(adsr)[i] : 0;
    currD += IN_PORT_D(adsr) ? IN_PORT_D(adsr)[i] : 0;
    currS += IN_PORT_S(adsr) ? IN_PORT_S(adsr)[i] : 0;
    currR += IN_PORT_R(adsr) ? IN_PORT_R(adsr)[i] : 0;

    currA = MAX(0, currA);
    currD = MAX(0, currD);
    currS = MAX(0, currS);
    currR = MAX(0, currR);

    R4 gateVal = IN_PORT_GATE(adsr) ? IN_PORT_GATE(adsr)[i] : 0;

    bool isNewHigh = adsr->prevSampleValue < VOLTSTD_GATE_HIGH_THRESH &&
                      gateVal > VOLTSTD_GATE_HIGH_THRESH;

    adsr->prevSampleValue = gateVal;

    adsr->isHeld = gateVal > VOLTSTD_GATE_HIGH_THRESH;

    // update envelope state
    // sample from correct equation
    R4 finalVal = 0;
    // printf("%d\n", okok++);

    if (isNewHigh)
    {
      adsr->prevADSRVal = adsr->prevADSRStop;
      adsr->section = ADSR_ASection;
      adsr->timeSinceSectionStart = 0;
      adsr->envelopeActive = 1;
      adsr->releaseStartVal = 0;
    }
    else if (adsr->envelopeActive)
    {
      adsr->timeSinceSectionStart += SEC_PER_SAMPLE;

      if (!adsr->isHeld && adsr->section != ADSR_RSection)
      {
        adsr->section = ADSR_RSection;
        adsr->timeSinceSectionStart = 0;
      }
      else
      {
        progressEnvelope(adsr, currA, currD, currS, currR);
      }

    }

    finalVal = sampleEnvelope(adsr, currA, currD, currS, currR);

    adsr->prevADSRStop = finalVal;
    finalVal = (VOLTSTD_MOD_CV_RANGE * finalVal) - VOLTSTD_MOD_CV_MAX;

    OUT_PORT_ENV(adsr)[i] = finalVal;
  }

}

static void pushCurrToPrev(void * modPtr)
{
  ADSR * adsr = (ADSR *)modPtr;
  memcpy(adsr->outputPortsPrev, adsr->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * ADSR_OUTCOUNT);
  CONTROL_PUSH_TO_PREV(adsr);
}

static R4 * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port >= ADSR_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static R4 * getInputAddr(void * modPtr, ModularPortID port)
{
  if (port >= ADSR_INCOUNT) return NULL;

  return IN_PORT_ADDR(modPtr, port);
}

static U4 getInCount(void * modPtr)
{
  return ADSR_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return ADSR_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return ADSR_CONTROLCOUNT;
}

static void setControlVal(void * modPtr, ModularPortID id, R4 val)
{
  if (id >= ADSR_CONTROLCOUNT) return;

  ADSR * adsr = (ADSR *)modPtr;
  adsr->controlsCurr[id] = val;
}

static R4 getControlVal(void * modPtr, ModularPortID id)
{
  if (id >= ADSR_CONTROLCOUNT) return 0;

  ADSR * adsr = (ADSR *)modPtr;
  return adsr->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, R4 * readAddr)
{
  if (port >= ADSR_INCOUNT) return;

  ADSR * adsr = (ADSR *)modPtr;
  adsr->inputPorts[port] = readAddr;
}

static void progressEnvelope(ADSR * adsr, R4 currA, R4 currD, R4 currS, R4 currR)
{
  switch (adsr->section)
  {
    case ADSR_ASection:
    if (adsr->timeSinceSectionStart / currA + adsr->prevADSRVal > 1)
    {
      adsr->section = ADSR_DSection;
      adsr->timeSinceSectionStart = 0;
    }
    break;

    case ADSR_DSection:
    if (adsr->timeSinceSectionStart > currD)
    {
      adsr->section = ADSR_SSection;
      adsr->timeSinceSectionStart = 0;
    }
    break;

    case ADSR_SSection:
    // if (adsr->timeSinceSectionStart > currS)
    // {
    //   adsr->section = ADSR_RSection;
    //   adsr->timeSinceSectionStart = 0;
    // }
    if (!adsr->isHeld)
    {
      adsr->section = ADSR_RSection;
      adsr->timeSinceSectionStart = 0;
    }
    break;

    case ADSR_RSection:
    if (adsr->timeSinceSectionStart > currR)
    {
      adsr->section = ADSR_ASection;
      adsr->timeSinceSectionStart = 0;
      adsr->envelopeActive = 0;
      adsr->releaseStartVal = 0;
    }
    break;
  }
}

static R4 sampleEnvelope(ADSR * adsr, R4 currA, R4 currD, R4 currS, R4 currR)
{
  R4 val;
  switch (adsr->section)
  {
    case ADSR_ASection:
    // printf("A\n");
    val = adsr->timeSinceSectionStart / currA + adsr->prevADSRVal;
    adsr->releaseStartVal = val;
    return val;
    break;

    case ADSR_DSection:
    // printf("D\n");
    val = (1 - currS) * -adsr->timeSinceSectionStart / currD + 1;
    adsr->releaseStartVal = val;
    return val;
    break;

    case ADSR_SSection:
    // printf("S\n");
    adsr->releaseStartVal = currS;
    return currS;
    break;

    case ADSR_RSection:
    // printf("R\n");
    val = adsr->releaseStartVal * (-adsr->timeSinceSectionStart / currR + 1);
    return val;
    break;
  }

  return 0;
}
