#include "Filter.h"

#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define EPSILON (0.0005f)

#define IN_PORT_ADDR(mod, port)           (((Filter*)(mod))->inputPorts[port]);

#define PREV_PORT_ADDR(mod, port)         (((Filter*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)         (((Filter*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_PORT_AUD(flt)                 ((flt)->inputPorts[FILTER_IN_PORT_AUD])
#define IN_PORT_FREQ(flt)                 ((flt)->inputPorts[FILTER_IN_PORT_FREQ])
#define IN_PORT_Q(flt)                 ((flt)->inputPorts[FILTER_IN_PORT_Q])

#define OUT_PORT_AUD(flt)                (CURR_PORT_ADDR(flt, FILTER_OUT_PORT_AUD))

#define GET_CONTROL_CURR_FREQ(flt)        ((flt)->controlsCurr[FILTER_CONTROL_FREQ])
#define GET_CONTROL_PREV_FREQ(flt)        ((flt)->controlsPrev[FILTER_CONTROL_FREQ])
#define GET_CONTROL_CURR_DB(flt)        ((flt)->controlsCurr[FILTER_CONTROL_DB])
#define GET_CONTROL_PREV_DB(flt)        ((flt)->controlsPrev[FILTER_CONTROL_DB])
#define GET_CONTROL_CURR_Q(flt)        ((flt)->controlsCurr[FILTER_CONTROL_Q])
#define GET_CONTROL_PREV_Q(flt)        ((flt)->controlsPrev[FILTER_CONTROL_Q])
#define GET_CONTROL_CURR_ENV(flt)        ((flt)->controlsCurr[FILTER_CONTROL_ENV])
#define GET_CONTROL_PREV_ENV(flt)        ((flt)->controlsPrev[FILTER_CONTROL_ENV])
#define GET_CONTROL_CURR_TYPE(flt)        ((flt)->controlsCurr[FILTER_CONTROL_TYPE])
#define GET_CONTROL_PREV_TYPE(flt)        ((flt)->controlsPrev[FILTER_CONTROL_TYPE])

#define SET_CONTROL_CURR_FREQ(flt, v)     ((flt)->controlsCurr[FILTER_CONTROL_FREQ] = (v))
#define SET_CONTROL_PREV_FREQ(flt, v)     ((flt)->controlsPrev[FILTER_CONTROL_FREQ] = (v))
#define SET_CONTROL_CURR_DB(flt, v)     ((flt)->controlsCurr[FILTER_CONTROL_DB] = (v))
#define SET_CONTROL_PREV_DB(flt, v)     ((flt)->controlsPrev[FILTER_CONTROL_DB] = (v))
#define SET_CONTROL_CURR_Q(flt, q)     ((flt)->controlsCurr[FILTER_CONTROL_Q] = (q))
#define SET_CONTROL_PREV_Q(flt, q)     ((flt)->controlsPrev[FILTER_CONTROL_Q] = (q))
#define SET_CONTROL_CURR_ENV(flt, e)     ((flt)->controlsCurr[FILTER_CONTROL_ENV] = (e))
#define SET_CONTROL_PREV_ENV(flt, e)     ((flt)->controlsPrev[FILTER_CONTROL_ENV] = (e))
#define SET_CONTROL_CURR_TYPE(flt, t)     ((flt)->controlsCurr[FILTER_CONTROL_TYPE] = (t))
#define SET_CONTROL_PREV_TYPE(flt, t)     ((flt)->controlsPrev[FILTER_CONTROL_TYPE] = (t))

#define CONTROL_PUSH_TO_PREV(vco)         for (U4 i = 0; i < FILTER_CONTROLCOUNT; i++) {(vco)->controlsPrev[i] = (vco)->controlsCurr[i];} for (U4 i = 0; i < FILTER_MIDI_CONTROLCOUNT; i++) {(vco)->midiControlsPrev[i] = (vco)->midiControlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

#define VOLT_TABLE_SIZE 1024
static float volt_to_freq_table[VOLT_TABLE_SIZE];
#define Q_TABLE_SIZE 1024
static float volt_to_q_table[Q_TABLE_SIZE];
#define SIN_TABLE_SIZE 1024
static float sin_table[SIN_TABLE_SIZE];
static bool tableInitDone = false;

