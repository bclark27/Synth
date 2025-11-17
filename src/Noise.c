#include "Noise.h"
#include "VoltUtils.h"

#define IN_PORT_ADDR(mod, port)		(((Noise*)(mod))->inputPorts[port]);
#define PREV_PORT_ADDR(mod, port)	(((Noise*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)	(((Noise*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_MIDI_PORT_ADDR(mod, port)(((Noise*)(mod))->inputMIDIPorts[port])
#define PREV_MIDI_PORT_ADDR(mod, port)	(((Noise*)(mod))->outputMIDIPortsPrev + MIDI_STREAM_BUFFER_SIZE * (port))
#define CURR_MIDI_PORT_ADDR(mod, port)	(((Noise*)(mod))->outputMIDIPortsCurr + MIDI_STREAM_BUFFER_SIZE * (port))

#define IN_PORT_CLK(noise)		((noise)->inputPorts[NOISE_IN_PORT_CLK])
#define OUT_PORT_WHITE(noise)		(CURR_PORT_ADDR(noise, NOISE_OUT_PORT_WHITE))
#define OUT_PORT_COLORED(noise)		(CURR_PORT_ADDR(noise, NOISE_OUT_PORT_COLORED))
#define OUT_PORT_RANDOM(noise)		(CURR_PORT_ADDR(noise, NOISE_OUT_PORT_RANDOM))

#define GET_CONTROL_CURR_NOISE_LEVEL(noise)	((noise)->controlsCurr[NOISE_CONTROL_NOISE_LEVEL])
#define GET_CONTROL_PREV_NOISE_LEVEL(noise)	((noise)->controlsPrev[NOISE_CONTROL_NOISE_LEVEL])
#define GET_CONTROL_CURR_COLORED_LOW(noise)	((noise)->controlsCurr[NOISE_CONTROL_COLORED_LOW])
#define GET_CONTROL_PREV_COLORED_LOW(noise)	((noise)->controlsPrev[NOISE_CONTROL_COLORED_LOW])
#define GET_CONTROL_CURR_COLORED_HIGH(noise)	((noise)->controlsCurr[NOISE_CONTROL_COLORED_HIGH])
#define GET_CONTROL_PREV_COLORED_HIGH(noise)	((noise)->controlsPrev[NOISE_CONTROL_COLORED_HIGH])
#define GET_CONTROL_CURR_RANDOM_LEVEL(noise)	((noise)->controlsCurr[NOISE_CONTROL_RANDOM_LEVEL])
#define GET_CONTROL_PREV_RANDOM_LEVEL(noise)	((noise)->controlsPrev[NOISE_CONTROL_RANDOM_LEVEL])

#define SET_CONTROL_CURR_NOISE_LEVEL(noise, v)	((noise)->controlsCurr[NOISE_CONTROL_NOISE_LEVEL] = (v))
#define SET_CONTROL_PREV_NOISE_LEVEL(noise, v)	((noise)->controlsPrev[NOISE_CONTROL_NOISE_LEVEL] = (v))
#define SET_CONTROL_CURR_COLORED_LOW(noise, v)	((noise)->controlsCurr[NOISE_CONTROL_COLORED_LOW] = (v))
#define SET_CONTROL_PREV_COLORED_LOW(noise, v)	((noise)->controlsPrev[NOISE_CONTROL_COLORED_LOW] = (v))
#define SET_CONTROL_CURR_COLORED_HIGH(noise, v)	((noise)->controlsCurr[NOISE_CONTROL_COLORED_HIGH] = (v))
#define SET_CONTROL_PREV_COLORED_HIGH(noise, v)	((noise)->controlsPrev[NOISE_CONTROL_COLORED_HIGH] = (v))
#define SET_CONTROL_CURR_RANDOM_LEVEL(noise, v)	((noise)->controlsCurr[NOISE_CONTROL_RANDOM_LEVEL] = (v))
#define SET_CONTROL_PREV_RANDOM_LEVEL(noise, v)	((noise)->controlsPrev[NOISE_CONTROL_RANDOM_LEVEL] = (v))


#define CONTROL_PUSH_TO_PREV(noise)         for (U4 i = 0; i < NOISE_CONTROLCOUNT; i++) {(noise)->controlsPrev[i] = (noise)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_noise(void * modPtr);
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
static inline float fastNoiseLCG(uint32_t* state);



//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool tableInitDone = false;

static char * inPortNames[NOISE_INCOUNT] = {
	"Clock",
};
static char * outPortNames[NOISE_OUTCOUNT] = {
	"White",
	"Colored",
	"Random",
};
static char * controlNames[NOISE_CONTROLCOUNT] = {
	"NoiseLevel",
	"ColoredLow",
	"ColoredHigh",
	"RandomLevel",
};

