#include "PolyKeys.h"

#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define IN_VOLT_STREAM_PORT(mod, port)           (((PolyKeys*)(mod))->inputPorts[port])
#define IN_MIDIDADA_PORT(mod, port)           (((PolyKeys*)(mod))->inputMIDIPorts[port])

#define PREV_VOLT_STREAM_PORT_ADDR(mod, port) (((PolyKeys*)(mod))->outputPortsPrev + (MODULE_BUFFER_SIZE * (port)))
#define CURR_VOLT_STREAM_PORT_ADDR(mod, port) (((PolyKeys*)(mod))->outputPortsCurr + (MODULE_BUFFER_SIZE * (port)))
#define PREV_MIDIDATA_PORT_ADDR(mod, port)    (((PolyKeys*)(mod))->outputMIDIPortsPrev + (port))
#define CURR_MIDIDATA_PORT_ADDR(mod, port)    (((PolyKeys*)(mod))->outputMIDIPortsCurr + (port))

#define OUT_PORT_OUTPUT(m)                (CURR_VOLT_STREAM_PORT_ADDR(m, POLYKEYS_OUTPORT_AUDIO))  

#define GET_MIDI_CONTROL_RING_BUFFER(mod, port)   (((PolyKeys*)(mod))->midiControlsRingBuffer + (MIDI_STREAM_BUFFER_SIZE * (port)))

#define CONTROL_PUSH_TO_PREV(m)         for (U4 i = 0; i < POLYKEYS_CONTROLCOUNT; i++) {((PolyKeys*)(m))->controlsPrev[i] = ((PolyKeys*)(m))->controlsCurr[i];} for (U4 i = 0; i < POLYKEYS_MIDI_CONTROLCOUNT; i++)

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_poly(void * modPtr);
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
static void initAdsrInputBuffers(void);
static void initVoice(PolyKeysVoice* voice);
static inline U4 getFreeOrOldVoice(PolyKeys* pk);
static void consumeMidiMessage(PolyKeys* pk);
static inline Volt midiNoteToFreqVolt(int midi_note);
static void voiceOnSetup(PolyKeys* pk, PolyKeysVoice* voice);
static void applyControlValsToModules(PolyKeys* pk, PolyKeysVoice* voice); // skip if voice setup ran, it will put in the values
static inline void assignADSRBuffer(PolyKeys* pk, PolyKeysVoice* voice);
static void updateVoice(PolyKeys* pk, PolyKeysVoice* voice);
static inline void checkADSRDone(PolyKeys* pk, PolyKeysVoice* voice);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool adsrInputBuffersInitDone = false;
static Volt fakeAdsrStart[MODULE_BUFFER_SIZE];
static Volt fakeAdsrSustain[MODULE_BUFFER_SIZE];
static Volt fakeAdsrRelease[MODULE_BUFFER_SIZE];

static char * inPortNames[POLYKEYS_INCOUNT + POLYKEYS_MIDI_INCOUNT] = {
    "MidiIn"
};

static char * outPortNames[POLYKEYS_OUTCOUNT + POLYKEYS_MIDI_OUTCOUNT] = {
    "Audio",
};

static char * controlNames[POLYKEYS_CONTROLCOUNT + POLYKEYS_MIDI_CONTROLCOUNT] = {
    "Attack",
    "Decay",
    "Sustain",
    "Release",
    "FltEnvAmt",
    "FltFreq",
    "FltRes",
    "Waveform",
    "Unison",
    "Detune",
};

