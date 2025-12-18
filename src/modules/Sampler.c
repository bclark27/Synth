
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
#define IN_PORT_RecordGate(sampler)		((sampler)->inputPorts[SAMPLER_IN_PORT_RecordGate])
#define IN_PORT_RecordAudio(sampler)		((sampler)->inputPorts[SAMPLER_IN_PORT_RecordAudio])
#define OUT_PORT_Audio(sampler)		(CURR_PORT_ADDR(sampler, SAMPLER_OUT_PORT_Audio))

#define IN_MIDI_PORT_Midi(sampler)		((sampler)->inputMIDIPorts[SAMPLER_IN_MIDI_PORT_Midi])

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
#define GET_CONTROL_CURR_Source(sampler)	((sampler)->controlsCurr[SAMPLER_CONTROL_Source])
#define GET_CONTROL_PREV_Source(sampler)	((sampler)->controlsPrev[SAMPLER_CONTROL_Source])
#define GET_CONTROL_CURR_PlaybackMode(sampler)	((sampler)->controlsCurr[SAMPLER_CONTROL_PlaybackMode])
#define GET_CONTROL_PREV_PlaybackMode(sampler)	((sampler)->controlsPrev[SAMPLER_CONTROL_PlaybackMode])
#define GET_CONTROL_CURR_PlaybackStart(sampler)	((sampler)->controlsCurr[SAMPLER_CONTROL_PlaybackStart])
#define GET_CONTROL_PREV_PlaybackStart(sampler)	((sampler)->controlsPrev[SAMPLER_CONTROL_PlaybackStart])
#define GET_CONTROL_CURR_LoopStart(sampler)	((sampler)->controlsCurr[SAMPLER_CONTROL_LoopStart])
#define GET_CONTROL_PREV_LoopStart(sampler)	((sampler)->controlsPrev[SAMPLER_CONTROL_LoopStart])
#define GET_CONTROL_CURR_LoopEnd(sampler)	((sampler)->controlsCurr[SAMPLER_CONTROL_LoopEnd])
#define GET_CONTROL_PREV_LoopEnd(sampler)	((sampler)->controlsPrev[SAMPLER_CONTROL_LoopEnd])
#define GET_CONTROL_CURR_CrossFade(sampler)	((sampler)->controlsCurr[SAMPLER_CONTROL_CrossFade])
#define GET_CONTROL_PREV_CrossFade(sampler)	((sampler)->controlsPrev[SAMPLER_CONTROL_CrossFade])

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
#define SET_CONTROL_CURR_Source(sampler, v)	((sampler)->controlsCurr[SAMPLER_CONTROL_Source] = (v))
#define SET_CONTROL_PREV_Source(sampler, v)	((sampler)->controlsPrev[SAMPLER_CONTROL_Source] = (v))
#define SET_CONTROL_CURR_PlaybackMode(sampler, v)	((sampler)->controlsCurr[SAMPLER_CONTROL_PlaybackMode] = (v))
#define SET_CONTROL_PREV_PlaybackMode(sampler, v)	((sampler)->controlsPrev[SAMPLER_CONTROL_PlaybackMode] = (v))
#define SET_CONTROL_CURR_PlaybackStart(sampler, v)	((sampler)->controlsCurr[SAMPLER_CONTROL_PlaybackStart] = (v))
#define SET_CONTROL_PREV_PlaybackStart(sampler, v)	((sampler)->controlsPrev[SAMPLER_CONTROL_PlaybackStart] = (v))
#define SET_CONTROL_CURR_LoopStart(sampler, v)	((sampler)->controlsCurr[SAMPLER_CONTROL_LoopStart] = (v))
#define SET_CONTROL_PREV_LoopStart(sampler, v)	((sampler)->controlsPrev[SAMPLER_CONTROL_LoopStart] = (v))
#define SET_CONTROL_CURR_LoopEnd(sampler, v)	((sampler)->controlsCurr[SAMPLER_CONTROL_LoopEnd] = (v))
#define SET_CONTROL_PREV_LoopEnd(sampler, v)	((sampler)->controlsPrev[SAMPLER_CONTROL_LoopEnd] = (v))
#define SET_CONTROL_CURR_CrossFade(sampler, v)	((sampler)->controlsCurr[SAMPLER_CONTROL_CrossFade] = (v))
#define SET_CONTROL_PREV_CrossFade(sampler, v)	((sampler)->controlsPrev[SAMPLER_CONTROL_CrossFade] = (v))


