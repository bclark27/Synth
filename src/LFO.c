
#include "LFO.h"
#include "VoltUtils.h"

#define IN_PORT_ADDR(mod, port)		(((LFO*)(mod))->inputPorts[port]);
#define PREV_PORT_ADDR(mod, port)	(((LFO*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)	(((LFO*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_MIDI_PORT_ADDR(mod, port)		(((LFO*)(mod))->inputMIDIPorts[port])
#define PREV_MIDI_PORT_ADDR(mod, port)	(((LFO*)(mod))->outputMIDIPortsPrev + MIDI_STREAM_BUFFER_SIZE * (port))
#define CURR_MIDI_PORT_ADDR(mod, port)	(((LFO*)(mod))->outputMIDIPortsCurr + MIDI_STREAM_BUFFER_SIZE * (port))

#define IN_PORT_FREQ(lfo)		((lfo)->inputPorts[LFO_IN_PORT_FREQ])
#define IN_PORT_CLK(lfo)		((lfo)->inputPorts[LFO_IN_PORT_CLK])
#define IN_PORT_PW(lfo)		((lfo)->inputPorts[LFO_IN_PORT_PW])
#define OUT_PORT_SIG(lfo)		(CURR_PORT_ADDR(lfo, LFO_OUT_PORT_SIG))
#define OUT_PORT_CLK(lfo)		(CURR_PORT_ADDR(lfo, LFO_OUT_PORT_CLK))

#define IN_MIDI_PORT_MIDI_asdasd(lfo)		((lfo)->inputPorts[LFO_IN_PORT_MIDI_asdasd])
#define OUT_MIDI_PORT_MIDI_okok(lfo)		(CURR_MIDI_PORT_ADDR(lfo, LFO_OUT_PORT_MIDI_okok))
#define GET_CONTROL_CURR_FREQ(lfo)	((lfo)->controlsCurr[LFO_CONTROL_FREQ])
#define GET_CONTROL_PREV_FREQ(lfo)	((lfo)->controlsPrev[LFO_CONTROL_FREQ])
#define GET_CONTROL_CURR_PW(lfo)	((lfo)->controlsCurr[LFO_CONTROL_PW])
#define GET_CONTROL_PREV_PW(lfo)	((lfo)->controlsPrev[LFO_CONTROL_PW])
#define GET_CONTROL_CURR_MIN(lfo)	((lfo)->controlsCurr[LFO_CONTROL_MIN])
#define GET_CONTROL_PREV_MIN(lfo)	((lfo)->controlsPrev[LFO_CONTROL_MIN])
#define GET_CONTROL_CURR_MAX(lfo)	((lfo)->controlsCurr[LFO_CONTROL_MAX])
#define GET_CONTROL_PREV_MAX(lfo)	((lfo)->controlsPrev[LFO_CONTROL_MAX])
#define GET_CONTROL_CURR_WAVE(lfo)	((lfo)->controlsCurr[LFO_CONTROL_WAVE])
#define GET_CONTROL_PREV_WAVE(lfo)	((lfo)->controlsPrev[LFO_CONTROL_WAVE])

#define SET_CONTROL_CURR_FREQ(lfo, v)	((lfo)->controlsCurr[LFO_CONTROL_FREQ] = (v))
#define SET_CONTROL_PREV_FREQ(lfo, v)	((lfo)->controlsPrev[LFO_CONTROL_FREQ] = (v))
#define SET_CONTROL_CURR_PW(lfo, v)	((lfo)->controlsCurr[LFO_CONTROL_PW] = (v))
#define SET_CONTROL_PREV_PW(lfo, v)	((lfo)->controlsPrev[LFO_CONTROL_PW] = (v))
#define SET_CONTROL_CURR_MIN(lfo, v)	((lfo)->controlsCurr[LFO_CONTROL_MIN] = (v))
#define SET_CONTROL_PREV_MIN(lfo, v)	((lfo)->controlsPrev[LFO_CONTROL_MIN] = (v))
#define SET_CONTROL_CURR_MAX(lfo, v)	((lfo)->controlsCurr[LFO_CONTROL_MAX] = (v))
#define SET_CONTROL_PREV_MAX(lfo, v)	((lfo)->controlsPrev[LFO_CONTROL_MAX] = (v))
#define SET_CONTROL_CURR_WAVE(lfo, v)	((lfo)->controlsCurr[LFO_CONTROL_WAVE] = (v))
#define SET_CONTROL_PREV_WAVE(lfo, v)	((lfo)->controlsPrev[LFO_CONTROL_WAVE] = (v))