static Module vtable = {
  .type = ModuleType_PolyKeys,
  .freeModule = free_poly,
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

Module * PolyKeys_init(char* name)
{
    if (!adsrInputBuffersInitDone)
        initAdsrInputBuffers();

    PolyKeys * pk = calloc(1, sizeof(PolyKeys));

    // set module
    pk->module = vtable;
    pk->module.name = name;

    // set all control values
    pk->controlsCurr[POLYKEYS_CONTROL_A] = 0.3;
    pk->controlsCurr[POLYKEYS_CONTROL_D] = 0.3;
    pk->controlsCurr[POLYKEYS_CONTROL_S] = 0.3;
    pk->controlsCurr[POLYKEYS_CONTROL_R] = 0.3;
    pk->controlsCurr[POLYKEYS_CONTROL_DETUNE] = 1.03;
    pk->controlsCurr[POLYKEYS_CONTROL_UNISON] = 5;
    pk->controlsCurr[POLYKEYS_CONTROL_WAVE] = 2.5;
    pk->controlsCurr[POLYKEYS_CONTROL_FILTER_ENV_AMT] = VOLTSTD_MOD_CV_ZERO;
    pk->controlsCurr[POLYKEYS_CONTROL_FILTER_Q] = ((VOLTSTD_MOD_CV_MAX + VOLTSTD_MOD_CV_ZERO) / 4);
    pk->controlsCurr[POLYKEYS_CONTROL_FILTER_FREQ] = VOLTSTD_MOD_CV_MAX;

    // push curr to prev
    CONTROL_PUSH_TO_PREV(pk);

    for (int i = 0; i < POLYKEYS_MIDI_CONTROLCOUNT; i++)
    {
        pk->midiRingRead[i] = 0;
        pk->midiRingWrite[i] = 0;
    }

    memset(pk->voices, 0, sizeof(PolyKeysVoice) * POLYKEYS_MAX_VOICES);
    for (int i = 0; i < POLYKEYS_MAX_VOICES; i++)
    {
        initVoice(&pk->voices[i]);
    }

    return (Module*)pk;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_poly(void * modPtr)
{
    PolyKeys * pk = (PolyKeys*)modPtr;
  
    Module * mod = (Module*)modPtr;
    free(mod->name);
  
    for (int i = 0; i < POLYKEYS_MAX_VOICES; i++)
    {
        PolyKeysVoice* voice = &pk->voices[i];
        voice->vco->module.freeModule(voice->vco);
        voice->attn->module.freeModule(voice->attn);
        voice->flt->module.freeModule(voice->flt);
        voice->adsr->module.freeModule(voice->adsr);
    }

    free(pk);
}

static void updateState(void * modPtr)
{
    PolyKeys * pk = (PolyKeys*)modPtr;

    consumeMidiMessage(pk);
    bool compileVoice[POLYKEYS_MAX_VOICES];
    for (int i = 0; i < POLYKEYS_MAX_VOICES; i++)
    {
        PolyKeysVoice* voice = &pk->voices[i];

        if (!voice->adsrActive)
        {
            compileVoice[i] = false;
            continue;
        }

        if (voice->firstOnSignal)
        {
            voiceOnSetup(pk, voice);
        }
        else
        {
            applyControlValsToModules(pk, voice);
        }

        assignADSRBuffer(pk, voice);
        updateVoice(pk, voice);
        checkADSRDone(pk, voice);

        voice->firstOnSignal = false;

        compileVoice[i] = true;
    }

    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
        OUT_PORT_OUTPUT(pk)[i] = 0;
    }

    for (int i = 0; i < POLYKEYS_MAX_VOICES; i++)
    {
        if (!compileVoice[i]) continue;
        for (int k = 0; k < MODULE_BUFFER_SIZE; k++)
        {
            OUT_PORT_OUTPUT(pk)[k] += pk->voices[i].voiceOutputBuffer[k];
        }
    }

    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
        OUT_PORT_OUTPUT(pk)[i] /= POLYKEYS_MAX_VOICES;
    }
}

static void pushCurrToPrev(void * modPtr)
{
  PolyKeys * mi = (PolyKeys*)modPtr;
  memcpy(mi->outputPortsPrev, mi->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * POLYKEYS_OUTCOUNT);
  memcpy(mi->outputMIDIPortsPrev, mi->outputMIDIPortsCurr, sizeof(MIDIData) * POLYKEYS_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE);
  CONTROL_PUSH_TO_PREV(mi);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port < POLYKEYS_OUTCOUNT) return PREV_VOLT_STREAM_PORT_ADDR(modPtr, port);
  else if ((port - POLYKEYS_OUTCOUNT) < POLYKEYS_MIDI_OUTCOUNT) return PREV_MIDIDATA_PORT_ADDR(modPtr, port - POLYKEYS_OUTCOUNT);

  return NULL;
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
    if (port < POLYKEYS_INCOUNT) return IN_VOLT_STREAM_PORT(modPtr, port);
    else if ((port - POLYKEYS_INCOUNT) < POLYKEYS_MIDI_INCOUNT) return IN_MIDIDADA_PORT(modPtr, port - POLYKEYS_INCOUNT);

  return NULL;
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < POLYKEYS_INCOUNT) return ModulePortType_VoltStream;
  else if ((port - POLYKEYS_INCOUNT) < POLYKEYS_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < POLYKEYS_OUTCOUNT) return ModulePortType_VoltStream;
  else if ((port - POLYKEYS_OUTCOUNT) < POLYKEYS_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < POLYKEYS_CONTROLCOUNT) return ModulePortType_VoltControl;
  else if ((port - POLYKEYS_CONTROLCOUNT) < POLYKEYS_MIDI_CONTROLCOUNT) return ModulePortType_MIDIControl;
  return ModulePortType_None;
}

