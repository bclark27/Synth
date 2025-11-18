#include "QuantizeCv.h"
#include "VoltUtils.h"

#define MAX_SCALE_SIZE 12

#define IN_PORT_ADDR(mod, port)		(((QuantizeCv*)(mod))->inputPorts[port]);
#define PREV_PORT_ADDR(mod, port)	(((QuantizeCv*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)	(((QuantizeCv*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_MIDI_PORT_ADDR(mod, port)		(((QuantizeCv*)(mod))->inputMIDIPorts[port])
#define PREV_MIDI_PORT_ADDR(mod, port)	(((QuantizeCv*)(mod))->outputMIDIPortsPrev + MIDI_STREAM_BUFFER_SIZE * (port))
#define CURR_MIDI_PORT_ADDR(mod, port)	(((QuantizeCv*)(mod))->outputMIDIPortsCurr + MIDI_STREAM_BUFFER_SIZE * (port))

#define IN_PORT_IN(quantize)		((quantize)->inputPorts[QUANTIZE_CV_IN_PORT_IN])
#define IN_PORT_SCALE(quantize)		((quantize)->inputPorts[QUANTIZE_CV_IN_PORT_SCALE])
#define IN_PORT_TRIGGER(quantize)		((quantize)->inputPorts[QUANTIZE_CV_IN_PORT_TRIGGER])
#define IN_PORT_TRANSPOSE(quantize)		((quantize)->inputPorts[QUANTIZE_CV_IN_PORT_TRANSPOSE])
#define OUT_PORT_OUT(quantize)		(CURR_PORT_ADDR(quantize, QUANTIZE_CV_OUT_PORT_OUT))
#define OUT_PORT_TRIGGER(quantize)		(CURR_PORT_ADDR(quantize, QUANTIZE_CV_OUT_PORT_TRIGGER))

#define GET_CONTROL_CURR_SCALE(quantize)	((quantize)->controlsCurr[QUANTIZE_CV_CONTROL_SCALE])
#define GET_CONTROL_PREV_SCALE(quantize)	((quantize)->controlsPrev[QUANTIZE_CV_CONTROL_SCALE])
#define GET_CONTROL_CURR_TRANSPOSE(quantize)	((quantize)->controlsCurr[QUANTIZE_CV_CONTROL_TRANSPOSE])
#define GET_CONTROL_PREV_TRANSPOSE(quantize)	((quantize)->controlsPrev[QUANTIZE_CV_CONTROL_TRANSPOSE])

#define SET_CONTROL_CURR_SCALE(quantize, v)	((quantize)->controlsCurr[QUANTIZE_CV_CONTROL_SCALE] = (v))
#define SET_CONTROL_PREV_SCALE(quantize, v)	((quantize)->controlsPrev[QUANTIZE_CV_CONTROL_SCALE] = (v))
#define SET_CONTROL_CURR_TRANSPOSE(quantize, v)	((quantize)->controlsCurr[QUANTIZE_CV_CONTROL_TRANSPOSE] = (v))
#define SET_CONTROL_PREV_TRANSPOSE(quantize, v)	((quantize)->controlsPrev[QUANTIZE_CV_CONTROL_TRANSPOSE] = (v))


#define CONTROL_PUSH_TO_PREV(quantize)         for (U4 i = 0; i < QUANTIZE_CV_CONTROLCOUNT; i++) {(quantize)->controlsPrev[i] = (quantize)->controlsCurr[i];}

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
static inline R4 quantizeSignal(R4 signal, QuantizeScale scale);



//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool tableInitDone = false;
static R4 zeroVoltTable[MODULE_BUFFER_SIZE];

static char * inPortNames[QUANTIZE_CV_INCOUNT] = {
	"In",
	"Scale",
	"Trigger",
	"Transpose",
};
static char * outPortNames[QUANTIZE_CV_OUTCOUNT] = {
	"Out",
	"Trigger",
};
static char * controlNames[QUANTIZE_CV_CONTROLCOUNT] = {
	"Scale",
	"Transpose",
};

