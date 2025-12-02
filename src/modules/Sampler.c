
#include "Sampler.h"

#include <sndfile.h>
#include <stdlib.h>
#include <stdio.h>

#define IN_PORT_ADDR(mod, port)		(((Sampler*)(mod))->inputPorts[port]);
#define PREV_PORT_ADDR(mod, port)	(((Sampler*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)	(((Sampler*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_MIDI_PORT_ADDR(mod, port)		(((Sampler*)(mod))->inputMIDIPorts[port])
#define PREV_MIDI_PORT_ADDR(mod, port)	(((Sampler*)(mod))->outputMIDIPortsPrev + MIDI_STREAM_BUFFER_SIZE * (port))
#define CURR_MIDI_PORT_ADDR(mod, port)	(((Sampler*)(mod))->outputMIDIPortsCurr + MIDI_STREAM_BUFFER_SIZE * (port))

#define IN_PORT_Pitch(sampler)		((sampler)->inputPorts[SAMPLER_IN_PORT_Pitch])
#define IN_PORT_Gate(sampler)		((sampler)->inputPorts[SAMPLER_IN_PORT_Gate])
#define IN_PORT_Attack(sampler)		((sampler)->inputPorts[SAMPLER_IN_PORT_Attack])
#define IN_PORT_Decay(sampler)		((sampler)->inputPorts[SAMPLER_IN_PORT_Decay])
#define IN_PORT_Sustain(sampler)		((sampler)->inputPorts[SAMPLER_IN_PORT_Sustain])
#define IN_PORT_Release(sampler)		((sampler)->inputPorts[SAMPLER_IN_PORT_Release])
#define OUT_PORT_Left(sampler)		(CURR_PORT_ADDR(sampler, SAMPLER_OUT_PORT_Left))
#define OUT_PORT_Right(sampler)		(CURR_PORT_ADDR(sampler, SAMPLER_OUT_PORT_Right))
#define OUT_PORT_Mono(sampler)		(CURR_PORT_ADDR(sampler, SAMPLER_OUT_PORT_Mono))

#define IN_MIDI_PORT_Midi(sampler)		((sampler)->inputPorts[SAMPLER_IN_PORT_Midi])
#define GET_CONTROL_CURR_Pitch(sampler)	((sampler)->controlsCurr[SAMPLER_CONTROL_Pitch])
#define GET_CONTROL_PREV_Pitch(sampler)	((sampler)->controlsPrev[SAMPLER_CONTROL_Pitch])
#define GET_CONTROL_CURR_Attack(sampler)	((sampler)->controlsCurr[SAMPLER_CONTROL_Attack])
#define GET_CONTROL_PREV_Attack(sampler)	((sampler)->controlsPrev[SAMPLER_CONTROL_Attack])
#define GET_CONTROL_CURR_Decay(sampler)	((sampler)->controlsCurr[SAMPLER_CONTROL_Decay])
#define GET_CONTROL_PREV_Decay(sampler)	((sampler)->controlsPrev[SAMPLER_CONTROL_Decay])
#define GET_CONTROL_CURR_Sustain(sampler)	((sampler)->controlsCurr[SAMPLER_CONTROL_Sustain])
#define GET_CONTROL_PREV_Sustain(sampler)	((sampler)->controlsPrev[SAMPLER_CONTROL_Sustain])
#define GET_CONTROL_CURR_Release(sampler)	((sampler)->controlsCurr[SAMPLER_CONTROL_Release])
#define GET_CONTROL_PREV_Release(sampler)	((sampler)->controlsPrev[SAMPLER_CONTROL_Release])