static U4 getInCount(void * modPtr)
{
  return POLYKEYS_INCOUNT + POLYKEYS_MIDI_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return POLYKEYS_OUTCOUNT + POLYKEYS_MIDI_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return POLYKEYS_CONTROLCOUNT + POLYKEYS_MIDI_CONTROLCOUNT;
}

static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
    if (id < POLYKEYS_CONTROLCOUNT)
  {
    Volt v = *(Volt*)val;
    switch (id)
    {
      case POLYKEYS_CONTROL_A:
      v = CLAMP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case POLYKEYS_CONTROL_D:
      v = CLAMP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case POLYKEYS_CONTROL_S:
      v = CLAMP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case POLYKEYS_CONTROL_R:
      v = CLAMP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case POLYKEYS_CONTROL_FILTER_ENV_AMT:
      v = CLAMP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case POLYKEYS_CONTROL_FILTER_FREQ:
      v = CLAMP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case POLYKEYS_CONTROL_FILTER_Q:
      v = CLAMP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case POLYKEYS_CONTROL_WAVE:
      v = CLAMP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case POLYKEYS_CONTROL_UNISON:
      v = CLAMP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      case POLYKEYS_CONTROL_DETUNE:
      v = CLAMP(VOLTSTD_MOD_CV_ZERO, VOLTSTD_MOD_CV_MAX, v);
      break;
      
      default:
        break;
    }

    ((PolyKeys*)modPtr)->controlsCurr[id] = v;
  }
  else if ((id - POLYKEYS_CONTROLCOUNT) < POLYKEYS_MIDI_CONTROLCOUNT) 
    {
        bool good = MIDI_PushRingBuffer(GET_MIDI_CONTROL_RING_BUFFER(modPtr, id - POLYKEYS_CONTROLCOUNT), *(MIDIData*)val, &(((PolyKeys*)modPtr)->midiRingWrite[id - POLYKEYS_CONTROLCOUNT]), &(((PolyKeys*)modPtr)->midiRingRead[id - POLYKEYS_CONTROLCOUNT]));
        if (!good) printf("dropped midi packet\n");
    }
}

static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id < POLYKEYS_CONTROLCOUNT) *(Volt*)ret = ((PolyKeys*)modPtr)->controlsCurr[id];
  else if ((id - POLYKEYS_CONTROLCOUNT) < POLYKEYS_MIDI_CONTROLCOUNT) *(MIDIData*)ret = MIDI_PeakRingBuffer(GET_MIDI_CONTROL_RING_BUFFER(modPtr, id - POLYKEYS_CONTROLCOUNT), &(((PolyKeys*)modPtr)->midiRingRead[id - POLYKEYS_CONTROLCOUNT]));
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
    PolyKeys * mi = (PolyKeys*)modPtr;
  if (port < POLYKEYS_INCOUNT) mi->inputPorts[port] = readAddr;
  else if ((port - POLYKEYS_INCOUNT) < POLYKEYS_MIDI_INCOUNT) mi->inputMIDIPorts[port - POLYKEYS_INCOUNT] = readAddr;
}

static void initAdsrInputBuffers(void)
{
    adsrInputBuffersInitDone = true;

    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
        fakeAdsrStart[i] = i == 0 ? VOLTSTD_GATE_LOW : VOLTSTD_GATE_HIGH;
        fakeAdsrSustain[i] = VOLTSTD_GATE_HIGH;
        fakeAdsrRelease[i] = VOLTSTD_GATE_LOW;
    }
}

