#include "Slew.h"
#include "VoltUtils.h"

#define IN_PORT_ADDR(mod, port)		(((Slew*)(mod))->inputPorts[port]);
#define PREV_PORT_ADDR(mod, port)	(((Slew*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)	(((Slew*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_MIDI_PORT_ADDR(mod, port)		(((Slew*)(mod))->inputMIDIPorts[port])
#define PREV_MIDI_PORT_ADDR(mod, port)	(((Slew*)(mod))->outputMIDIPortsPrev + MIDI_STREAM_BUFFER_SIZE * (port))
#define CURR_MIDI_PORT_ADDR(mod, port)	(((Slew*)(mod))->outputMIDIPortsCurr + MIDI_STREAM_BUFFER_SIZE * (port))

#define IN_PORT_IN0(slew)		((slew)->inputPorts[SLEW_IN_PORT_IN0])
#define IN_PORT_IN1(slew)		((slew)->inputPorts[SLEW_IN_PORT_IN1])
#define IN_PORT_IN2(slew)		((slew)->inputPorts[SLEW_IN_PORT_IN2])
#define IN_PORT_IN3(slew)		((slew)->inputPorts[SLEW_IN_PORT_IN3])
#define IN_PORT_SLEW0(slew)		((slew)->inputPorts[SLEW_IN_PORT_SLEW0])
#define IN_PORT_SLEW1(slew)		((slew)->inputPorts[SLEW_IN_PORT_SLEW1])
#define IN_PORT_SLEW2(slew)		((slew)->inputPorts[SLEW_IN_PORT_SLEW2])
#define IN_PORT_SLEW3(slew)		((slew)->inputPorts[SLEW_IN_PORT_SLEW3])
#define OUT_PORT_OUT0(slew)		(CURR_PORT_ADDR(slew, SLEW_OUT_PORT_OUT0))
#define OUT_PORT_OUT1(slew)		(CURR_PORT_ADDR(slew, SLEW_OUT_PORT_OUT1))
#define OUT_PORT_OUT2(slew)		(CURR_PORT_ADDR(slew, SLEW_OUT_PORT_OUT2))
#define OUT_PORT_OUT3(slew)		(CURR_PORT_ADDR(slew, SLEW_OUT_PORT_OUT3))

#define GET_CONTROL_CURR_SLEW0(slew)	((slew)->controlsCurr[SLEW_CONTROL_SLEW0])
#define GET_CONTROL_PREV_SLEW0(slew)	((slew)->controlsPrev[SLEW_CONTROL_SLEW0])
#define GET_CONTROL_CURR_SLEW1(slew)	((slew)->controlsCurr[SLEW_CONTROL_SLEW1])
#define GET_CONTROL_PREV_SLEW1(slew)	((slew)->controlsPrev[SLEW_CONTROL_SLEW1])
#define GET_CONTROL_CURR_SLEW2(slew)	((slew)->controlsCurr[SLEW_CONTROL_SLEW2])
#define GET_CONTROL_PREV_SLEW2(slew)	((slew)->controlsPrev[SLEW_CONTROL_SLEW2])
#define GET_CONTROL_CURR_SLEW3(slew)	((slew)->controlsCurr[SLEW_CONTROL_SLEW3])
#define GET_CONTROL_PREV_SLEW3(slew)	((slew)->controlsPrev[SLEW_CONTROL_SLEW3])

#define SET_CONTROL_CURR_SLEW0(slew, v)	((slew)->controlsCurr[SLEW_CONTROL_SLEW0] = (v))
#define SET_CONTROL_PREV_SLEW0(slew, v)	((slew)->controlsPrev[SLEW_CONTROL_SLEW0] = (v))
#define SET_CONTROL_CURR_SLEW1(slew, v)	((slew)->controlsCurr[SLEW_CONTROL_SLEW1] = (v))
#define SET_CONTROL_PREV_SLEW1(slew, v)	((slew)->controlsPrev[SLEW_CONTROL_SLEW1] = (v))
#define SET_CONTROL_CURR_SLEW2(slew, v)	((slew)->controlsCurr[SLEW_CONTROL_SLEW2] = (v))
#define SET_CONTROL_PREV_SLEW2(slew, v)	((slew)->controlsPrev[SLEW_CONTROL_SLEW2] = (v))
#define SET_CONTROL_CURR_SLEW3(slew, v)	((slew)->controlsCurr[SLEW_CONTROL_SLEW3] = (v))
#define SET_CONTROL_PREV_SLEW3(slew, v)	((slew)->controlsPrev[SLEW_CONTROL_SLEW3] = (v))


