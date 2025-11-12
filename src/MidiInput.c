#include "MidiInput.h"

#include "AudioSettings.h"
#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define IN_VOLT_STREAM_PORT(mod, port)           (((MidiInput*)(mod))->inputPorts[port])
#define IN_MIDIDADA_PORT(mod, port)           (((MidiInput*)(mod))->inputMIDIPorts[port])

#define PREV_VOLT_STREAM_PORT_ADDR(mod, port) (((MidiInput*)(mod))->outputPortsPrev + (MODULE_BUFFER_SIZE * (port)))
#define CURR_VOLT_STREAM_PORT_ADDR(mod, port) (((MidiInput*)(mod))->outputPortsCurr + (MODULE_BUFFER_SIZE * (port)))
#define PREV_MIDIDATA_PORT_ADDR(mod, port)    (((MidiInput*)(mod))->outputMIDIPortsPrev + (port))
#define CURR_MIDIDATA_PORT_ADDR(mod, port)    (((MidiInput*)(mod))->outputMIDIPortsCurr + (port))

#define OUT_PORT_OUTPUT(m)                (CURR_MIDIDATA_PORT_ADDR(m, MIDIINPUT_MIDI_OUTPUT_OUTPUT))  

#define GET_MIDI_CONTROL_RING_BUFFER(mod, port)   (((MidiInput*)(mod))->midiControlsRingBuffer + (MIDI_STREAM_BUFFER_SIZE * (port)))

#define CONTROL_PUSH_TO_PREV(m)         for (U4 i = 0; i < MIDIINPUT_CONTROLCOUNT; i++) {((MidiInput*)(m))->controlsPrev[i] = ((MidiInput*)(m))->controlsCurr[i];} for (U4 i = 0; i < MIDIINPUT_MIDI_CONTROLCOUNT; i++)

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_midi(void * modPtr);
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

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static char * inPortNames[MIDIINPUT_INCOUNT + MIDIINPUT_MIDI_INCOUNT] = {
};

static char * outPortNames[MIDIINPUT_OUTCOUNT + MIDIINPUT_MIDI_OUTCOUNT] = {
  "MidiOut",
};

static char * controlNames[MIDIINPUT_CONTROLCOUNT + MIDIINPUT_MIDI_CONTROLCOUNT] = {
  "MidiIn",
};

