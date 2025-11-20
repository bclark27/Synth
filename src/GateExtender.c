#include "GateExtender.h"
#include "VoltUtils.h"

#define IN_PORT_ADDR(mod, port)		(((GateExtender*)(mod))->inputPorts[port]);
#define PREV_PORT_ADDR(mod, port)	(((GateExtender*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)	(((GateExtender*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_MIDI_PORT_ADDR(mod, port)		(((GateExtender*)(mod))->inputMIDIPorts[port])
#define PREV_MIDI_PORT_ADDR(mod, port)	(((GateExtender*)(mod))->outputMIDIPortsPrev + MIDI_STREAM_BUFFER_SIZE * (port))
#define CURR_MIDI_PORT_ADDR(mod, port)	(((GateExtender*)(mod))->outputMIDIPortsCurr + MIDI_STREAM_BUFFER_SIZE * (port))

#define IN_PORT_IN(gateextender)		((gateextender)->inputPorts[GATEEXTENDER_IN_PORT_IN])
#define IN_PORT_LENGTH(gateextender)		((gateextender)->inputPorts[GATEEXTENDER_IN_PORT_LENGTH])
#define OUT_PORT_OUT(gateextender)		(CURR_PORT_ADDR(gateextender, GATEEXTENDER_OUT_PORT_OUT))

#define GET_CONTROL_CURR_LENGTH(gateextender)	((gateextender)->controlsCurr[GATEEXTENDER_CONTROL_LENGTH])
#define GET_CONTROL_PREV_LENGTH(gateextender)	((gateextender)->controlsPrev[GATEEXTENDER_CONTROL_LENGTH])

#define SET_CONTROL_CURR_LENGTH(gateextender, v)	((gateextender)->controlsCurr[GATEEXTENDER_CONTROL_LENGTH] = (v))
#define SET_CONTROL_PREV_LENGTH(gateextender, v)	((gateextender)->controlsPrev[GATEEXTENDER_CONTROL_LENGTH] = (v))


#define CONTROL_PUSH_TO_PREV(gateextender)         for (U4 i = 0; i < GATEEXTENDER_CONTROLCOUNT; i++) {(gateextender)->controlsPrev[i] = (gateextender)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_gateextender(void * modPtr);
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
static void initTables();
static inline U4 fastVoltToSampleTime(R4 v);



//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool tableInitDone = false;
#define SAMPLE_TIME_TABLE_SIZE 128
static float voltToSampleTimeTable[SAMPLE_TIME_TABLE_SIZE];
static R4 maxVoltTable[MODULE_BUFFER_SIZE];
static R4 zeroVoltTable[MODULE_BUFFER_SIZE];

static char * inPortNames[GATEEXTENDER_INCOUNT] = {
	"In",
	"Length",
};
static char * outPortNames[GATEEXTENDER_OUTCOUNT] = {
	"Out",
};
static char * controlNames[GATEEXTENDER_CONTROLCOUNT] = {
	"Length",
};

static Module vtable = {
  .type = ModuleType_GateExtender,
  .freeModule = free_gateextender,
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
  .inPortNamesCount = ARRAY_LEN(inPortNames),
  .inPortNames = inPortNames,
  .outPortNamesCount = ARRAY_LEN(outPortNames),
  .outPortNames = outPortNames,
  .controlNamesCount = ARRAY_LEN(controlNames),
  .controlNames = controlNames,
};

#define DEFAULT_CONTROL_LENGTH    3

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


void GateExtender_initInPlace(GateExtender* gateextender, char* name)
{
  if (!tableInitDone)
  {
    initTables();
    tableInitDone = true;
  }

  // set vtable
  gateextender->module = vtable;

  // set name of module
  gateextender->module.name = name;
  
  // set all control values

	SET_CONTROL_CURR_LENGTH(gateextender, DEFAULT_CONTROL_LENGTH);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(gateextender);

  gateextender->lastInputVoltage = VOLTSTD_MOD_CV_ZERO;
}



