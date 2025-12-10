#include "SampleHold.h"

#define IN_PORT_ADDR(mod, port)		(((SampleHold*)(mod))->inputPorts[port]);
#define PREV_PORT_ADDR(mod, port)	(((SampleHold*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)	(((SampleHold*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_MIDI_PORT_ADDR(mod, port)		(((SampleHold*)(mod))->inputMIDIPorts[port])
#define PREV_MIDI_PORT_ADDR(mod, port)	(((SampleHold*)(mod))->outputMIDIPortsPrev + MIDI_STREAM_BUFFER_SIZE * (port))
#define CURR_MIDI_PORT_ADDR(mod, port)	(((SampleHold*)(mod))->outputMIDIPortsCurr + MIDI_STREAM_BUFFER_SIZE * (port))

#define IN_PORT_TRIGGER(samplehold)		((samplehold)->inputPorts[SAMPLEHOLD_IN_PORT_TRIGGER])
#define IN_PORT_IN0(samplehold)		((samplehold)->inputPorts[SAMPLEHOLD_IN_PORT_IN0])
#define IN_PORT_IN1(samplehold)		((samplehold)->inputPorts[SAMPLEHOLD_IN_PORT_IN1])
#define IN_PORT_IN2(samplehold)		((samplehold)->inputPorts[SAMPLEHOLD_IN_PORT_IN2])
#define IN_PORT_IN3(samplehold)		((samplehold)->inputPorts[SAMPLEHOLD_IN_PORT_IN3])
#define IN_PORT_TRIGGER0(samplehold)		((samplehold)->inputPorts[SAMPLEHOLD_IN_PORT_TRIGGER0])
#define IN_PORT_TRIGGER1(samplehold)		((samplehold)->inputPorts[SAMPLEHOLD_IN_PORT_TRIGGER1])
#define IN_PORT_TRIGGER2(samplehold)		((samplehold)->inputPorts[SAMPLEHOLD_IN_PORT_TRIGGER2])
#define IN_PORT_TRIGGER3(samplehold)		((samplehold)->inputPorts[SAMPLEHOLD_IN_PORT_TRIGGER3])
#define OUT_PORT_OUT0(samplehold)		(CURR_PORT_ADDR(samplehold, SAMPLEHOLD_OUT_PORT_OUT0))
#define OUT_PORT_OUT1(samplehold)		(CURR_PORT_ADDR(samplehold, SAMPLEHOLD_OUT_PORT_OUT1))
#define OUT_PORT_OUT2(samplehold)		(CURR_PORT_ADDR(samplehold, SAMPLEHOLD_OUT_PORT_OUT2))
#define OUT_PORT_OUT3(samplehold)		(CURR_PORT_ADDR(samplehold, SAMPLEHOLD_OUT_PORT_OUT3))




#define CONTROL_PUSH_TO_PREV(samplehold)         for (U4 i = 0; i < SAMPLEHOLD_CONTROLCOUNT; i++) {(samplehold)->controlsPrev[i] = (samplehold)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_samplehold(void * modPtr);
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
static void setControlVal(void * modPtr, ModularPortID id, void* val, unsigned int len);
static void getControlVal(void * modPtr, ModularPortID id, void* ret, unsigned int* len);
static void linkToInput(void * modPtr, ModularPortID port, void * readAddr);
static void initTables();



//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool tableInitDone = false;
static R4 noTriggerTable[MODULE_BUFFER_SIZE];

static char * inPortNames[SAMPLEHOLD_INCOUNT] = {
	"Trigger",
	"In0",
	"In1",
	"In2",
	"In3",
	"Trigger0",
	"Trigger1",
	"Trigger2",
	"Trigger3",
};
static char * outPortNames[SAMPLEHOLD_OUTCOUNT] = {
	"Out0",
	"Out1",
	"Out2",
	"Out3",
};
static char * controlNames[SAMPLEHOLD_CONTROLCOUNT] = {
};