static int scaleTableLengths[QuantizeScale_Count] = {
  1,
  12,
  7,
  7,
  5,
  5,
  5,
  7,
};
static int scaleTables[QuantizeScale_Count][MAX_SCALE_SIZE] = {
  {0},
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
  {0, 2, 4, 5, 7, 9, 11},
  {0, 2, 3, 5, 7, 8, 10},
  {0, 2, 4, 7, 9},
  {0, 3, 5, 7, 10},
  {0, 1, 5, 7, 10},
  {0, 2, 3, 5, 7, 9, 10},
};
static R4 scaleTableVoltCutoffs[QuantizeScale_Count][MAX_SCALE_SIZE];
static R4 scaleTableVoltValues[QuantizeScale_Count][MAX_SCALE_SIZE];

static const R4 quantizeScaleMaxVolt = QuantizeScale_Count - 0.5f;

static Module vtable = {
  .type = ModuleType_QuantizeCv,
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

#define DEFAULT_CONTROL_SCALE    VOLTSTD_MOD_CV_ZERO
#define DEFAULT_CONTROL_TRANSPOSE    VOLTSTD_MOD_CV_ZERO

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


void QuantizeCv_initInPlace(QuantizeCv* quantize, char* name)
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
	SET_CONTROL_CURR_TRANSPOSE(quantize, DEFAULT_CONTROL_TRANSPOSE);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(quantize);

  quantize->lastQuantize = -999;
  quantize->lastTriggerInputVolt = VOLTSTD_MOD_CV_ZERO;
}



Module * QuantizeCv_init(char* name)
{
  QuantizeCv * quantize = (QuantizeCv*)calloc(1, sizeof(QuantizeCv));

  QuantizeCv_initInPlace(quantize, name);
  return (Module*)quantize;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_quantize(void * modPtr)
{
  QuantizeCv * quantize = (QuantizeCv *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(quantize);
}

static void updateState(void * modPtr)
{
  QuantizeCv * quantize = (QuantizeCv*)modPtr;

  R4* cvInSig = IN_PORT_IN(quantize) ? IN_PORT_IN(quantize) : zeroVoltTable;
  R4* cvInScale = IN_PORT_SCALE(quantize) ? IN_PORT_SCALE(quantize) : zeroVoltTable;
  R4* cvInTranspose = IN_PORT_TRANSPOSE(quantize) ? IN_PORT_TRANSPOSE(quantize) : zeroVoltTable;
  R4* cvInTrigger = IN_PORT_TRIGGER(quantize);

  R4 controlScale = GET_CONTROL_CURR_SCALE(quantize);
  R4 controlTranspose = GET_CONTROL_CURR_TRANSPOSE(quantize);

  if (!cvInTrigger)
  {
    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
      R4 transposedInputSignal = cvInSig[i] + controlTranspose + cvInTranspose[i];
      QuantizeScale scale = (QuantizeScale)(int)(CLAMPF(0, quantizeScaleMaxVolt, controlScale + cvInScale[i]));
      
      R4 quantSig = quantizeSignal(transposedInputSignal, scale);
      
      OUT_PORT_TRIGGER(quantize)[i] = (int)(quantSig == quantize->lastQuantize) * VOLTSTD_GATE_HIGH;
      OUT_PORT_OUT(quantize)[i] = quantSig;
      quantize->lastQuantize = quantSig;
    }

    quantize->lastTriggerInputVolt = VOLTSTD_MOD_CV_ZERO;
  }
  else
  {
    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
      if (quantize->lastTriggerInputVolt < VOLTSTD_GATE_HIGH_THRESH && cvInTrigger[i] >= VOLTSTD_GATE_HIGH_THRESH)
      {
        R4 transposedInputSignal = cvInSig[i] + controlTranspose + cvInTranspose[i];
        QuantizeScale scale = (QuantizeScale)(int)(CLAMPF(0, quantizeScaleMaxVolt, controlScale + cvInScale[i]));
  
        R4 quantSig = quantizeSignal(transposedInputSignal, scale);
  
        OUT_PORT_TRIGGER(quantize)[i] = (int)(quantSig == quantize->lastQuantize) * VOLTSTD_GATE_HIGH;
        OUT_PORT_OUT(quantize)[i] = quantSig;
        quantize->lastQuantize = quantSig;
      }
      else
      {
        OUT_PORT_TRIGGER(quantize)[i] = VOLTSTD_MOD_CV_ZERO;
        OUT_PORT_OUT(quantize)[i] = quantize->lastQuantize;
      }

      quantize->lastTriggerInputVolt = cvInTrigger[i];
    }
  }
}