#define GET_MIDI_CONTROL_RING_BUFFER(mod, port)   (((Sampler*)(mod))->midiControlsRingBuffer + (MIDI_STREAM_BUFFER_SIZE * (port)))


#define CONTROL_PUSH_TO_PREV(sampler)         for (U4 i = 0; i < SAMPLER_CONTROLCOUNT; i++) {(sampler)->controlsPrev[i] = (sampler)->controlsCurr[i];}

/////////////
//  TYPES  //
/////////////

typedef enum PlaybackMode
{
  PlaybackMode_OneShot,
  PlaybackMode_Loop,
} PlaybackMode;


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
static void setControlVal(void * modPtr, ModularPortID id, void* val, unsigned int len);
static void getControlVal(void * modPtr, ModularPortID id, void* ret, unsigned int* len);
static void linkToInput(void * modPtr, ModularPortID port, void * readAddr);
static void initTables();
static bool reloadAudioFile(Sampler* sampler);
static void updateEnvelope(Sampler* sampler);



//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool tableInitDone = false;
static R4 zeroVoltTable[MODULE_BUFFER_SIZE];
static R4 maxCvTable[MODULE_BUFFER_SIZE];
static R4 gateTriggerTable[MODULE_BUFFER_SIZE];

static char * inPortNames[SAMPLER_INCOUNT + SAMPLER_MIDI_INCOUNT] = {
	"Pitch",
	"Gate",
	"Attack",
	"Decay",
	"Sustain",
	"Release",
	"RecordGate",
  "RecordAudio",
	"Midi",
};
static char * outPortNames[SAMPLER_OUTCOUNT + SAMPLER_MIDI_INCOUNT] = {
	"Audio",
};
static char * controlNames[SAMPLER_CONTROLCOUNT + SAMPLER_MIDI_CONTROLCOUNT + SAMPLER_BYTE_CONTROLCOUNT] = {
	"Pitch",
	"Attack",
	"Decay",
	"Sustain",
	"Release",
	"Source",
	"PlaybackMode",
	"PlaybackStart",
	"LoopStart",
	"LoopEnd",
	"CrossFade",
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
#define DEFAULT_CONTROL_Attack    0.1
#define DEFAULT_CONTROL_Decay    0.2
#define DEFAULT_CONTROL_Sustain    10
#define DEFAULT_CONTROL_Release    0.6
#define DEFAULT_CONTROL_Source    0
#define DEFAULT_CONTROL_PlaybackMode    0
#define DEFAULT_CONTROL_PlaybackStart    0
#define DEFAULT_CONTROL_LoopStart    0
#define DEFAULT_CONTROL_LoopEnd    VOLTSTD_MOD_CV_MAX
#define DEFAULT_CONTROL_CrossFade    0

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
	SET_CONTROL_CURR_Source(sampler, DEFAULT_CONTROL_Source);
	SET_CONTROL_CURR_PlaybackMode(sampler, DEFAULT_CONTROL_PlaybackMode);
	SET_CONTROL_CURR_PlaybackStart(sampler, DEFAULT_CONTROL_PlaybackStart);
	SET_CONTROL_CURR_LoopStart(sampler, DEFAULT_CONTROL_LoopStart);
	SET_CONTROL_CURR_LoopEnd(sampler, DEFAULT_CONTROL_LoopEnd);

  // push curr to prev
  CONTROL_PUSH_TO_PREV(sampler);

  for (int i = 0; i < SAMPLER_MIDI_CONTROLCOUNT; i++)
  {
    sampler->midiRingRead[i] = 0;
    sampler->midiRingWrite[i] = 0;
  }

  ADSR_initInPlace(&sampler->adsr, (char*)malloc(1));
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

  ADSR_freeInPlace(&sampler->adsr);

  free(sampler);
}