static inline void setLowpass(Filter *f, float cutoff, float q);
static inline void setHighpass(Filter *f, float cutoff, float q);
static inline float processFilter(Filter *f, float x);
static inline float voltToMoogQ(float volts);

static char * inPortNames[FILTER_INCOUNT] = {
  "Audio",
  "Freq",
};

static char * outPortNames[FILTER_OUTCOUNT] = {
  "Audio",
};

static char * controlNames[FILTER_CONTROLCOUNT] = {
  "Freq",
  "Q",
  "DB",
  "Env",
  "Type",
};

static void free_attn(void * modPtr);
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
static void initVoltTable();
static inline float fastVoltToFreq(float v);
static void initQTable();
static inline float fastVoltToQ(float v);

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

#define DEFAULT_CONTROL_FREQ   VOLTSTD_MOD_CV_MAX
#define DEFAULT_CONTROL_Q   ((VOLTSTD_MOD_CV_MAX + VOLTSTD_MOD_CV_ZERO) / 4)
#define DEFAULT_CONTROL_DB   VOLTSTD_MOD_CV_ZERO
#define DEFAULT_CONTROL_ENV   VOLTSTD_MOD_CV_MAX
#define DEFAULT_CONTROL_TYPE   VOLTSTD_MOD_CV_ZERO

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

void Filter_initInPlace(Filter* flt, char* name)
{
  if (!tableInitDone)
  {
    tableInitDone = true;
    initVoltTable();
    initQTable();
  }

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

  flt->prevVoltsFreq = -999;
  flt->prevControlVoltsQ = -999;
  flt->prevVoltsFreq = -999;
}

Module * Filter_init(char* name)
{
  

  Filter * flt = calloc(1, sizeof(Filter));

  Filter_initInPlace(flt, name);

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
    
    // interp the control input
    //R4 controlVoltsDb = INTERP(GET_CONTROL_PREV_DB(flt), GET_CONTROL_CURR_DB(flt), MODULE_BUFFER_SIZE, i);
    
    
    
    // convert voltage into usable values
    //printf("%f, %f, %f, %f\n", realQ, controlVoltsEnv, inVoltsFreqEnv, controlVoltsFreq);
    
    if ((i & 7) == 7)
    {
      R4 inVoltsFreqEnv = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, IN_PORT_FREQ(flt) ? IN_PORT_FREQ(flt)[i] : VOLTSTD_MOD_CV_ZERO); // [0, 10]
      R4 controlVoltsFreq = INTERP(GET_CONTROL_PREV_FREQ(flt), GET_CONTROL_CURR_FREQ(flt), MODULE_BUFFER_SIZE, i);// [0, 10]
      R4 controlVoltsQ = INTERP(GET_CONTROL_PREV_Q(flt), GET_CONTROL_CURR_Q(flt), MODULE_BUFFER_SIZE, i);// [0, 10]
      R4 controlVoltsEnv = INTERP(GET_CONTROL_PREV_ENV(flt), GET_CONTROL_CURR_ENV(flt), MODULE_BUFFER_SIZE, i);// [0, 10]
      R4 controlEnvMult = MAP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, 0.0f, 1.0f, controlVoltsEnv);
      R4 voltsFreq = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, (controlEnvMult * inVoltsFreqEnv) + controlVoltsFreq);
      R4 controlVoltsType = INTERP(GET_CONTROL_PREV_TYPE(flt), GET_CONTROL_CURR_TYPE(flt), MODULE_BUFFER_SIZE, i);// [0, 10]

      bool change = fabsf(flt->prevVoltsFreq - voltsFreq) > EPSILON ||
                  fabsf(flt->prevControlVoltsQ - controlVoltsQ) > EPSILON ||
                  fabsf(flt->prevType - controlVoltsType) > EPSILON;

      if (change)
      {
        flt->prevVoltsFreq = voltsFreq;
        flt->prevControlVoltsQ = controlVoltsQ;
        flt->prevType = controlVoltsType;

        R4 realFreq = fastVoltToFreq(voltsFreq);//fastVoltToFreq(voltsFreq);//20 * powf(2, voltsFreq);
        R4 realQ = fastVoltToQ(controlVoltsQ);//voltToMoogQ(controlVoltsQ);
  
        if (controlVoltsType < 5)
        {
          setLowpass(flt, realFreq, realQ);
        }
        else
        {
          setHighpass(flt, realFreq, realQ);
        }
      }
    }

    OUT_PORT_AUD(flt)[i] = processFilter(flt, IN_PORT_AUD(flt)[i]);
  }
}

