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

#define OUT_PORT_OUTPUT(m)                (CURR_MIDIDATA_PORT_ADDR(m, POLYKEYS_MIDI_OUTPUT_OUTPUT))  

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
static void consumeMidiMessage(PolyKeys* pk);
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

    for (int i = 0; i < POLYKEYS_MIDI_CONTROLCOUNT; i++)
    {
        pk->midiRingRead[i] = 0;
        pk->midiRingWrite[i] = 1;
    }

    memset(pk->voices, 0, sizeof(PolyKeysVoice) * POLYKEYS_MAX_VOICES);
    for (int i = 0; i < POLYKEYS_MAX_VOICES; i++)
    {
        initVoice(&pk->voices[i]);
    }
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
  if ((port - POLYKEYS_OUTCOUNT) < POLYKEYS_MIDI_OUTCOUNT) return PREV_MIDIDATA_PORT_ADDR(modPtr, port - POLYKEYS_OUTCOUNT);

  return NULL;
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
    if (port < POLYKEYS_INCOUNT) return IN_VOLT_STREAM_PORT(modPtr, port);
    if ((port - POLYKEYS_INCOUNT) < POLYKEYS_MIDI_INCOUNT) return IN_MIDIDADA_PORT(modPtr, port - POLYKEYS_INCOUNT);

  return NULL;
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < POLYKEYS_INCOUNT) return ModulePortType_VoltStream;
  if ((port - POLYKEYS_INCOUNT) < POLYKEYS_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < POLYKEYS_OUTCOUNT) return ModulePortType_VoltStream;
  if ((port - POLYKEYS_OUTCOUNT) < POLYKEYS_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < POLYKEYS_CONTROLCOUNT) return ModulePortType_VoltControl;
  if ((port - POLYKEYS_CONTROLCOUNT) < POLYKEYS_MIDI_CONTROLCOUNT) return ModulePortType_MIDIControl;
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
  if (id < POLYKEYS_CONTROLCOUNT) ((PolyKeys*)modPtr)->controlsCurr[id] = *(Volt*)val;
  if ((id - POLYKEYS_CONTROLCOUNT) < POLYKEYS_MIDI_CONTROLCOUNT) 
  {
    bool good = MIDI_PushRingBuffer(GET_MIDI_CONTROL_RING_BUFFER(modPtr, id - POLYKEYS_CONTROLCOUNT), *(MIDIData*)val, &(((PolyKeys*)modPtr)->midiRingWrite[id - POLYKEYS_CONTROLCOUNT]), &(((PolyKeys*)modPtr)->midiRingRead[id - POLYKEYS_CONTROLCOUNT]));
    if (!good) printf("dropped midi packet\n");
  }
}

static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id < POLYKEYS_CONTROLCOUNT) *(Volt*)ret = ((PolyKeys*)modPtr)->controlsCurr[id];
  if ((id - POLYKEYS_CONTROLCOUNT) < POLYKEYS_MIDI_CONTROLCOUNT) *(MIDIData*)ret = MIDI_PeakRingBuffer(GET_MIDI_CONTROL_RING_BUFFER(modPtr, id - POLYKEYS_CONTROLCOUNT), &(((PolyKeys*)modPtr)->midiRingRead[id - POLYKEYS_CONTROLCOUNT]));
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
    PolyKeys * mi = (PolyKeys*)modPtr;
  if (port < POLYKEYS_INCOUNT) mi->inputPorts[port] = readAddr;
  if ((port - POLYKEYS_INCOUNT) < POLYKEYS_MIDI_INCOUNT) mi->inputMIDIPorts[port - POLYKEYS_INCOUNT] = readAddr;
}

static void initAdsrInputBuffers(void)
{
    adsrInputBuffersInitDone = true;

    for (int i = 0; i < MODULE_BUFFER_SIZE; i++)
    {
        fakeAdsrStart[i] = i == 0 ? VOLTSTD_GATE_LOW : VOLTSTD_GATE_HIGH;
        fakeAdsrSustain[i] = VOLTSTD_GATE_HIGH;
        fakeAdsrStart[i] = VOLTSTD_GATE_LOW;
    }
}

static void initVoice(PolyKeysVoice* voice)
{
    voice->vco = (VCO*)VCO_init("mod");
    voice->adsr = (ADSR*)ADSR_init("mod");
    voice->flt = (Filter*)Filter_init("mod");
    voice->attn = (Attenuator*)Attenuator_init("mod");

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
        ATTN_IN_PORT_VOL,
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
        ATTN_IN_PORT_AUD,
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
        ATTN_OUT_PORT_AUD
    );
}

static void consumeMidiMessage(PolyKeys* pk)
{
    // in here we read all the midi data off the input
    // turn of/off/sustain whatever notes

    // mainly if a the voices are full and a new note is pressed then kick off the oldest one
    // if the note is turning on for the first time then set noteIsOn, adsrActive, firstOnSignal, note, startVelocity
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
}

static void applyControlValsToModules(PolyKeys* pk, PolyKeysVoice* voice)
{
    // push all the relevent control values of the full poly module to the relivent sub modules
}

static inline void assignADSRBuffer(PolyKeys* pk, PolyKeysVoice* voice)
{
    // depending on the values of noteIsOn and firstOnSignal then set the input gate buffer of the adsr
    if (voice->noteIsOn)
    {
        if (voice->firstOnSignal)
        {
            voice->adsr->inputPorts[ADSR_IN_PORT_GATE] = fakeAdsrStart;
        }
        else
        {
            voice->adsr->inputPorts[ADSR_IN_PORT_GATE] = fakeAdsrSustain;
        }
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