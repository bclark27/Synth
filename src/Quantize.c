#include "Quantize.h"
#include "VoltUtils.h"

#define MAX_SCALE_SIZE 12

#define IN_PORT_ADDR(mod, port)		(((Quantize*)(mod))->inputPorts[port]);
#define PREV_PORT_ADDR(mod, port)	(((Quantize*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)	(((Quantize*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_MIDI_PORT_ADDR(mod, port)		(((Quantize*)(mod))->inputMIDIPorts[port])
#define PREV_MIDI_PORT_ADDR(mod, port)	(((Quantize*)(mod))->outputMIDIPortsPrev + MIDI_STREAM_BUFFER_SIZE * (port))
#define CURR_MIDI_PORT_ADDR(mod, port)	(((Quantize*)(mod))->outputMIDIPortsCurr + MIDI_STREAM_BUFFER_SIZE * (port))

#define IN_PORT_IN(quantize)		((quantize)->inputPorts[QUANTIZE_IN_PORT_IN])
#define IN_PORT_SCALE(quantize)		((quantize)->inputPorts[QUANTIZE_IN_PORT_SCALE])
#define IN_PORT_TRIGGER(quantize)		((quantize)->inputPorts[QUANTIZE_IN_PORT_TRIGGER])
#define IN_PORT_TRANSPOSE(quantize)		((quantize)->inputPorts[QUANTIZE_IN_PORT_TRANSPOSE])
#define OUT_PORT_OUT(quantize)		(CURR_PORT_ADDR(quantize, QUANTIZE_OUT_PORT_OUT))
#define OUT_PORT_TRIGGER(quantize)		(CURR_PORT_ADDR(quantize, QUANTIZE_OUT_PORT_TRIGGER))

#define GET_CONTROL_CURR_SCALE(quantize)	((quantize)->controlsCurr[QUANTIZE_CONTROL_SCALE])
#define GET_CONTROL_PREV_SCALE(quantize)	((quantize)->controlsPrev[QUANTIZE_CONTROL_SCALE])
#define GET_CONTROL_CURR_TRANSPOSE(quantize)	((quantize)->controlsCurr[QUANTIZE_CONTROL_TRANSPOSE])
#define GET_CONTROL_PREV_TRANSPOSE(quantize)	((quantize)->controlsPrev[QUANTIZE_CONTROL_TRANSPOSE])

#define SET_CONTROL_CURR_SCALE(quantize, v)	((quantize)->controlsCurr[QUANTIZE_CONTROL_SCALE] = (v))
#define SET_CONTROL_PREV_SCALE(quantize, v)	((quantize)->controlsPrev[QUANTIZE_CONTROL_SCALE] = (v))
#define SET_CONTROL_CURR_TRANSPOSE(quantize, v)	((quantize)->controlsCurr[QUANTIZE_CONTROL_TRANSPOSE] = (v))
#define SET_CONTROL_PREV_TRANSPOSE(quantize, v)	((quantize)->controlsPrev[QUANTIZE_CONTROL_TRANSPOSE] = (v))


#define CONTROL_PUSH_TO_PREV(quantize)         for (U4 i = 0; i < QUANTIZE_CONTROLCOUNT; i++) {(quantize)->controlsPrev[i] = (quantize)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_quantize(void * modPtr);
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



//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool tableInitDone = false;

static char * inPortNames[QUANTIZE_INCOUNT] = {
	"In",
	"Scale",
	"Trigger",
	"Transpose",
};
static char * outPortNames[QUANTIZE_OUTCOUNT] = {
	"Out",
	"Trigger",
};
static char * controlNames[QUANTIZE_CONTROLCOUNT] = {
	"Scale",
	"Transpose",
};