#define CONTROL_PUSH_TO_PREV(slew)         for (U4 i = 0; i < SLEW_CONTROLCOUNT; i++) {(slew)->controlsPrev[i] = (slew)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_slew(void * modPtr);
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
static inline float fastVoltToSlewAlpha(float v);
static void initTables();



//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool tableInitDone = false;
static R4 maxVoltTable[MODULE_BUFFER_SIZE];
static R4 zeroVoltTable[MODULE_BUFFER_SIZE];
#define SLEW_VOLT_TO_ALPHA_TABLE_SIZE 1024
static R4 voltToAlphaTable[SLEW_VOLT_TO_ALPHA_TABLE_SIZE];

static char * inPortNames[SLEW_INCOUNT] = {
	"In0",
	"In1",
	"In2",
	"In3",
	"Slew0",
	"Slew1",
	"Slew2",
	"Slew3",
};
static char * outPortNames[SLEW_OUTCOUNT] = {
	"Out0",
	"Out1",
	"Out2",
	"Out3",
};
static char * controlNames[SLEW_CONTROLCOUNT] = {
	"Slew0",
	"Slew1",
	"Slew2",
	"Slew3",
};

static Module vtable = {
  .type = ModuleType_ADSR,
  .freeModule = free_slew,
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

#define DEFAULT_CONTROL_SLEW0    1.f
#define DEFAULT_CONTROL_SLEW1    1.f
#define DEFAULT_CONTROL_SLEW2    1.f
#define DEFAULT_CONTROL_SLEW3    1.f

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


void Slew_initInPlace(Slew* slew, char* name)
{
  if (!tableInitDone)
  {
    initTables();
    tableInitDone = true;
  }

  // set vtable
  slew->module = vtable;

  // set name of module
  slew->module.name = name;
  
  // set all control values

	SET_CONTROL_CURR_SLEW0(slew, DEFAULT_CONTROL_SLEW0);
	SET_CONTROL_CURR_SLEW1(slew, DEFAULT_CONTROL_SLEW1);
	SET_CONTROL_CURR_SLEW2(slew, DEFAULT_CONTROL_SLEW2);
	SET_CONTROL_CURR_SLEW3(slew, DEFAULT_CONTROL_SLEW3);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(slew);

}



Module * Slew_init(char* name)
{
  Slew * slew = (Slew*)calloc(1, sizeof(Slew));

  Slew_initInPlace(slew, name);
  return (Module*)slew;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_slew(void * modPtr)
{
  Slew * slew = (Slew *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(slew);
}

static void updateState(void * modPtr)
{
    Slew * slew = (Slew*)modPtr;

    R4 controlSlew0 = GET_CONTROL_CURR_SLEW0(slew);
    R4 controlSlew1 = GET_CONTROL_CURR_SLEW1(slew);
    R4 controlSlew2 = GET_CONTROL_CURR_SLEW2(slew);
    R4 controlSlew3 = GET_CONTROL_CURR_SLEW3(slew);

    R4* in0 = IN_PORT_IN0(slew) ? IN_PORT_IN0(slew) : zeroVoltTable;
    R4* in1 = IN_PORT_IN1(slew) ? IN_PORT_IN1(slew) : zeroVoltTable;
    R4* in2 = IN_PORT_IN2(slew) ? IN_PORT_IN2(slew) : zeroVoltTable;
    R4* in3 = IN_PORT_IN3(slew) ? IN_PORT_IN3(slew) : zeroVoltTable;

    R4* slewCv0 = IN_PORT_SLEW0(slew) ? IN_PORT_SLEW0(slew) : zeroVoltTable;
    R4* slewCv1 = IN_PORT_SLEW1(slew) ? IN_PORT_SLEW1(slew) : zeroVoltTable;
    R4* slewCv2 = IN_PORT_SLEW2(slew) ? IN_PORT_SLEW2(slew) : zeroVoltTable;
    R4* slewCv3 = IN_PORT_SLEW3(slew) ? IN_PORT_SLEW3(slew) : zeroVoltTable;

    R4 prevSlew0 = slew->prevSlew[0];
    R4 prevSlew1 = slew->prevSlew[1];
    R4 prevSlew2 = slew->prevSlew[2];
    R4 prevSlew3 = slew->prevSlew[3];

    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
        prevSlew0 = prevSlew0 + (in0[i] - prevSlew0) * fastVoltToSlewAlpha(CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, slewCv0[i] + controlSlew0));
        prevSlew1 = prevSlew1 + (in1[i] - prevSlew1) * fastVoltToSlewAlpha(CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, slewCv1[i] + controlSlew1));
        prevSlew2 = prevSlew2 + (in2[i] - prevSlew2) * fastVoltToSlewAlpha(CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, slewCv2[i] + controlSlew2));
        prevSlew3 = prevSlew3 + (in3[i] - prevSlew3) * fastVoltToSlewAlpha(CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, slewCv3[i] + controlSlew3));

        OUT_PORT_OUT0(slew)[i] = prevSlew0;
        printf("%f\n", prevSlew0);
        OUT_PORT_OUT1(slew)[i] = prevSlew1;
        OUT_PORT_OUT2(slew)[i] = prevSlew2;
        OUT_PORT_OUT3(slew)[i] = prevSlew3;
    }

    slew->prevSlew[0] = prevSlew0;
    slew->prevSlew[1] = prevSlew1;
    slew->prevSlew[2] = prevSlew2;
    slew->prevSlew[3] = prevSlew3;
}