static void initVoice(PolyKeysVoice* voice)
{
    voice->vco = (VCO*)VCO_init("mod");
    voice->adsr = (ADSR*)ADSR_init("mod");
    voice->flt = (Filter*)Filter_init("mod");
    voice->attn = (Attenuverter*)Attenuverter_init("mod");

    // connect the input of the flt to the out of the vco audio
    voice->flt->module.linkToInput(
        (Module*)voice->flt,
        FILTER_IN_PORT_AUD,
        voice->vco->module.getOutputAddr(
            (Module*)voice->vco, 
            VCO_OUT_PORT_AUD
        )
    );

    // connect the input of the attn vol to the output env of the adsr
    voice->attn->module.linkToInput(
        (Module*)voice->attn,
        ATTN_IN_PORT_ATTN,
        voice->adsr->module.getOutputAddr(
            (Module*)voice->adsr, 
            ADSR_OUT_PORT_ENV
        )
    );

    // connect the input of the fitler env to the output of the adsr
    voice->flt->module.linkToInput(
        (Module*)voice->flt,
        FILTER_IN_PORT_FREQ,
        voice->adsr->module.getOutputAddr(
            (Module*)voice->adsr, 
            ADSR_OUT_PORT_ENV
        )
    );

    // connect the input of the attn to the output of the fitler
    voice->attn->module.linkToInput(
        (Module*)voice->attn,
        ATTN_IN_PORT_SIG,
        voice->flt->module.getOutputAddr(
            (Module*)voice->flt, 
            FILTER_OUT_PORT_AUD
        )
    );
    
    // conenct the input of the vco to the volt stream input
    voice->vco->module.linkToInput(
        (Module*)voice->vco,
        VCO_IN_PORT_FREQ,
        voice->voiceInputFreqBuffer
    );

    // set up the output of the attn as the last step to the voice
    voice->voiceOutputBuffer = voice->attn->module.getOutputAddr(
        (Module*)voice->attn, 
        ATTN_OUT_PORT_SIG
    );
}

static inline U4 getFreeOrOldVoice(PolyKeys* pk)
{
    U8 oldestTime = 0;
    U4 oldestIdx = 0;
    for (int i = 0; i < POLYKEYS_MAX_VOICES; i++)
    {
        if (!pk->voices[i].adsrActive) return i;

        if (oldestTime <= pk->voices[i].noteAge)
        {
            oldestIdx = i;
            oldestTime = pk->voices[i].noteAge;
        }
    }

    return oldestIdx;
}

static void consumeMidiMessage(PolyKeys* pk)
{
    // in here we read all the midi data off the input
    // turn of/off/sustain whatever notes

    // mainly if a the voices are full and a new note is pressed then kick off the oldest one
    // if the note is turning on for the first time then set noteIsOn, adsrActive, firstOnSignal, note, startVelocity

    PolyKeysVoice* v;
    int idx;
    for (int i = 0; i < MIDI_STREAM_BUFFER_SIZE; i++)
    {
        MIDIData data = IN_MIDIDADA_PORT(pk, POLYKEYS_MIDI_INPUT_MIDIIN)[i];

        //if (data.type != MIDIDataType_None) printf("%d: %02x, %d\n", i, data.type, data.data1);

        switch (data.type)
        {
            // if not on then we need to kick someone out
            case MIDIDataType_NoteOn:
            {
                v = NULL;
                for (int i = 0; i < POLYKEYS_MAX_VOICES; i++)
                {
                    if (pk->voices[i].note == data.data1)
                    {
                        v = &pk->voices[i];
                        break;
                    }
                }

                if (!v)
                {
                    v = &pk->voices[getFreeOrOldVoice(pk)];
                }
                v->noteIsOn = true;
                v->adsrActive = true;
                v->firstOnSignal = true;
                v->note = data.data1;
                v->startVelocity = data.data2;
                v->velocityAmplitudeMultiplier = data.data2 / 127.0f;
                //printf("Turning on voice %d with note %d\n", idx, v->note);
                break;
            }
            // if note off then we can set one of the notes here to off which corrosponds
            case MIDIDataType_NoteOff:
            {
                v = NULL;
                for (int i = 0; i < POLYKEYS_MAX_VOICES; i++)
                {
                    if (pk->voices[i].note == data.data1)
                    {
                        v = &pk->voices[i];
                        //printf("Turning OFF voice %d with note %d\n", i, v->note);
                        break;
                    }
                }

                if (!v) break;

                v->noteIsOn = false;
                break;
            }
            case MIDIDataType_None:
            {
                break;
            }
            default:
            {
                //printf("Unused midi info: %d\n", data.type);
                break;
            }
        }
    }
}

static inline Volt midiNoteToFreqVolt(int midi_note)
{
    return (midi_note - 48) / 12.0f;
}

static void voiceOnSetup(PolyKeys* pk, PolyKeysVoice* voice)
{
    /*
    assume that the following feilds are filled in correctly:
        note
        startVelocity
        firstOnSignal
        noteIsOn
        adsrActive
    
    TODO: also be sure to set the control values of the modules and push curr to prev so no interp goes weird between note presses***
    fill up the voiceInputFreqBuffer with currect note freq
    */

    Volt freq = midiNoteToFreqVolt(voice->note);
    for (int i = 0; i < MIDI_STREAM_BUFFER_SIZE; i++)
    {
        voice->voiceInputFreqBuffer[i] = freq;
    }

    // put control updates here
    applyControlValsToModules(pk, voice);

    voice->adsr->module.pushCurrToPrev(voice->adsr);
    voice->vco->module.pushCurrToPrev(voice->vco);
    voice->flt->module.pushCurrToPrev(voice->flt);
    voice->attn->module.pushCurrToPrev(voice->attn);
}