static void pushCurrToPrev(void * modPtr)
{
  Filter * flt = (Filter *)modPtr;
  memcpy(flt->outputPortsPrev, flt->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * FILTER_OUTCOUNT);
  memcpy(flt->outputMIDIPortsPrev, flt->outputMIDIPortsCurr, sizeof(MIDIData) * FILTER_MIDI_OUTCOUNT);
  CONTROL_PUSH_TO_PREV(flt);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port >= FILTER_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
  if (port >= FILTER_INCOUNT) return NULL;

  return IN_PORT_ADDR(modPtr, port);
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < FILTER_INCOUNT) return ModulePortType_VoltStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < FILTER_OUTCOUNT) return ModulePortType_VoltStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < FILTER_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}


static U4 getInCount(void * modPtr)
{
  return FILTER_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return FILTER_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return FILTER_CONTROLCOUNT;
}

static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
  if (id < FILTER_CONTROLCOUNT)
  {
    Volt v = *(Volt*)val;
    switch (id)
    {
      case FILTER_CONTROL_FREQ:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case FILTER_CONTROL_Q:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case FILTER_CONTROL_DB:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case FILTER_CONTROL_ENV:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case FILTER_CONTROL_TYPE:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      default:
        break;
    }

    ((Filter*)modPtr)->controlsCurr[id] = v;
  }
  else if ((id - FILTER_CONTROLCOUNT) < FILTER_MIDI_CONTROLCOUNT)
  {
    ((Filter*)modPtr)->midiControlsCurr[id - FILTER_CONTROLCOUNT] = *(MIDIData*)val;
  }
}

static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id >= FILTER_CONTROLCOUNT) return;

  Filter * vco = (Filter *)modPtr;
  *((Volt*)ret) = vco->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
  if (port >= FILTER_INCOUNT) return;

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
  const float V_MIN = VOLTSTD_MOD_CV_ZERO;
  const float V_MAX = VOLTSTD_MOD_CV_MAX;
  
  // Normalize voltage to 0..1
  float t = (volts - V_MIN) / (V_MAX - V_MIN);
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  
  // Exponential mapping: small voltage changes at high end give big Q changes
  float q = Q_MIN * powf(Q_MAX / Q_MIN, t);
  
  return q;
}

static void initVoltTable()
{
  for (int i = 0; i < VOLT_TABLE_SIZE; i++) 
  {
      float v = (float)i / (VOLT_TABLE_SIZE - 1) * 10.0f; // 0→10 V
      volt_to_freq_table[i] = 20.0f * powf(2.0f, v);
  }
}

static inline float fastVoltToFreq(float v)
{
  if (v < 0) v = 0;
  if (v > 10) v = 10;
  float idx = v / 10.0f * (VOLT_TABLE_SIZE - 1);
  int i = (int)idx;
  float frac = idx - i;
  return volt_to_freq_table[i] * (1.0f - frac)
       + volt_to_freq_table[i + 1] * frac; // linear interpolation
}

static void initQTable() {
  const float Q_MIN = 0.5f, Q_MAX = 100.0f;
  for (int i = 0; i < Q_TABLE_SIZE; i++) {
      float t = (float)i / (Q_TABLE_SIZE - 1);
      volt_to_q_table[i] = Q_MIN * powf(Q_MAX / Q_MIN, t);
  }
}

static inline float fastVoltToQ(float v) {
  if (v < 0) v = 0;
  if (v > 10) v = 10;
  float idx = v / 10.0f * (Q_TABLE_SIZE - 1);
  int i = (int)idx;
  float frac = idx - i;
  return volt_to_q_table[i] * (1.0f - frac)
       + volt_to_q_table[i + 1] * frac;
}