#define SET_CONTROL_CURR_Pitch(sampler, v)	((sampler)->controlsCurr[SAMPLER_CONTROL_Pitch] = (v))
#define SET_CONTROL_PREV_Pitch(sampler, v)	((sampler)->controlsPrev[SAMPLER_CONTROL_Pitch] = (v))
#define SET_CONTROL_CURR_Attack(sampler, v)	((sampler)->controlsCurr[SAMPLER_CONTROL_Attack] = (v))
#define SET_CONTROL_PREV_Attack(sampler, v)	((sampler)->controlsPrev[SAMPLER_CONTROL_Attack] = (v))
#define SET_CONTROL_CURR_Decay(sampler, v)	((sampler)->controlsCurr[SAMPLER_CONTROL_Decay] = (v))
#define SET_CONTROL_PREV_Decay(sampler, v)	((sampler)->controlsPrev[SAMPLER_CONTROL_Decay] = (v))
#define SET_CONTROL_CURR_Sustain(sampler, v)	((sampler)->controlsCurr[SAMPLER_CONTROL_Sustain] = (v))
#define SET_CONTROL_PREV_Sustain(sampler, v)	((sampler)->controlsPrev[SAMPLER_CONTROL_Sustain] = (v))
#define SET_CONTROL_CURR_Release(sampler, v)	((sampler)->controlsCurr[SAMPLER_CONTROL_Release] = (v))
#define SET_CONTROL_PREV_Release(sampler, v)	((sampler)->controlsPrev[SAMPLER_CONTROL_Release] = (v))


#define GET_MIDI_CONTROL_RING_BUFFER(mod, port)   (((Sampler*)(mod))->midiControlsRingBuffer + (MIDI_STREAM_BUFFER_SIZE * (port)))


#define CONTROL_PUSH_TO_PREV(sampler)         for (U4 i = 0; i < SAMPLER_CONTROLCOUNT; i++) {(sampler)->controlsPrev[i] = (sampler)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_sampler(void * modPtr);
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
static bool reloadAudioFile(Sampler* sampler);



//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool tableInitDone = false;

static char * inPortNames[SAMPLER_INCOUNT + SAMPLER_MIDI_INCOUNT] = {
	"Pitch",
	"Gate",
	"Attack",
	"Decay",
	"Sustain",
	"Release",
	"Midi",
};
static char * outPortNames[SAMPLER_OUTCOUNT + SAMPLER_MIDI_INCOUNT] = {
	"Left",
	"Right",
	"Mono",
};
static char * controlNames[SAMPLER_CONTROLCOUNT + SAMPLER_MIDI_CONTROLCOUNT + SAMPLER_BYTE_CONTROLCOUNT] = {
	"Pitch",
	"Attack",
	"Decay",
	"Sustain",
	"Release",
	"File",
};