static void applyControlValsToModules(PolyKeys* pk, PolyKeysVoice* voice)
{
    // push all the relevent control values of the full poly module to the relivent sub modules
    // if want to get more performance then directly modify the curr control arrays of the controls and bypass the functions
    R4 A = pk->controlsCurr[POLYKEYS_CONTROL_A];
    R4 D = pk->controlsCurr[POLYKEYS_CONTROL_D];
    R4 S = pk->controlsCurr[POLYKEYS_CONTROL_S];
    R4 R = pk->controlsCurr[POLYKEYS_CONTROL_R];
    R4 fltEnvAmt = pk->controlsCurr[POLYKEYS_CONTROL_FILTER_ENV_AMT];
    R4 fltFreq = pk->controlsCurr[POLYKEYS_CONTROL_FILTER_FREQ];
    R4 fltQ = pk->controlsCurr[POLYKEYS_CONTROL_FILTER_Q];
    R4 wave = pk->controlsCurr[POLYKEYS_CONTROL_WAVE];
    R4 unison = pk->controlsCurr[POLYKEYS_CONTROL_UNISON];
    R4 detune = pk->controlsCurr[POLYKEYS_CONTROL_DETUNE];
    
    voice->adsr->module.setControlVal(voice->adsr, ADSR_CONTROL_A, &A);
    voice->adsr->module.setControlVal(voice->adsr, ADSR_CONTROL_D, &D);
    voice->adsr->module.setControlVal(voice->adsr, ADSR_CONTROL_S, &S);
    voice->adsr->module.setControlVal(voice->adsr, ADSR_CONTROL_R, &R);

    voice->flt->module.setControlVal(voice->flt, FILTER_CONTROL_ENV, &fltEnvAmt);
    voice->flt->module.setControlVal(voice->flt, FILTER_CONTROL_FREQ, &fltFreq);
    voice->flt->module.setControlVal(voice->flt, FILTER_CONTROL_Q, &fltQ);

    voice->vco->module.setControlVal(voice->vco, VCO_CONTROL_UNI, &unison);
    voice->vco->module.setControlVal(voice->vco, VCO_CONTROL_DET, &detune);
    voice->vco->module.setControlVal(voice->vco, VCO_CONTROL_WAVE, &wave);
}

static inline void assignADSRBuffer(PolyKeys* pk, PolyKeysVoice* voice)
{
    // depending on the values of noteIsOn and firstOnSignal then set the input gate buffer of the adsr
    if (voice->firstOnSignal)
    {
        voice->adsr->inputPorts[ADSR_IN_PORT_GATE] = fakeAdsrStart;
    } 
    else if (voice->noteIsOn)
    {
        voice->adsr->inputPorts[ADSR_IN_PORT_GATE] = fakeAdsrSustain;
    }
    else
    {
        voice->adsr->inputPorts[ADSR_IN_PORT_GATE] = fakeAdsrRelease;
    }
}

static void updateVoice(PolyKeys* pk, PolyKeysVoice* voice)
{
    // update modules in the right order
    // push curr to prev for each module should take place inidietly after the mod update to get continuous flow of sound out the end
    
    voice->adsr->module.updateState(voice->adsr);
    voice->adsr->module.pushCurrToPrev(voice->adsr);

    voice->vco->module.updateState(voice->vco);
    voice->vco->module.pushCurrToPrev(voice->vco);

    voice->flt->module.updateState(voice->flt);
    voice->flt->module.pushCurrToPrev(voice->flt);

    voice->attn->module.updateState(voice->attn);
    voice->attn->module.pushCurrToPrev(voice->attn);

    // multiply the voiceOutputBuffer values by velocityAmplitudeMultiplier for velocity volume
    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
        voice->voiceOutputBuffer[i] *= voice->velocityAmplitudeMultiplier;
    }
}

static inline void checkADSRDone(PolyKeys* pk, PolyKeysVoice* voice)
{
    // the main metric for if a note is still hanging on is if the adsr has finished its envelope
    voice->adsrActive = voice->adsr->envelopeActive;
}