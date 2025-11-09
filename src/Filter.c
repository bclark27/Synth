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
#define GET_CONTROL_CURR_DB(flt)        ((flt)->controlsCurr[FITLER_CONTROL_DB])
#define GET_CONTROL_PREV_DB(flt)        ((flt)->controlsPrev[FITLER_CONTROL_DB])
#define GET_CONTROL_CURR_Q(flt)        ((flt)->controlsCurr[FITLER_CONTROL_Q])
#define GET_CONTROL_PREV_Q(flt)        ((flt)->controlsPrev[FITLER_CONTROL_Q])
#define GET_CONTROL_CURR_ENV(flt)        ((flt)->controlsCurr[FITLER_CONTROL_ENV])
#define GET_CONTROL_PREV_ENV(flt)        ((flt)->controlsPrev[FITLER_CONTROL_ENV])
#define GET_CONTROL_CURR_TYPE(flt)        ((flt)->controlsCurr[FITLER_CONTROL_TYPE])
#define GET_CONTROL_PREV_TYPE(flt)        ((flt)->controlsPrev[FITLER_CONTROL_TYPE])

#define SET_CONTROL_CURR_FREQ(flt, v)     ((flt)->controlsCurr[FITLER_CONTROL_FREQ] = (v))
#define SET_CONTROL_PREV_FREQ(flt, v)     ((flt)->controlsPrev[FITLER_CONTROL_FREQ] = (v))
#define SET_CONTROL_CURR_DB(flt, v)     ((flt)->controlsCurr[FITLER_CONTROL_DB] = (v))
#define SET_CONTROL_PREV_DB(flt, v)     ((flt)->controlsPrev[FITLER_CONTROL_DB] = (v))
#define SET_CONTROL_CURR_Q(flt, q)     ((flt)->controlsCurr[FITLER_CONTROL_Q] = (q))
#define SET_CONTROL_PREV_Q(flt, q)     ((flt)->controlsPrev[FITLER_CONTROL_Q] = (q))
#define SET_CONTROL_CURR_ENV(flt, e)     ((flt)->controlsCurr[FITLER_CONTROL_ENV] = (e))
#define SET_CONTROL_PREV_ENV(flt, e)     ((flt)->controlsPrev[FITLER_CONTROL_ENV] = (e))
#define SET_CONTROL_CURR_TYPE(flt, t)     ((flt)->controlsCurr[FITLER_CONTROL_TYPE] = (t))
#define SET_CONTROL_PREV_TYPE(flt, t)     ((flt)->controlsPrev[FITLER_CONTROL_TYPE] = (t))

#define CONTROL_PUSH_TO_PREV(vco)         for (U4 i = 0; i < FITLER_CONTROLCOUNT; i++) {(vco)->controlsPrev[i] = (vco)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static inline void setLowpass(Filter *f, float cutoff, float q);
static inline void setHighpass(Filter *f, float cutoff, float q);
static inline float processFilter(Filter *f, float x);
static inline float voltToMoogQ(float volts);

static char * inPortNames[FITLER_INCOUNT] = {
  "Audio",
  "Freq",
};

static char * outPortNames[FITLER_OUTCOUNT] = {
  "Audio",
};