static void updateState(void * modPtr)
{
  Sampler * sampler = (Sampler*)modPtr;

  R4* out = OUT_PORT_Audio(sampler);

  updateEnvelope(sampler);
  // if true, then we use the file as src
  // if false, we are using the recorded audio from the ports
  bool srcModeIsFile = GET_CONTROL_CURR_Source(sampler) < 5; // lot voltage = file mode. high voltage = port input mode

  R4* recordGate = IN_PORT_RecordGate(sampler) ? IN_PORT_RecordGate(sampler) : zeroVoltTable;
  R4* playbackGate = IN_PORT_Gate(sampler) ? IN_PORT_Gate(sampler) : zeroVoltTable;
  R4* inputAudio = IN_PORT_RecordAudio(sampler) ? IN_PORT_RecordAudio(sampler) : zeroVoltTable;

  bool samplerIsActive = sampler->adsr.envelopeActive;
  updateEnvelope(sampler);
  samplerIsActive |= sampler->adsr.envelopeActive;

  R4* envelop = sampler->adsr.outputPortsPrev + (MODULE_BUFFER_SIZE * ADSR_OUT_PORT_ENV);
  // if the sampler was active at the beginning of the frame or is active at the end, we will consider the sampler as active and try to parse audio. the gate will then determin the exact times when the idx in the audio sample is reset

  PlaybackMode playMode = GET_CONTROL_CURR_PlaybackMode(sampler) < 5 ? PlaybackMode_OneShot : PlaybackMode_Loop;

  R4* src;
  unsigned int srcLen;
  if (srcModeIsFile)
  {
    src = sampler->audioBuffer.data;
    srcLen = sampler->audioBuffer.frames;
    if (!AtomicHelpers_TryGetLock(&sampler->byteArrayLock[SAMPLER_BYTE_CONTROL_File]))
    {
      srcLen = 0;
    }
    else
    {
      if (sampler->byteArrayDirty[SAMPLER_BYTE_CONTROL_File])
      {
        // load the new audio from wav file
        bool loaded = reloadAudioFile(sampler);
        sampler->byteArrayDirty[SAMPLER_BYTE_CONTROL_File] = false;
    
        if (loaded)
        {
          // new audio file
          src = sampler->audioBuffer.data;
          srcLen = sampler->audioBuffer.frames;
        }
        else
        {
          srcLen = 0;
        }
      }
    
      AtomicHelpers_FreeLock(&sampler->byteArrayLock[SAMPLER_BYTE_CONTROL_File]);
    }
  
  }
  else
  {
    src = sampler->portRecordedAudio;
    srcLen = sampler->portRecordedAudioLength;
  }

  if (!src || !srcLen)
  {
    memset(out, 0, MODULE_BUFFER_SIZE);
    return;
  }

  for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
  {

    // first if the record gate has just gone high
    bool recordGateIsHigh = recordGate[i] >= VOLTSTD_GATE_HIGH_THRESH;
    bool recordGateWentHigh = sampler->lastRecordGateValue < VOLTSTD_GATE_HIGH_THRESH && recordGateIsHigh;
    sampler->lastRecordGateValue = recordGate[i];

    // lets also get if out playback gate went high
    bool playbackGateWentHigh = sampler->lastPlaybackGateValue < VOLTSTD_GATE_HIGH_THRESH && playbackGate[i] >= VOLTSTD_GATE_HIGH_THRESH;
    sampler->lastPlaybackGateValue = playbackGate[i];

    // On rising edge: reset recording buffer
    if (recordGateWentHigh)
    {
      sampler->portRecordedAudioLength = 0;
      sampler->portRecordedAudioStartHead = 0;
    }

    // While gate is high: record
    if (recordGateIsHigh)
    {
      // Compute write index (tail)
      U4 writeIdx =
          (sampler->portRecordedAudioStartHead +
          sampler->portRecordedAudioLength) %
          SAMPLER_MAX_RECORD_SAMPLES;

      // Write new sample
      sampler->portRecordedAudio[writeIdx] = inputAudio[i];

      if (sampler->portRecordedAudioLength < SAMPLER_MAX_RECORD_SAMPLES)
      {
        // Buffer not full yet → just grow
        sampler->portRecordedAudioLength++;
      }
      else
      {
        // Buffer full → overwrite oldest
        sampler->portRecordedAudioStartHead =
            (sampler->portRecordedAudioStartHead + 1) %
            SAMPLER_MAX_RECORD_SAMPLES;
      }
    }

    
  }
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


static void setControlVal(void * modPtr, ModularPortID id, void* val, unsigned int len)
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
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
  

      case SAMPLER_CONTROL_Decay:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
  

      case SAMPLER_CONTROL_Sustain:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
  

      case SAMPLER_CONTROL_Release:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
  

      case SAMPLER_CONTROL_Source:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
  

      case SAMPLER_CONTROL_PlaybackMode:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
  

      case SAMPLER_CONTROL_PlaybackStart:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
  

      case SAMPLER_CONTROL_LoopStart:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
  

      case SAMPLER_CONTROL_LoopEnd:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
  

      case SAMPLER_CONTROL_CrossFade:
      v = CLAMPF(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
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
    int p = id - (SAMPLER_CONTROLCOUNT + SAMPLER_MIDI_CONTROLCOUNT);
    Sampler* s = (Sampler*)modPtr;
    AtomicHelpers_TryGetLockSpin(&s->byteArrayLock[p]);

    if (val && len)
    {
        char* newBuff = malloc(len);
        memcpy(newBuff, val, len);
        s->byteArrayDirty[p] = 1;
        if (s->byteArrayControlStorage[p])
        {
            free(s->byteArrayControlStorage[p]);
        }
        s->byteArrayControlStorage[p] = newBuff;
        s->byteArrayLen[p] = len;
    }
    else
    {
        s->byteArrayControlStorage[p] = NULL;
        s->byteArrayLen[p] = 0;
        s->byteArrayDirty[p] = 1;
    }

    AtomicHelpers_FreeLock(&s->byteArrayLock[p]);
  }
}