static Module vtable = {
  .type = ModuleType_MidiInput,
  .freeModule = free_midi,
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

#define DEFAULT_CONTROL_RATE    1

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * MidiInput_init(char* name)
{
  MidiInput * mi = calloc(1, sizeof(MidiInput));

  // set module
  mi->module = vtable;
  mi->module.name = name;

  for (int i = 0; i < MIDIINPUT_MIDI_CONTROLCOUNT; i++)
  {
    mi->midiRingRead[i] = 0;
    mi->midiRingWrite[i] = 1;
  }

  return (Module*)mi;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_midi(void * modPtr)
{
  MidiInput * mi = (MidiInput*)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free(mi);
}

static void updateState(void * modPtr)
{
  MidiInput * mi = (MidiInput*)modPtr;

  MIDI_PopRingBuffer(mi->midiControlsRingBuffer, CURR_MIDIDATA_PORT_ADDR(mi, MIDIINPUT_MIDI_OUTPUT_OUTPUT), &(mi->midiRingWrite[MIDIINPUT_MIDI_OUTPUT_OUTPUT]), &(mi->midiRingRead[MIDIINPUT_MIDI_OUTPUT_OUTPUT]));

  /*
  */
  for (int i = 0; i < MIDI_STREAM_BUFFER_SIZE; i++)
  {
    U4 type = CURR_MIDIDATA_PORT_ADDR(mi, MIDIINPUT_MIDI_OUTPUT_OUTPUT)[i].type;
    if (type != MIDIDataType_None)
    {
      printf("%d: %02x\n", i, type);
    }
  }
}

static void pushCurrToPrev(void * modPtr)
{
  MidiInput * mi = (MidiInput*)modPtr;
  memcpy(mi->outputPortsPrev, mi->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * MIDIINPUT_OUTCOUNT);
  memcpy(mi->outputMIDIPortsPrev, mi->outputMIDIPortsCurr, sizeof(MIDIData) * MIDIINPUT_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE);
  CONTROL_PUSH_TO_PREV(mi);
}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{
  if (port < MIDIINPUT_OUTCOUNT) return PREV_VOLT_STREAM_PORT_ADDR(modPtr, port);
  if ((port - MIDIINPUT_OUTCOUNT) < MIDIINPUT_MIDI_OUTCOUNT) return PREV_MIDIDATA_PORT_ADDR(modPtr, port - MIDIINPUT_OUTCOUNT);
  return NULL;
}

static void * getInputAddr(void * modPtr, ModularPortID port)
{
  if (port >= MIDIINPUT_INCOUNT + MIDIINPUT_MIDI_INCOUNT) return NULL;

  return NULL;
}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{
  if (port < MIDIINPUT_INCOUNT) return ModulePortType_VoltStream;
  if ((port - MIDIINPUT_INCOUNT) < MIDIINPUT_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{
  if (port < MIDIINPUT_OUTCOUNT) return ModulePortType_VoltStream;
  if ((port - MIDIINPUT_OUTCOUNT) < MIDIINPUT_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{
  if (port < MIDIINPUT_CONTROLCOUNT) return ModulePortType_VoltControl;
  if ((port - MIDIINPUT_CONTROLCOUNT) < MIDIINPUT_MIDI_CONTROLCOUNT) return ModulePortType_MIDIControl;
  return ModulePortType_None;
}

static U4 getInCount(void * modPtr)
{
  return MIDIINPUT_INCOUNT + MIDIINPUT_MIDI_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return MIDIINPUT_OUTCOUNT + MIDIINPUT_MIDI_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return MIDIINPUT_CONTROLCOUNT + MIDIINPUT_MIDI_CONTROLCOUNT;
}

static void setControlVal(void * modPtr, ModularPortID id, void* val)
{
  if (id < MIDIINPUT_CONTROLCOUNT) ((MidiInput*)modPtr)->controlsCurr[id] = *(Volt*)val;
  if ((id - MIDIINPUT_CONTROLCOUNT) < MIDIINPUT_MIDI_CONTROLCOUNT) 
  {
    bool good = MIDI_PushRingBuffer(GET_MIDI_CONTROL_RING_BUFFER(modPtr, id - MIDIINPUT_CONTROLCOUNT), *(MIDIData*)val, &(((MidiInput*)modPtr)->midiRingWrite[id - MIDIINPUT_CONTROLCOUNT]), &(((MidiInput*)modPtr)->midiRingRead[id - MIDIINPUT_CONTROLCOUNT]));
    if (!good) printf("dropped midi packet\n");
  }
}

static void getControlVal(void * modPtr, ModularPortID id, void* ret)
{
  if (id < MIDIINPUT_CONTROLCOUNT) *(Volt*)ret = ((MidiInput*)modPtr)->controlsCurr[id];
  if ((id - MIDIINPUT_CONTROLCOUNT) < MIDIINPUT_MIDI_CONTROLCOUNT) *(MIDIData*)ret = MIDI_PeakRingBuffer(GET_MIDI_CONTROL_RING_BUFFER(modPtr, id - MIDIINPUT_CONTROLCOUNT), &(((MidiInput*)modPtr)->midiRingRead[id - MIDIINPUT_CONTROLCOUNT]));
}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{
  MidiInput * mi = (MidiInput*)modPtr;
  if (port < MIDIINPUT_INCOUNT) mi->inputPorts[port] = readAddr;
  if ((port - MIDIINPUT_INCOUNT) < MIDIINPUT_MIDI_INCOUNT) mi->inputMIDIPorts[port - MIDIINPUT_INCOUNT] = readAddr;
}