static char * controlNames[FITLER_CONTROLCOUNT] = {
  "Freq",
  "Q",
  "DB",
  "Env",
  "Type",
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

#define DEFAULT_CONTROL_FREQ   3
#define DEFAULT_CONTROL_Q   -5
#define DEFAULT_CONTROL_DB   0
#define DEFAULT_CONTROL_ENV   0
#define DEFAULT_CONTROL_TYPE   -5

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
  SET_CONTROL_CURR_ENV(flt, DEFAULT_CONTROL_ENV);
  SET_CONTROL_CURR_TYPE(flt, DEFAULT_CONTROL_TYPE);

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
  for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
  {
    // get the input voltage
    R4 inVoltsFreq = IN_PORT_FREQ(flt) ? IN_PORT_FREQ(flt)[i] : 0;

    // interp the control input
    R4 controlVoltsFreq = INTERP(GET_CONTROL_PREV_FREQ(flt), GET_CONTROL_CURR_FREQ(flt), MODULE_BUFFER_SIZE, i);
    R4 controlVoltsQ = INTERP(GET_CONTROL_PREV_Q(flt), GET_CONTROL_CURR_Q(flt), MODULE_BUFFER_SIZE, i);
    R4 controlVoltsDb = INTERP(GET_CONTROL_PREV_DB(flt), GET_CONTROL_CURR_DB(flt), MODULE_BUFFER_SIZE, i);
    R4 controlVoltsEnv = INTERP(GET_CONTROL_PREV_ENV(flt), GET_CONTROL_CURR_ENV(flt), MODULE_BUFFER_SIZE, i);
    R4 controlVoltsType = INTERP(GET_CONTROL_PREV_TYPE(flt), GET_CONTROL_CURR_TYPE(flt), MODULE_BUFFER_SIZE, i);

    R4 controlEnv = MAP(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, 0.0f, 1.0f, controlVoltsEnv);
    float filterType = MAP(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, 0.0f, 2.0f, controlVoltsType);

    R4 voltsFreq = CLAMP(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, controlEnv * inVoltsFreq + controlVoltsFreq);

    // convert voltage into usable values
    R4 realFreq = CLAMP(1, 22000, VoltUtils_voltToFreq(voltsFreq));
    R4 realQ = voltToMoogQ(controlVoltsQ);

    if (filterType < 1)
    {
      setLowpass(flt, realFreq, realQ);
    }
    else
    {
      setHighpass(flt, realFreq, realQ);
    }

    // out = mult * inputSig
    OUT_PORT_AUD(flt)[i] = processFilter(flt, IN_PORT_AUD(flt)[i]);
  }
}

static void pushCurrToPrev(void * modPtr)
{
  Filter * flt = (Filter *)modPtr;
  memcpy(flt->outputPortsPrev, flt->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * FITLER_OUTCOUNT);
  CONTROL_PUSH_TO_PREV(flt);
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

static inline void setLowpass(Filter *f, float cutoff, float q)
{
    float w0 = 2.0f * PI * cutoff / SAMPLE_RATE;
    float alpha = sinf(w0) / (2.0f * q);
    float cosw0 = cosf(w0);

    float b0 = (1.0f - cosw0) * 0.5f;
    float b1 = 1.0f - cosw0;
    float b2 = (1.0f - cosw0) * 0.5f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosw0;
    float a2 = 1.0f - alpha;

    f->b0 = b0 / a0;
    f->b1 = b1 / a0;
    f->b2 = b2 / a0;
    f->a1 = a1 / a0;
    f->a2 = a2 / a0;
}

static inline void setHighpass(Filter *f, float cutoff, float q)
{
    float w0 = 2.0f * PI * cutoff / SAMPLE_RATE;
    float cosw0 = cosf(w0);
    float sinw0 = sinf(w0);
    float alpha = sinw0 / (2.0f * q);

    float b0 =  (1 + cosw0) / 2;
    float b1 = -(1 + cosw0);
    float b2 =  (1 + cosw0) / 2;
    float a0 =  1 + alpha;
    float a1 = -2 * cosw0;
    float a2 =  1 - alpha;

    f->b0 = b0 / a0;
    f->b1 = b1 / a0;
    f->b2 = b2 / a0;
    f->a1 = a1 / a0;
    f->a2 = a2 / a0;
}

static inline float processFilter(Filter *f, float x)
{
    float y = f->b0*x + f->b1*f->x1 + f->b2*f->x2
              - f->a1*f->y1 - f->a2*f->y2;

    f->x2 = f->x1;
    f->x1 = x;
    f->y2 = f->y1;
    f->y1 = y;
    return y;
}

static inline float voltToMoogQ(float volts)
{
  const float Q_MIN = 0.5f;     // very gentle slope
  const float Q_MAX = 100.0f;   // near self-oscillation
  const float V_MIN = VOLTSTD_MOD_CV_MIN;
  const float V_MAX = VOLTSTD_MOD_CV_MAX;
  
  // Normalize voltage to 0..1
  float t = (volts - V_MIN) / (V_MAX - V_MIN);
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  
  // Exponential mapping: small voltage changes at high end give big Q changes
  float q = Q_MIN * powf(Q_MAX / Q_MIN, t);
  
  return q;
}