static void getControlVal(void * modPtr, ModularPortID id, void* ret, unsigned int* len)
{
  if (id < SAMPLER_CONTROLCOUNT)
  {
    *len = sizeof(Volt);
    *(Volt*)ret = ((Sampler*)modPtr)->controlsCurr[id];
  }
  else if ((id - SAMPLER_CONTROLCOUNT) < SAMPLER_MIDI_CONTROLCOUNT)
  {
    ModularPortID port = id - SAMPLER_CONTROLCOUNT;
    *len = sizeof(MIDIData);  
    *(MIDIData*)ret = MIDI_PeakRingBuffer(GET_MIDI_CONTROL_RING_BUFFER(modPtr, port), &(((Sampler*)modPtr)->midiRingRead[port]));
  }
  else if ((id - (SAMPLER_CONTROLCOUNT + SAMPLER_MIDI_CONTROLCOUNT)) < SAMPLER_BYTE_CONTROLCOUNT)
  {
    ModularPortID port = id - (SAMPLER_CONTROLCOUNT + SAMPLER_MIDI_CONTROLCOUNT);
    Sampler* s = (Sampler*)modPtr;
    AtomicHelpers_TryGetLockSpin(&s->byteArrayLock[port]);
    *len = s->byteArrayLen[port];
    if (s->byteArrayControlStorage[port])
    {
      memcpy(ret, s->byteArrayControlStorage[port], *len);
    }
    AtomicHelpers_FreeLock(&s->byteArrayLock[port]);
  }
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
    Sampler * mi = (Sampler*)modPtr;
  if (port < SAMPLER_INCOUNT) mi->inputPorts[port] = (R4*)readAddr;
  else if ((port - SAMPLER_INCOUNT) < SAMPLER_MIDI_INCOUNT) mi->inputMIDIPorts[port - SAMPLER_INCOUNT] = (MIDIData*)readAddr;
}

static void initTables()
{
  for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
  {
    zeroVoltTable[i] = VOLTSTD_MOD_CV_ZERO;
  }
}