#define CONTROL_PUSH_TO_PREV(lfo)         for (U4 i = 0; i < LFO_CONTROLCOUNT; i++) {(lfo)->controlsPrev[i] = (lfo)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_lfo(void * modPtr);
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

static char * inPortNames[LFO_INCOUNT] = {
	"FREQ",
	"CLK",
	"PW",
};
static char * outPortNames[LFO_OUTCOUNT] = {
	"SIG",
	"CLK",
};
static char * controlNames[LFO_CONTROLCOUNT] = {
	"FREQ",
	"PW",
	"MIN",
	"MAX",
	"WAVE",
};

static Module vtable = {
  .type = ModuleType_LFO,
  .freeModule = free_lfo,
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

#define DEFAULT_CONTROL_FREQ	0
#define DEFAULT_CONTROL_PW	0
#define DEFAULT_CONTROL_MIN	0
#define DEFAULT_CONTROL_MAX	0
#define DEFAULT_CONTROL_WAVE	0

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


void LFO_initInPlace(LFO* lfo, char* name)
{
  if (!tableInitDone)
  {
    initTables();
    tableInitDone = true;
  }

  // set vtable
  lfo->module = vtable;

  // set name of module
  lfo->module.name = name;
  
  // set all control values

	SET_CONTROL_CURR_FREQ(lfo, DEFAULT_CONTROL_FREQ);
	SET_CONTROL_CURR_PW(lfo, DEFAULT_CONTROL_PW);
	SET_CONTROL_CURR_MIN(lfo, DEFAULT_CONTROL_MIN);
	SET_CONTROL_CURR_MAX(lfo, DEFAULT_CONTROL_MAX);
	SET_CONTROL_CURR_WAVE(lfo, DEFAULT_CONTROL_WAVE);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(lfo);

}



Module * LFO_init(char* name)
{
  LFO * lfo = (LFO*)calloc(1, sizeof(LFO));

  LFO_initInPlace(lfo, name);
  return (Module*)lfo;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_lfo(void * modPtr)
{
  LFO * lfo = (LFO *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(lfo);
}

static void updateState(void * modPtr)
{
    LFO * lfo = (LFO*)modPtr;
}


static void pushCurrToPrev(void * modPtr)
{
  LFO * mi = (LFO*)modPtr;
  memcpy(mi->outputPortsPrev, mi->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * LFO_OUTCOUNT);
  memcpy(mi->outputMIDIPortsPrev, mi->outputMIDIPortsCurr, sizeof(MIDIData) * LFO_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE);
  CONTROL_PUSH_TO_PREV(mi);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port < LFO_OUTCOUNT) return PREV_PORT_ADDR(modPtr, port);
  else if ((port - LFO_OUTCOUNT) < LFO_MIDI_OUTCOUNT) return PREV_MIDI_PORT_ADDR(modPtr, port - LFO_OUTCOUNT);

  return NULL;
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
    if (port < LFO_INCOUNT) {return IN_PORT_ADDR(modPtr, port);}
    else if ((port - LFO_INCOUNT) < LFO_MIDI_INCOUNT) return IN_MIDI_PORT_ADDR(modPtr, port - LFO_INCOUNT);

  return NULL;
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < LFO_INCOUNT) return ModulePortType_VoltStream;
  else if ((port - LFO_INCOUNT) < LFO_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < LFO_OUTCOUNT) return ModulePortType_VoltStream;
  else if ((port - LFO_OUTCOUNT) < LFO_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < LFO_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}

static U4 getInCount(void * modPtr)
{
  return LFO_INCOUNT + LFO_MIDI_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return LFO_OUTCOUNT + LFO_MIDI_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return LFO_CONTROLCOUNT;
}


static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
    if (id < LFO_CONTROLCOUNT)
  {
    Volt v = *(Volt*)val;
    switch (id)
    {

        case LFO_CONTROL_FREQ:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case LFO_CONTROL_PW:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case LFO_CONTROL_MIN:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case LFO_CONTROL_MAX:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case LFO_CONTROL_WAVE:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    

      default:
        break;
    }

    ((LFO*)modPtr)->controlsCurr[id] = v;
  }
}


static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id < LFO_CONTROLCOUNT) *(Volt*)ret = ((LFO*)modPtr)->controlsCurr[id];
}
static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
    LFO * mi = (LFO*)modPtr;
  if (port < LFO_INCOUNT) mi->inputPorts[port] = (R4*)readAddr;
  else if ((port - LFO_INCOUNT) < LFO_MIDI_INCOUNT) mi->inputMIDIPorts[port - LFO_INCOUNT] = (MIDIData*)readAddr;
}
static void initTables()
{
    
}