Module * GateExtender_init(char* name)
{
  GateExtender * gateextender = (GateExtender*)calloc(1, sizeof(GateExtender));

  GateExtender_initInPlace(gateextender, name);
  return (Module*)gateextender;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_gateextender(void * modPtr)
{
  GateExtender * gateextender = (GateExtender *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(gateextender);
}

static void updateState(void * modPtr)
{
    GateExtender * gateextender = (GateExtender*)modPtr;

    R4* inSignal = IN_PORT_IN(gateextender) ? IN_PORT_IN(gateextender) : zeroVoltTable;
    R4* inLengthVolt = IN_PORT_LENGTH(gateextender) ? IN_PORT_LENGTH(gateextender) : maxVoltTable;
    R4 controlLengthVolt = GET_CONTROL_CURR_LENGTH(gateextender);

    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
        gateextender->gateHighNext = false;
        if (gateextender->lastInputVoltage < VOLTSTD_GATE_HIGH_THRESH && inSignal[i] >= VOLTSTD_GATE_HIGH_THRESH)
        {
            gateextender->gateHighNext = true;
        }

        if (gateextender->gateHighNext)
        {
            gateextender->lastInputVoltage = inSignal[i];
            gateextender->samplesSinceLastTrigger = 0;
            OUT_PORT_OUT(gateextender)[i] = VOLTSTD_GATE_LOW;
            continue;
        }

        gateextender->gateHighNext = false;
        U4 gateExpire = fastVoltToSampleTime(MAP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, VOLTSTD_MOD_CV_ZERO, controlLengthVolt, inLengthVolt[i]));
        
        if (gateextender->samplesSinceLastTrigger < gateExpire)
        {
            OUT_PORT_OUT(gateextender)[i] = VOLTSTD_GATE_HIGH;
            gateextender->samplesSinceLastTrigger++;
        }
        else
        {
            OUT_PORT_OUT(gateextender)[i] = VOLTSTD_GATE_LOW;
        }

        gateextender->lastInputVoltage = inSignal[i];
    }
}


static void pushCurrToPrev(void * modPtr)
{
  GateExtender * mi = (GateExtender*)modPtr;
  memcpy(mi->outputPortsPrev, mi->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * GATEEXTENDER_OUTCOUNT);
  memcpy(mi->outputMIDIPortsPrev, mi->outputMIDIPortsCurr, sizeof(MIDIData) * GATEEXTENDER_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE);
  CONTROL_PUSH_TO_PREV(mi);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port < GATEEXTENDER_OUTCOUNT) return PREV_PORT_ADDR(modPtr, port);
  else if ((port - GATEEXTENDER_OUTCOUNT) < GATEEXTENDER_MIDI_OUTCOUNT) return PREV_MIDI_PORT_ADDR(modPtr, port - GATEEXTENDER_OUTCOUNT);

  return NULL;
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
    if (port < GATEEXTENDER_INCOUNT) {return IN_PORT_ADDR(modPtr, port);}
    else if ((port - GATEEXTENDER_INCOUNT) < GATEEXTENDER_MIDI_INCOUNT) return IN_MIDI_PORT_ADDR(modPtr, port - GATEEXTENDER_INCOUNT);

  return NULL;
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < GATEEXTENDER_INCOUNT) return ModulePortType_VoltStream;
  else if ((port - GATEEXTENDER_INCOUNT) < GATEEXTENDER_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < GATEEXTENDER_OUTCOUNT) return ModulePortType_VoltStream;
  else if ((port - GATEEXTENDER_OUTCOUNT) < GATEEXTENDER_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < GATEEXTENDER_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}

static U4 getInCount(void * modPtr)
{
  return GATEEXTENDER_INCOUNT + GATEEXTENDER_MIDI_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return GATEEXTENDER_OUTCOUNT + GATEEXTENDER_MIDI_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return GATEEXTENDER_CONTROLCOUNT;
}


static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
    if (id < GATEEXTENDER_CONTROLCOUNT)
  {
    Volt v = *(Volt*)val;
    switch (id)
    {

        case GATEEXTENDER_CONTROL_LENGTH:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    

      default:
        break;
    }

    ((GateExtender*)modPtr)->controlsCurr[id] = v;
  }
}


static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id < GATEEXTENDER_CONTROLCOUNT) *(Volt*)ret = ((GateExtender*)modPtr)->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
    GateExtender * mi = (GateExtender*)modPtr;
  if (port < GATEEXTENDER_INCOUNT) mi->inputPorts[port] = (R4*)readAddr;
  else if ((port - GATEEXTENDER_INCOUNT) < GATEEXTENDER_MIDI_INCOUNT) mi->inputMIDIPorts[port - GATEEXTENDER_INCOUNT] = (MIDIData*)readAddr;
}

static void initTables()
{
    float inc = (VOLTSTD_MOD_CV_MAX - VOLTSTD_MOD_CV_ZERO) / (float)SAMPLE_TIME_TABLE_SIZE;
    float v = VOLTSTD_MOD_CV_ZERO;
    for (int i = 0; i < SAMPLE_TIME_TABLE_SIZE; i++)
    {
        voltToSampleTimeTable[i] = (0.01f * pow(2.3868f, v)) * SAMPLE_RATE;
        v += inc;
    }

    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
        maxVoltTable[i] = VOLTSTD_MOD_CV_MAX;
        zeroVoltTable[i] = VOLTSTD_MOD_CV_ZERO;
    }
}

static inline U4 fastVoltToSampleTime(R4 v)
{
    v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
    float idx = (v / (VOLTSTD_MOD_CV_MAX - VOLTSTD_MOD_CV_ZERO)) * (SAMPLE_TIME_TABLE_SIZE - 1);
    int i = (int)idx;
    float frac = idx - i;
    float ret = voltToSampleTimeTable[i] * (1.0f - frac)
        + voltToSampleTimeTable[i + 1] * frac;
    return ret;
}