static Module vtable = {
  .type = ModuleType_Sampler,
  .freeModule = free_sampler,
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

#define DEFAULT_CONTROL_Pitch    0
#define DEFAULT_CONTROL_Attack    0
#define DEFAULT_CONTROL_Decay    0
#define DEFAULT_CONTROL_Sustain    0
#define DEFAULT_CONTROL_Release    0

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


void Sampler_initInPlace(Sampler* sampler, char* name)
{
  if (!tableInitDone)
  {
    initTables();
    tableInitDone = true;
  }

  memset(sampler, 0, sizeof(Sampler));

  // set vtable
  sampler->module = vtable;

  // set name of module
  sampler->module.name = name;
  
  // set all control values

	SET_CONTROL_CURR_Pitch(sampler, DEFAULT_CONTROL_Pitch);
	SET_CONTROL_CURR_Attack(sampler, DEFAULT_CONTROL_Attack);
	SET_CONTROL_CURR_Decay(sampler, DEFAULT_CONTROL_Decay);
	SET_CONTROL_CURR_Sustain(sampler, DEFAULT_CONTROL_Sustain);
	SET_CONTROL_CURR_Release(sampler, DEFAULT_CONTROL_Release);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(sampler);

  for (int i = 0; i < SAMPLER_MIDI_CONTROLCOUNT; i++)
  {
    sampler->midiRingRead[i] = 0;
    sampler->midiRingWrite[i] = 0;
  }

  sampler->byteArrayControlStorage[SAMPLER_BYTE_CONTROL_File] = "/media/ben/e872fd43-83e4-40e6-b40d-9b7bf7bd43d5/Ableton/All Stuffs Ableton/Drums/Kick/mhak Kicks/mhak Kick 08 F.wav";
  sampler->byteArrayDirty[SAMPLER_BYTE_CONTROL_File] = 1;
  sampler->byteArrayLen[SAMPLER_BYTE_CONTROL_File] = strlen("/media/ben/e872fd43-83e4-40e6-b40d-9b7bf7bd43d5/Ableton/All Stuffs Ableton/Drums/Kick/mhak Kicks/mhak Kick 08 F.wav");
}



Module * Sampler_init(char* name)
{
  Sampler * sampler = (Sampler*)calloc(1, sizeof(Sampler));

  Sampler_initInPlace(sampler, name);
  return (Module*)sampler;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////


static void free_sampler(void * modPtr)
{
  Sampler * sampler = (Sampler *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  for (int i = 0; i < SAMPLER_BYTE_CONTROLCOUNT; i++)
  {
    if (sampler->byteArrayControlStorage[i]) free(sampler->byteArrayControlStorage[i]);
  }

  if (sampler->audioBuffer.data) free(sampler->audioBuffer.data);

  free(sampler);
}

static void updateState(void * modPtr)
{
    Sampler * sampler = (Sampler*)modPtr;

    R4* out = OUT_PORT_Mono(sampler);

    if (!ByteArrayHelpers_TryGetLock(&sampler->byteArrayLock[SAMPLER_BYTE_CONTROL_File]))
    {
        memset(out, 0, MODULE_BUFFER_SIZE);
        return;
    }

    if (sampler->byteArrayDirty[SAMPLER_BYTE_CONTROL_File])
    {
        // load the new audio from wav file
        bool suc = reloadAudioFile(sampler);
        sampler->byteArrayDirty[SAMPLER_BYTE_CONTROL_File] = false;

        if (suc)
        {
            printf("YEAAAA WE GOT EM BOYS\n");
        }
    }

    ByteArrayHelpers_FreeLock(&sampler->byteArrayLock[SAMPLER_BYTE_CONTROL_File]);
}


static void pushCurrToPrev(void * modPtr)
{
  Sampler * mi = (Sampler*)modPtr;
  memcpy(mi->outputPortsPrev, mi->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * SAMPLER_OUTCOUNT);
  memcpy(mi->outputMIDIPortsPrev, mi->outputMIDIPortsCurr, sizeof(MIDIData) * SAMPLER_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE);
  CONTROL_PUSH_TO_PREV(mi);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port < SAMPLER_OUTCOUNT) return PREV_PORT_ADDR(modPtr, port);
  else if ((port - SAMPLER_OUTCOUNT) < SAMPLER_MIDI_OUTCOUNT) return PREV_MIDI_PORT_ADDR(modPtr, port - SAMPLER_OUTCOUNT);

  return NULL;
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
    if (port < SAMPLER_INCOUNT) {return IN_PORT_ADDR(modPtr, port);}
    else if ((port - SAMPLER_INCOUNT) < SAMPLER_MIDI_INCOUNT) return IN_MIDI_PORT_ADDR(modPtr, port - SAMPLER_INCOUNT);

  return NULL;
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < SAMPLER_INCOUNT) return ModulePortType_VoltStream;
  else if ((port - SAMPLER_INCOUNT) < SAMPLER_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < SAMPLER_OUTCOUNT) return ModulePortType_VoltStream;
  else if ((port - SAMPLER_OUTCOUNT) < SAMPLER_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < SAMPLER_CONTROLCOUNT) return ModulePortType_VoltControl;
  else if ((port - SAMPLER_CONTROLCOUNT) < SAMPLER_MIDI_CONTROLCOUNT) return ModulePortType_MIDIControl;
  else if ((port - SAMPLER_CONTROLCOUNT - SAMPLER_MIDI_CONTROLCOUNT) < SAMPLER_BYTE_CONTROLCOUNT) return ModulePortType_ByteArrayControl;
  return ModulePortType_None;
}

static U4 getInCount(void * modPtr)
{
  return SAMPLER_INCOUNT + SAMPLER_MIDI_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return SAMPLER_OUTCOUNT + SAMPLER_MIDI_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return SAMPLER_CONTROLCOUNT + SAMPLER_MIDI_CONTROLCOUNT + SAMPLER_BYTE_CONTROLCOUNT;
}


static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
    if (id < SAMPLER_CONTROLCOUNT)
  {
    Volt v = *(Volt*)val;
    switch (id)
    {

        case SAMPLER_CONTROL_Pitch:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case SAMPLER_CONTROL_Attack:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case SAMPLER_CONTROL_Decay:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case SAMPLER_CONTROL_Sustain:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    

        case SAMPLER_CONTROL_Release:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    

      default:
        break;
    }

    ((Sampler*)modPtr)->controlsCurr[id] = v;
  }

  else if ((id - SAMPLER_CONTROLCOUNT) < SAMPLER_MIDI_CONTROLCOUNT) 
  {
    int p = id - SAMPLER_CONTROLCOUNT;
    bool good = MIDI_PushRingBuffer(GET_MIDI_CONTROL_RING_BUFFER(modPtr, p), *(MIDIData*)val, &(((Sampler*)modPtr)->midiRingWrite[p]), &(((Sampler*)modPtr)->midiRingRead[p]));
    if (!good) printf("dropped midi packet\n");
  }
  else if ((id - (SAMPLER_CONTROLCOUNT + SAMPLER_MIDI_CONTROLCOUNT)) < SAMPLER_BYTE_CONTROLCOUNT)
  {
    ByteArray* ba = (ByteArray*)val;
    int p = id - (SAMPLER_CONTROLCOUNT + SAMPLER_MIDI_CONTROLCOUNT);
    Sampler* s = (Sampler*)modPtr;
    while (!ByteArrayHelpers_TryGetLock(&s->byteArrayLock[p])) { }
    
    if (ba && ba->length)
    {
        char* newBuff = malloc(ba->length);
        memcpy(newBuff, ba->bytes, ba->length);
        s->byteArrayDirty[p] = 1;
        if (s->byteArrayControlStorage[p])
        {
            free(s->byteArrayControlStorage[p]);
        }
        s->byteArrayControlStorage[p] = newBuff;
        s->byteArrayLen[p] = ba->length;
    
        
    }
    else
    {
        s->byteArrayControlStorage[p] = NULL;
        s->byteArrayLen[p] = 0;
        s->byteArrayDirty[p] = 1;
    }

    ByteArrayHelpers_FreeLock(&s->byteArrayLock[p]);
  }
}