static Module vtable = {
  .type = ModuleType_SampleHold,
  .freeModule = free_samplehold,
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


//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


void SampleHold_initInPlace(SampleHold* samplehold, char* name)
{
  if (!tableInitDone)
  {
    initTables();
    tableInitDone = true;
  }

  // set vtable
  samplehold->module = vtable;

  // set name of module
  samplehold->module.name = name;
  
  // set all control values


  // push curr to prev
  CONTROL_PUSH_TO_PREV(samplehold);

}



Module * SampleHold_init(char* name)
{
  SampleHold * samplehold = (SampleHold*)calloc(1, sizeof(SampleHold));

  SampleHold_initInPlace(samplehold, name);
  return (Module*)samplehold;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_samplehold(void * modPtr)
{
  SampleHold * samplehold = (SampleHold *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(samplehold);
}

static void updateState(void * modPtr)
{
    SampleHold * samplehold = (SampleHold*)modPtr;

    R4* globalTrigger = IN_PORT_TRIGGER(samplehold) ? IN_PORT_TRIGGER(samplehold) : noTriggerTable;

    R4* t0 = IN_PORT_TRIGGER0(samplehold) ? IN_PORT_TRIGGER0(samplehold) : globalTrigger;
    R4* t1 = IN_PORT_TRIGGER1(samplehold) ? IN_PORT_TRIGGER1(samplehold) : globalTrigger;
    R4* t2 = IN_PORT_TRIGGER2(samplehold) ? IN_PORT_TRIGGER2(samplehold) : globalTrigger;
    R4* t3 = IN_PORT_TRIGGER3(samplehold) ? IN_PORT_TRIGGER3(samplehold) : globalTrigger;
    
    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
        if (t0[i] < 1.f && samplehold->lastTriggerVoltage[0] >= 1.f && IN_PORT_IN0(samplehold))
        {
            samplehold->holding[0] = IN_PORT_IN0(samplehold)[i];
            OUT_PORT_OUT0(samplehold)[i] = samplehold->holding[0];
        }
        else
        {
            OUT_PORT_OUT0(samplehold)[i] = samplehold->holding[0];
        }
        
        if (t1[i] < 1.f && samplehold->lastTriggerVoltage[1] >= 1.f && IN_PORT_IN1(samplehold))
        {
            samplehold->holding[1] = IN_PORT_IN1(samplehold)[i];
            OUT_PORT_OUT1(samplehold)[i] = samplehold->holding[1];
        }
        else
        {
            OUT_PORT_OUT1(samplehold)[i] = samplehold->holding[1];
        }

        if (t2[i] < 1.f && samplehold->lastTriggerVoltage[2] >= 1.f && IN_PORT_IN2(samplehold))
        {
            samplehold->holding[2] = IN_PORT_IN2(samplehold)[i];
            OUT_PORT_OUT2(samplehold)[i] = samplehold->holding[2];
        }
        else
        {
            OUT_PORT_OUT2(samplehold)[i] = samplehold->holding[2];
        }

        if (t3[i] < 1.f && samplehold->lastTriggerVoltage[3] >= 1.f && IN_PORT_IN3(samplehold))
        {
            samplehold->holding[3] = IN_PORT_IN3(samplehold)[i];
            OUT_PORT_OUT3(samplehold)[i] = samplehold->holding[3];
        }
        else
        {
            OUT_PORT_OUT3(samplehold)[i] = samplehold->holding[3];
        }

        samplehold->lastTriggerVoltage[0] = t0[i];
        samplehold->lastTriggerVoltage[1] = t1[i];
        samplehold->lastTriggerVoltage[2] = t2[i];
        samplehold->lastTriggerVoltage[3] = t3[i];
    }
}


static void pushCurrToPrev(void * modPtr)
{
  SampleHold * mi = (SampleHold*)modPtr;
  memcpy(mi->outputPortsPrev, mi->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * SAMPLEHOLD_OUTCOUNT);
  memcpy(mi->outputMIDIPortsPrev, mi->outputMIDIPortsCurr, sizeof(MIDIData) * SAMPLEHOLD_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE);
  CONTROL_PUSH_TO_PREV(mi);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port < SAMPLEHOLD_OUTCOUNT) return PREV_PORT_ADDR(modPtr, port);
  else if ((port - SAMPLEHOLD_OUTCOUNT) < SAMPLEHOLD_MIDI_OUTCOUNT) return PREV_MIDI_PORT_ADDR(modPtr, port - SAMPLEHOLD_OUTCOUNT);

  return NULL;
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
    if (port < SAMPLEHOLD_INCOUNT) {return IN_PORT_ADDR(modPtr, port);}
    else if ((port - SAMPLEHOLD_INCOUNT) < SAMPLEHOLD_MIDI_INCOUNT) return IN_MIDI_PORT_ADDR(modPtr, port - SAMPLEHOLD_INCOUNT);

  return NULL;
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < SAMPLEHOLD_INCOUNT) return ModulePortType_VoltStream;
  else if ((port - SAMPLEHOLD_INCOUNT) < SAMPLEHOLD_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < SAMPLEHOLD_OUTCOUNT) return ModulePortType_VoltStream;
  else if ((port - SAMPLEHOLD_OUTCOUNT) < SAMPLEHOLD_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < SAMPLEHOLD_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}

static U4 getInCount(void * modPtr)
{
  return SAMPLEHOLD_INCOUNT + SAMPLEHOLD_MIDI_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return SAMPLEHOLD_OUTCOUNT + SAMPLEHOLD_MIDI_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return SAMPLEHOLD_CONTROLCOUNT;
}


static void setControlVal(void * modPtr, ModularPortID id, void* val, unsigned int len)
{
    if (id < SAMPLEHOLD_CONTROLCOUNT)
  {
    Volt v = *(Volt*)val;
    switch (id)
    {

      default:
        break;
    }

    ((SampleHold*)modPtr)->controlsCurr[id] = v;
  }
}


static void getControlVal(void * modPtr, ModularPortID id, void* ret, unsigned int* len)
{
  if (id < SAMPLEHOLD_CONTROLCOUNT)
  {
    *len = sizeof(Volt);
    *(Volt*)ret = ((SampleHold*)modPtr)->controlsCurr[id];
  }
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
    SampleHold * mi = (SampleHold*)modPtr;
  if (port < SAMPLEHOLD_INCOUNT) mi->inputPorts[port] = (R4*)readAddr;
  else if ((port - SAMPLEHOLD_INCOUNT) < SAMPLEHOLD_MIDI_INCOUNT) mi->inputMIDIPorts[port - SAMPLEHOLD_INCOUNT] = (MIDIData*)readAddr;
}

static void initTables()
{
    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
        noTriggerTable[i] = VOLTSTD_MOD_CV_ZERO;
    }
}