static void pushCurrToPrev(void * modPtr)
{
  Slew * mi = (Slew*)modPtr;
  memcpy(mi->outputPortsPrev, mi->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * SLEW_OUTCOUNT);
  memcpy(mi->outputMIDIPortsPrev, mi->outputMIDIPortsCurr, sizeof(MIDIData) * SLEW_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE);
  CONTROL_PUSH_TO_PREV(mi);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port < SLEW_OUTCOUNT) return PREV_PORT_ADDR(modPtr, port);
  else if ((port - SLEW_OUTCOUNT) < SLEW_MIDI_OUTCOUNT) return PREV_MIDI_PORT_ADDR(modPtr, port - SLEW_OUTCOUNT);

  return NULL;
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
    if (port < SLEW_INCOUNT) {return IN_PORT_ADDR(modPtr, port);}
    else if ((port - SLEW_INCOUNT) < SLEW_MIDI_INCOUNT) return IN_MIDI_PORT_ADDR(modPtr, port - SLEW_INCOUNT);

  return NULL;
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < SLEW_INCOUNT) return ModulePortType_VoltStream;
  else if ((port - SLEW_INCOUNT) < SLEW_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < SLEW_OUTCOUNT) return ModulePortType_VoltStream;
  else if ((port - SLEW_OUTCOUNT) < SLEW_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < SLEW_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}

static U4 getInCount(void * modPtr)
{
  return SLEW_INCOUNT + SLEW_MIDI_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return SLEW_OUTCOUNT + SLEW_MIDI_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return SLEW_CONTROLCOUNT;
}


static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
    if (id < SLEW_CONTROLCOUNT)
  {
    Volt v = *(Volt*)val;
    switch (id)
    {

        case SLEW_CONTROL_SLEW0:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case SLEW_CONTROL_SLEW1:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case SLEW_CONTROL_SLEW2:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case SLEW_CONTROL_SLEW3:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    

      default:
        break;
    }

    ((Slew*)modPtr)->controlsCurr[id] = v;
  }
}


static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id < SLEW_CONTROLCOUNT) *(Volt*)ret = ((Slew*)modPtr)->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
    Slew * mi = (Slew*)modPtr;
  if (port < SLEW_INCOUNT) mi->inputPorts[port] = (R4*)readAddr;
  else if ((port - SLEW_INCOUNT) < SLEW_MIDI_INCOUNT) mi->inputMIDIPorts[port - SLEW_INCOUNT] = (MIDIData*)readAddr;
}

static inline float fastVoltToSlewAlpha(float v)
{
  v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
  float idx = (v / (VOLTSTD_MOD_CV_MAX - VOLTSTD_MOD_CV_ZERO)) * (SLEW_VOLT_TO_ALPHA_TABLE_SIZE - 1);
  int i = (int)idx;
  float frac = idx - i;
  float ret = voltToAlphaTable[i] * (1.0f - frac)
       + voltToAlphaTable[i + 1] * frac;
  return ret;
}

static void initTables()
{
    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
        maxVoltTable[i] = VOLTSTD_MOD_CV_MAX;
        zeroVoltTable[i] = VOLTSTD_MOD_CV_ZERO;
    }

    float inc = (VOLTSTD_MOD_CV_MAX - VOLTSTD_MOD_CV_ZERO) / (float)SLEW_VOLT_TO_ALPHA_TABLE_SIZE;
    float v = VOLTSTD_MOD_CV_ZERO;
    for (int i = 0; i < SLEW_VOLT_TO_ALPHA_TABLE_SIZE; i++)
    {
        voltToAlphaTable[i] = 0.000001f * pow(3.9810717f, v);
        v += inc;
    }
}