static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id < SAMPLER_CONTROLCOUNT) *(Volt*)ret = ((Sampler*)modPtr)->controlsCurr[id];
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
    Sampler * mi = (Sampler*)modPtr;
  if (port < SAMPLER_INCOUNT) mi->inputPorts[port] = (R4*)readAddr;
  else if ((port - SAMPLER_INCOUNT) < SAMPLER_MIDI_INCOUNT) mi->inputMIDIPorts[port - SAMPLER_INCOUNT] = (MIDIData*)readAddr;
}

static void initTables()
{
    
}

static bool reloadAudioFile(Sampler* sampler)
{
    if (sampler->audioBuffer.data) free(sampler->audioBuffer.data);
    sampler->audioBuffer.data = NULL;
    sampler->audioBuffer.frames = 0;

    if (sampler->byteArrayControlStorage[SAMPLER_BYTE_CONTROL_File] && sampler->byteArrayLen[SAMPLER_BYTE_CONTROL_File])
    {
        char strArr[sampler->byteArrayLen[SAMPLER_BYTE_CONTROL_File] + 1];
        memcpy(strArr, sampler->byteArrayControlStorage[SAMPLER_BYTE_CONTROL_File], sampler->byteArrayLen[SAMPLER_BYTE_CONTROL_File]);
        strArr[sampler->byteArrayLen[SAMPLER_BYTE_CONTROL_File]] = 0;

        SF_INFO info = {0};
        SNDFILE* file = sf_open(strArr, SFM_READ, &info);

        if (!file) 
        {
            printf("Failed to open WAV: %s\n", strArr);
            return false;
        }

        sampler->audioBuffer.frames = info.frames;
        sampler->audioBuffer.channels = info.channels;
        sampler->audioBuffer.sampleRate = info.samplerate;

        int totalSamples = info.frames * info.channels;
        sampler->audioBuffer.data = malloc(sizeof(float) * totalSamples);

        // libsndfile gives samples in range [-1, 1]
        sf_readf_float(file, sampler->audioBuffer.data, info.frames);
        sf_close(file);

        // Scale to [-5, 5]
        for (int i = 0; i < totalSamples; i++) 
        {
            sampler->audioBuffer.data[i] = MAP(-1.f, 1.f, VOLTSTD_AUD_MIN, VOLTSTD_AUD_MAX, sampler->audioBuffer.data[i]);
        }
    }

    return true;
}