static int scaleTableLengths[QuantizeScale_Count] = {
    12,
    7,
    7,
    5,
    5,
    5,
    7,
};
static int scaleTables[QuantizeScale_Count][MAX_SCALE_SIZE] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    {0, 2, 4, 5, 7, 9, 11},
    {0, 2, 3, 5, 7, 8, 10},
    {0, 2, 4, 7, 9},
    {0, 3, 5, 7, 10},
    {0, 1, 5, 7, 10},
    {0, 2, 3, 5, 7, 9, 10},
};

static Module vtable = {
  .type = ModuleType_Quantize,
  .freeModule = free_quantize,
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

#define DEFAULT_CONTROL_SCALE    0

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


void Quantize_initInPlace(Quantize* quantize, char* name)
{
  if (!tableInitDone)
  {
    initTables();
    tableInitDone = true;
  }

  // set vtable
  quantize->module = vtable;

  // set name of module
  quantize->module.name = name;
  
  // set all control values

	SET_CONTROL_CURR_SCALE(quantize, DEFAULT_CONTROL_SCALE);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(quantize);

}



Module * Quantize_init(char* name)
{
  Quantize * quantize = (Quantize*)calloc(1, sizeof(Quantize));

  Quantize_initInPlace(quantize, name);
  return (Module*)quantize;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_quantize(void * modPtr)
{
  Quantize * quantize = (Quantize *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(quantize);
}

static void updateState(void * modPtr)
{
    Quantize * quantize = (Quantize*)modPtr;
}


static void pushCurrToPrev(void * modPtr)
{
  Quantize * mi = (Quantize*)modPtr;
  memcpy(mi->outputPortsPrev, mi->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * QUANTIZE_OUTCOUNT);
  memcpy(mi->outputMIDIPortsPrev, mi->outputMIDIPortsCurr, sizeof(MIDIData) * QUANTIZE_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE);
  CONTROL_PUSH_TO_PREV(mi);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port < QUANTIZE_OUTCOUNT) return PREV_PORT_ADDR(modPtr, port);
  else if ((port - QUANTIZE_OUTCOUNT) < QUANTIZE_MIDI_OUTCOUNT) return PREV_MIDI_PORT_ADDR(modPtr, port - QUANTIZE_OUTCOUNT);

  return NULL;
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
    if (port < QUANTIZE_INCOUNT) {return IN_PORT_ADDR(modPtr, port);}
    else if ((port - QUANTIZE_INCOUNT) < QUANTIZE_MIDI_INCOUNT) return IN_MIDI_PORT_ADDR(modPtr, port - QUANTIZE_INCOUNT);

  return NULL;
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < QUANTIZE_INCOUNT) return ModulePortType_VoltStream;
  else if ((port - QUANTIZE_INCOUNT) < QUANTIZE_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < QUANTIZE_OUTCOUNT) return ModulePortType_VoltStream;
  else if ((port - QUANTIZE_OUTCOUNT) < QUANTIZE_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < QUANTIZE_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}

static U4 getInCount(void * modPtr)
{
  return QUANTIZE_INCOUNT + QUANTIZE_MIDI_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return QUANTIZE_OUTCOUNT + QUANTIZE_MIDI_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return QUANTIZE_CONTROLCOUNT;
}


static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
    if (id < QUANTIZE_CONTROLCOUNT)
  {
    Volt v = *(Volt*)val;
    switch (id)
    {

        case QUANTIZE_CONTROL_SCALE:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    
        case QUANTIZE_CONTROL_TRANSPOSE:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    

      default:
        break;
    }

    ((Quantize*)modPtr)->controlsCurr[id] = v;
  }
}


static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id < QUANTIZE_CONTROLCOUNT) *(Volt*)ret = ((Quantize*)modPtr)->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
    Quantize * mi = (Quantize*)modPtr;
  if (port < QUANTIZE_INCOUNT) mi->inputPorts[port] = (R4*)readAddr;
  else if ((port - QUANTIZE_INCOUNT) < QUANTIZE_MIDI_INCOUNT) mi->inputMIDIPorts[port - QUANTIZE_INCOUNT] = (MIDIData*)readAddr;
}

static void initTables()
{
    
}