static bool reloadAudioFile(Sampler* sampler)
{
  // Free previous buffer
  if (sampler->audioBuffer.data)
  {
    free(sampler->audioBuffer.data);
    sampler->audioBuffer.data = NULL;
  }

  sampler->audioBuffer.frames = 0;
  sampler->audioBuffer.channels = 1;

  if (!sampler->byteArrayControlStorage[SAMPLER_BYTE_CONTROL_File] ||
    sampler->byteArrayLen[SAMPLER_BYTE_CONTROL_File] == 0)
  {
    return false;
  }

  // Copy path to null-terminated string
  char path[sampler->byteArrayLen[SAMPLER_BYTE_CONTROL_File] + 1];
  memcpy(path,
          sampler->byteArrayControlStorage[SAMPLER_BYTE_CONTROL_File],
          sampler->byteArrayLen[SAMPLER_BYTE_CONTROL_File]);
  path[sampler->byteArrayLen[SAMPLER_BYTE_CONTROL_File]] = '\0';

  SF_INFO info = {0};
  SNDFILE* file = sf_open(path, SFM_READ, &info);
  if (!file)
  {
    printf("Failed to open WAV: %s\n", path);
    return false;
  }

  sampler->audioBuffer.frames = info.frames;
  sampler->audioBuffer.sampleRate = info.samplerate;

  // Temporary interleaved read buffer
  const int inChannels = info.channels;
  float* interleaved = malloc(sizeof(float) * info.frames * inChannels);
  if (!interleaved)
  {
    sf_close(file);
    return false;
  }

  sf_readf_float(file, interleaved, info.frames);
  sf_close(file);

  // Allocate mono output buffer
  sampler->audioBuffer.data = malloc(sizeof(float) * info.frames);
  if (!sampler->audioBuffer.data)
  {
    free(interleaved);
    return false;
  }

  // Collapse to mono
  for (int f = 0; f < info.frames; f++)
  {
    float sum = 0.0f;
    for (int c = 0; c < inChannels; c++)
    {
      sum += interleaved[f * inChannels + c];
    }

    // Average channels → mono
    float mono = sum / (float)inChannels;

    // Scale [-1,1] → [-5,5]
    sampler->audioBuffer.data[f] =
      MAP(-1.f, 1.f,
          VOLTSTD_AUD_MIN, VOLTSTD_AUD_MAX,
          mono);
  }

  free(interleaved);
  return true;
}

static void updateEnvelope(Sampler* sampler)
{

  
  R4* playbackGate = IN_PORT_Gate(sampler);
  R4* pitch = IN_PORT_Pitch(sampler);
  MIDIData* midiIn = IN_MIDI_PORT_Midi(sampler);
  
  bool useMidi = (!playbackGate || !pitch) && midiIn;

  if (useMidi)
  {
    MIDIData* md;
    for (int i = 0; i < MIDI_STREAM_BUFFER_SIZE; i++)
    {
      
    }
  }
  else
  {
    if (!playbackGate)
    {
      playbackGate = zeroVoltTable;
    }

    sampler->adsr.inputPorts[ADSR_IN_PORT_GATE] = playbackGate;
    sampler->adsr.inputPorts[ADSR_IN_PORT_A] = IN_PORT_Attack(sampler);
    sampler->adsr.inputPorts[ADSR_IN_PORT_D] = IN_PORT_Decay(sampler);
    sampler->adsr.inputPorts[ADSR_IN_PORT_S] = IN_PORT_Sustain(sampler);
    sampler->adsr.inputPorts[ADSR_IN_PORT_R] = IN_PORT_Release(sampler);
    sampler->adsr.controlsCurr[ADSR_CONTROL_A] = GET_CONTROL_CURR_Attack(sampler);
    sampler->adsr.controlsCurr[ADSR_CONTROL_D] = GET_CONTROL_CURR_Decay(sampler);
    sampler->adsr.controlsCurr[ADSR_CONTROL_S] = GET_CONTROL_CURR_Sustain(sampler);
    sampler->adsr.controlsCurr[ADSR_CONTROL_R] = GET_CONTROL_CURR_Release(sampler);
    sampler->adsr.module.updateState(&sampler->adsr);
    sampler->adsr.module.pushCurrToPrev(&sampler->adsr);
  }
}