static void pushCurrToPrev(void * modPtr)
{
  QuantizeCv * mi = (QuantizeCv*)modPtr;
  memcpy(mi->outputPortsPrev, mi->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * QUANTIZE_CV_OUTCOUNT);
  memcpy(mi->outputMIDIPortsPrev, mi->outputMIDIPortsCurr, sizeof(MIDIData) * QUANTIZE_CV_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE);
  CONTROL_PUSH_TO_PREV(mi);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port < QUANTIZE_CV_OUTCOUNT) return PREV_PORT_ADDR(modPtr, port);
  else if ((port - QUANTIZE_CV_OUTCOUNT) < QUANTIZE_CV_MIDI_OUTCOUNT) return PREV_MIDI_PORT_ADDR(modPtr, port - QUANTIZE_CV_OUTCOUNT);

  return NULL;
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
    if (port < QUANTIZE_CV_INCOUNT) {return IN_PORT_ADDR(modPtr, port);}
    else if ((port - QUANTIZE_CV_INCOUNT) < QUANTIZE_CV_MIDI_INCOUNT) return IN_MIDI_PORT_ADDR(modPtr, port - QUANTIZE_CV_INCOUNT);

  return NULL;
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < QUANTIZE_CV_INCOUNT) return ModulePortType_VoltStream;
  else if ((port - QUANTIZE_CV_INCOUNT) < QUANTIZE_CV_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < QUANTIZE_CV_OUTCOUNT) return ModulePortType_VoltStream;
  else if ((port - QUANTIZE_CV_OUTCOUNT) < QUANTIZE_CV_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < QUANTIZE_CV_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}

static U4 getInCount(void * modPtr)
{
  return QUANTIZE_CV_INCOUNT + QUANTIZE_CV_MIDI_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return QUANTIZE_CV_OUTCOUNT + QUANTIZE_CV_MIDI_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return QUANTIZE_CV_CONTROLCOUNT;
}


static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
    if (id < QUANTIZE_CV_CONTROLCOUNT)
  {
    Volt v = *(Volt*)val;
    switch (id)
    {

        case QUANTIZE_CV_CONTROL_SCALE:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    
        case QUANTIZE_CV_CONTROL_TRANSPOSE:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    

      default:
        break;
    }

    ((QuantizeCv*)modPtr)->controlsCurr[id] = v;
  }
}


static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id < QUANTIZE_CV_CONTROLCOUNT) *(Volt*)ret = ((QuantizeCv*)modPtr)->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
  QuantizeCv * mi = (QuantizeCv*)modPtr;
  if (port < QUANTIZE_CV_INCOUNT) mi->inputPorts[port] = (R4*)readAddr;
  else if ((port - QUANTIZE_CV_INCOUNT) < QUANTIZE_CV_MIDI_INCOUNT) mi->inputMIDIPorts[port - QUANTIZE_CV_INCOUNT] = (MIDIData*)readAddr;
}

static void initTables()
{
  for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
  {
    zeroVoltTable[i] = VOLTSTD_MOD_CV_ZERO;
  }

  for (int tableIdx = 0; tableIdx < QuantizeScale_Count; tableIdx++)
  {
    int len = scaleTableLengths[tableIdx];
    int* table = scaleTables[tableIdx];
    R4* cutoffTable = scaleTableVoltCutoffs[tableIdx];
    R4* voltValueTable = scaleTableVoltValues[tableIdx];
    for (int noteIdx = 0; noteIdx < len; noteIdx++)
    {
      R4 thisNoteVolt = table[noteIdx] / 12.f;
      voltValueTable[noteIdx] = thisNoteVolt;
      R4 nextNoteVolt = 1.f;

      if (noteIdx != len - 1)
      {
        nextNoteVolt = table[noteIdx + 1] / 12.f;
      }

      cutoffTable[noteIdx] = (thisNoteVolt + nextNoteVolt) / 2.f;
    }
  }
}

static inline R4 quantizeSignal(R4 signal, QuantizeScale scale)
{
  int scaleLen = scaleTableLengths[scale];
  R4* scaleCutoffTable = scaleTableVoltCutoffs[scale];
  R4* scaleVoltTable = scaleTableVoltValues[scale];

  R4 octaveVolt = floorf(signal);
  R4 noteVolt = signal - octaveVolt;

  for (int i = 0; i < scaleLen; i++)
  {
    if (noteVolt < scaleCutoffTable[i])
    {
      return scaleVoltTable[i] + octaveVolt;
    }
  }

  return octaveVolt + 1.f;
}