static Module vtable = {
  .type = ModuleType_Noise,
  .freeModule = free_noise,
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

#define DEFAULT_CONTROL_NOISE_LEVEL 5
#define DEFAULT_CONTROL_COLORED_LOW 0
#define DEFAULT_CONTROL_COLORED_HIGH 0
#define DEFAULT_CONTROL_RANDOM_LEVEL 2

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


void Noise_initInPlace(Noise* noise, char* name)
{
  if (!tableInitDone)
  {
    initTables();
    tableInitDone = true;
  }

  // set vtable
  noise->module = vtable;

  // set name of module
  noise->module.name = name;
  
  // set all control values

	SET_CONTROL_CURR_NOISE_LEVEL(noise, DEFAULT_CONTROL_NOISE_LEVEL);
	SET_CONTROL_CURR_COLORED_LOW(noise, DEFAULT_CONTROL_COLORED_LOW);
	SET_CONTROL_CURR_COLORED_HIGH(noise, DEFAULT_CONTROL_COLORED_HIGH);
	SET_CONTROL_CURR_RANDOM_LEVEL(noise, DEFAULT_CONTROL_RANDOM_LEVEL);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(noise);

  noise->noiseState = 238764;
  noise->currentClockSampleLength = SAMPLE_RATE;
}



Module * Noise_init(char* name)
{
  Noise * noise = (Noise*)calloc(1, sizeof(Noise));

  Noise_initInPlace(noise, name);
  return (Module*)noise;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_noise(void * modPtr)
{
  Noise * noise = (Noise *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(noise);
}

static void updateState(void * modPtr)
{
    Noise * noise = (Noise*)modPtr;

    float* clockIn = IN_PORT_CLK(noise);

    bool tick[MODULE_BUFFER_SIZE];
    if (clockIn)
    {
        for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
        {
            if (clockIn[i] >= 1.f && noise->lastClockInSampleVoltage < 1.f)
            {
                noise->currentClockSampleLength = noise->currentClockSampleCount;
                noise->currentClockSampleCount = 0;
                tick[i] = 1;
            }
            else
            {
                noise->currentClockSampleCount++;
                tick[i] = 0;
            }
        }
    }
    else
    {
        for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
        {
            if (noise->currentClockSampleCount >= noise->currentClockSampleLength)
            {
                noise->currentClockSampleCount = 0;
                tick[i] = 1;
            }
            else
            {
                noise->currentClockSampleCount++;
                tick[i] = 0;
            }
        }
    }


    float* outWhite   = OUT_PORT_WHITE(noise);
    float* outColored = OUT_PORT_COLORED(noise);
    float* outRandom  = OUT_PORT_RANDOM(noise);

    float noiseLevel  = GET_CONTROL_CURR_NOISE_LEVEL(noise);
    float lowColor    = GET_CONTROL_CURR_COLORED_LOW(noise);
    float highColor   = GET_CONTROL_CURR_COLORED_HIGH(noise);
    float randomLevel = GET_CONTROL_CURR_RANDOM_LEVEL(noise);

    for (int i = 0; i < MODULE_BUFFER_SIZE; i++) 
    {
        float rand = fastNoiseLCG(&noise->noiseState);

        // rand sample hold
        if (tick[i])
        {
            outRandom[i] = rand * randomLevel;
            noise->currentRandomLevel = rand;
        }
        else
        {
            outRandom[i] = noise->currentRandomLevel * randomLevel;
        }

        // WHITE NOISE
        float w = rand * noiseLevel;
        outWhite[i] = w;

        // COLORED NOISE (A-118 behavior: simple tilt EQ)
        float c = w * lowColor + (w * w * w) * highColor;  
        outColored[i] = c;
    }

    
}


static void pushCurrToPrev(void * modPtr)
{
  Noise * mi = (Noise*)modPtr;
  memcpy(mi->outputPortsPrev, mi->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * NOISE_OUTCOUNT);
  memcpy(mi->outputMIDIPortsPrev, mi->outputMIDIPortsCurr, sizeof(MIDIData) * NOISE_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE);
  CONTROL_PUSH_TO_PREV(mi);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port < NOISE_OUTCOUNT) return PREV_PORT_ADDR(modPtr, port);
  else if ((port - NOISE_OUTCOUNT) < NOISE_MIDI_OUTCOUNT) return PREV_MIDI_PORT_ADDR(modPtr, port - NOISE_OUTCOUNT);

  return NULL;
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
    if (port < NOISE_INCOUNT) {return IN_PORT_ADDR(modPtr, port);}
    else if ((port - NOISE_INCOUNT) < NOISE_MIDI_INCOUNT) return IN_MIDI_PORT_ADDR(modPtr, port - NOISE_INCOUNT);

  return NULL;
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < NOISE_INCOUNT) return ModulePortType_VoltStream;
  else if ((port - NOISE_INCOUNT) < NOISE_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < NOISE_OUTCOUNT) return ModulePortType_VoltStream;
  else if ((port - NOISE_OUTCOUNT) < NOISE_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < NOISE_CONTROLCOUNT) return ModulePortType_VoltControl;
  return ModulePortType_None;
}

static U4 getInCount(void * modPtr)
{
  return NOISE_INCOUNT + NOISE_MIDI_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return NOISE_OUTCOUNT + NOISE_MIDI_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return NOISE_CONTROLCOUNT;
}


static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
    if (id < NOISE_CONTROLCOUNT)
  {
    Volt v = *(Volt*)val;
    switch (id)
    {

        case NOISE_CONTROL_NOISE_LEVEL:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case NOISE_CONTROL_COLORED_LOW:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case NOISE_CONTROL_COLORED_HIGH:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case NOISE_CONTROL_RANDOM_LEVEL:
        v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
        break;
    

      default:
        break;
    }

    ((Noise*)modPtr)->controlsCurr[id] = v;
  }
}


static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id < NOISE_CONTROLCOUNT) *(Volt*)ret = ((Noise*)modPtr)->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
    Noise * mi = (Noise*)modPtr;
  if (port < NOISE_INCOUNT) mi->inputPorts[port] = (R4*)readAddr;
  else if ((port - NOISE_INCOUNT) < NOISE_MIDI_INCOUNT) mi->inputMIDIPorts[port - NOISE_INCOUNT] = (MIDIData*)readAddr;
}

static void initTables()
{
    
}

static inline float fastNoiseLCG(uint32_t* state) 
{
    *state = (*state * 1664525u) + 1013904223u;
    return ((*state >> 8) & 0xFFFFFF) * (1.0f / 16777216.0f) * 2.0f - 1.0f;
}
