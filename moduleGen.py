# module_defines_generator.py

# Configuration for your module
module_name = "Sampler"
module_name_u = module_name.upper()
module_name_l = module_name.lower()

inputs = ["Pitch", "Gate", "Attack", "Decay", "Sustain", "Release", "RecordGate"]
outputs = ["Audio"]
inputs_m = ["Midi"]
outputs_m = []
controls = ["Pitch", "Attack", "Decay", "Sustain", "Release", "Source", "PlaybackMode", "PlaybackStart", "LoopStart", "LoopEnd"]
controls_b = ["File"]
controls_m = []

module_buffer_size = "MODULE_BUFFER_SIZE"  # keep as define
module_midi_buffer_size = "MIDI_STREAM_BUFFER_SIZE"  # keep as define

print(f"#ifndef {module_name_u}_H_\n#define {module_name_u}_H_\n\n#include \"Module.h\"\n\n")

print(f"#define {module_name_u}_INCOUNT {len(inputs)}")
print(f"#define {module_name_u}_OUTCOUNT {len(outputs)}")
print(f"#define {module_name_u}_CONTROLCOUNT {len(controls)}")
print(f"#define {module_name_u}_BYTE_CONTROLCOUNT {len(controls_b)}")
print(f"#define {module_name_u}_MIDI_CONTROLCOUNT {len(controls_m)}")


# Generate IN_PORT macros
for i, inp in enumerate(inputs):
    print(f"#define {module_name_u}_IN_PORT_{inp}\t{i}")
print()
for i, out in enumerate(outputs):
    print(f"#define {module_name_u}_OUT_PORT_{out}\t{i}")
print()
for i, ctrl in enumerate(controls):
    print(f"#define {module_name_u}_CONTROL_{ctrl}\t{i}")
print()
for i, ctrl in enumerate(controls_b):
    print(f"#define {module_name_u}_BYTE_CONTROL_{ctrl}\t{i}")
print()
for i, ctrl in enumerate(controls_m):
    print(f"#define {module_name_u}_MIDI_CONTROL_{ctrl}\t{i}")
print()
print(f"#define {module_name_u}_MIDI_INCOUNT {len(inputs_m)}")
print(f"#define {module_name_u}_MIDI_OUTCOUNT {len(outputs_m)}")
print()
for i, inp in enumerate(inputs_m):
    print(f"#define {module_name_u}_IN_MIDI_PORT_{inp}\t{i}")
print()
for i, out in enumerate(outputs_m):
    print(f"#define {module_name_u}_OUT_MIDI_PORT_{out}\t{i}")


print(f'''

///////////
// TYPES //
///////////

typedef struct {module_name}
{{
  Module module;

  // input ports
  R4 * inputPorts[{module_name_u}_INCOUNT];

  // output ports
  R4 outputPortsPrev[MODULE_BUFFER_SIZE * {module_name_u}_OUTCOUNT];
  R4 outputPortsCurr[MODULE_BUFFER_SIZE * {module_name_u}_OUTCOUNT];

  R4 controlsCurr[{module_name_u}_CONTROLCOUNT];
  R4 controlsPrev[{module_name_u}_CONTROLCOUNT];

  // input ports
  MIDIData* inputMIDIPorts[{module_name_u}_MIDI_INCOUNT];

  // output ports
  MIDIData outputMIDIPortsPrev[{module_name_u}_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  MIDIData outputMIDIPortsCurr[{module_name_u}_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE];
  
  atomic_uint midiRingRead[{module_name_u}_MIDI_CONTROLCOUNT];
  atomic_uint midiRingWrite[{module_name_u}_MIDI_CONTROLCOUNT];
  MIDIData midiControlsRingBuffer[{module_name_u}_MIDI_CONTROLCOUNT * MIDI_STREAM_BUFFER_SIZE];

  // byte array type storage
  atomic_bool byteArrayLock[{module_name_u}_BYTE_CONTROLCOUNT];
  volatile bool byteArrayDirty[{module_name_u}_BYTE_CONTROLCOUNT];
  volatile unsigned int byteArrayLen[{module_name_u}_BYTE_CONTROLCOUNT];
  char* byteArrayControlStorage[{module_name_u}_BYTE_CONTROLCOUNT];
}} {module_name};

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Module * {module_name}_init(char* name);
void {module_name}_initInPlace({module_name} * {module_name_l}, char* name);

#endif


''')















print(f"\n\n\n\n\n\n\n\n#include \"{module_name}.h\"\n")

print(f"#define IN_PORT_ADDR(mod, port)\t\t((({module_name}*)(mod))->inputPorts[port]);")
print(f"#define PREV_PORT_ADDR(mod, port)\t((({module_name}*)(mod))->outputPortsPrev + {module_buffer_size} * (port))")
print(f"#define CURR_PORT_ADDR(mod, port)\t((({module_name}*)(mod))->outputPortsCurr + {module_buffer_size} * (port))\n")

print(f"#define IN_MIDI_PORT_ADDR(mod, port)\t\t((({module_name}*)(mod))->inputMIDIPorts[port])")
print(f"#define PREV_MIDI_PORT_ADDR(mod, port)\t((({module_name}*)(mod))->outputMIDIPortsPrev + {module_midi_buffer_size} * (port))")
print(f"#define CURR_MIDI_PORT_ADDR(mod, port)\t((({module_name}*)(mod))->outputMIDIPortsCurr + {module_midi_buffer_size} * (port))\n")

# Individual port pointers
for inp in inputs:
    print(f"#define IN_PORT_{inp}({module_name_l})\t\t(({module_name_l})->inputPorts[{module_name_u}_IN_PORT_{inp}])")
for out in outputs:
    print(f"#define OUT_PORT_{out}({module_name_l})\t\t(CURR_PORT_ADDR({module_name_l}, {module_name_u}_OUT_PORT_{out}))")
print()

for inp in inputs_m:
    print(f"#define IN_MIDI_PORT_{inp}({module_name_l})\t\t(({module_name_l})->inputPorts[{module_name_u}_IN_PORT_{inp}])")
for out in outputs_m:
    print(f"#define OUT_MIDI_PORT_{out}({module_name_l})\t\t(CURR_MIDI_PORT_ADDR({module_name_l}, {module_name_u}_OUT_PORT_{out}))")


# Control getters
for ctrl in controls:
    print(f"#define GET_CONTROL_CURR_{ctrl}({module_name_l})\t(({module_name_l})->controlsCurr[{module_name_u}_CONTROL_{ctrl}])")
    print(f"#define GET_CONTROL_PREV_{ctrl}({module_name_l})\t(({module_name_l})->controlsPrev[{module_name_u}_CONTROL_{ctrl}])")
print()

# Control setters
for ctrl in controls:
    print(f"#define SET_CONTROL_CURR_{ctrl}({module_name_l}, v)\t(({module_name_l})->controlsCurr[{module_name_u}_CONTROL_{ctrl}] = (v))")
    print(f"#define SET_CONTROL_PREV_{ctrl}({module_name_l}, v)\t(({module_name_l})->controlsPrev[{module_name_u}_CONTROL_{ctrl}] = (v))")


print()
print()
print(f"#define GET_MIDI_CONTROL_RING_BUFFER(mod, port)   ((({module_name}*)(mod))->midiControlsRingBuffer + (MIDI_STREAM_BUFFER_SIZE * (port)))")

print()

print(f'''
#define CONTROL_PUSH_TO_PREV({module_name_l})         for (U4 i = 0; i < {module_name_u}_CONTROLCOUNT; i++) {{({module_name_l})->controlsPrev[i] = ({module_name_l})->controlsCurr[i];}}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_{module_name_l}(void * modPtr);
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



//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static bool tableInitDone = false;
''')
# static char * inPortNames[VCO_INCOUNT] = {
#   "Freq",
#   "PW",
# };

# static char * outPortNames[VCO_OUTCOUNT] = {
#   "Audio",
# };

# static char * controlNames[VCO_CONTROLCOUNT] = {
#   "Freq",
#   "PW",
#   "Waveform",
#   "Unison",
#   "Detune",
# };


print(f"static char * inPortNames[{module_name_u}_INCOUNT + {module_name_u}_MIDI_INCOUNT] = {{")
for inp in inputs:
    print(f"\t\"{inp}\",")
for inp in inputs_m:
    print(f"\t\"{inp}\",")
print(f"}};")

print(f"static char * outPortNames[{module_name_u}_OUTCOUNT + {module_name_u}_MIDI_INCOUNT] = {{")
for inp in outputs:
    print(f"\t\"{inp}\",")
for inp in outputs_m:
    print(f"\t\"{inp}\",")
print(f"}};")

print(f"static char * controlNames[{module_name_u}_CONTROLCOUNT + {module_name_u}_MIDI_CONTROLCOUNT + {module_name_u}_BYTE_CONTROLCOUNT] = {{")
for inp in controls:
    print(f"\t\"{inp}\",")
for inp in controls_m:
  print(f"\t\"{inp}\",")
for inp in controls_b:
    print(f"\t\"{inp}\",")
print(f"}};")


print(f'''
static Module vtable = {{
  .type = ModuleType_{module_name},
  .freeModule = free_{module_name_l},
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
}};
''')

for inp in controls:
    print(f"#define DEFAULT_CONTROL_{inp}    0")


print(f'''
//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////


void {module_name}_initInPlace({module_name}* {module_name_l}, char* name)
{{
  if (!tableInitDone)
  {{
    initTables();
    tableInitDone = true;
  }}

  memset({module_name_l}, 0, sizeof({module_name}));

  // set vtable
  {module_name_l}->module = vtable;

  // set name of module
  {module_name_l}->module.name = name;
  
  // set all control values
''')

for i in controls:
    print(f"\tSET_CONTROL_CURR_{i}({module_name_l}, DEFAULT_CONTROL_{i});")

print(f'''
  // push curr to prev
  CONTROL_PUSH_TO_PREV({module_name_l});

  for (int i = 0; i < {module_name_u}_MIDI_CONTROLCOUNT; i++)
  {{
    {module_name_l}->midiRingRead[i] = 0;
    {module_name_l}->midiRingWrite[i] = 0;
  }}

}}



Module * {module_name}_init(char* name)
{{
  {module_name} * {module_name_l} = ({module_name}*)calloc(1, sizeof({module_name}));

  {module_name}_initInPlace({module_name_l}, name);
  return (Module*){module_name_l};
}}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_{module_name_l}(void * modPtr)
{{
  {module_name} * {module_name_l} = ({module_name} *)modPtr;
  
  Module * mod = (Module*)modPtr;
  free(mod->name);
  
  free({module_name_l});
}}

static void updateState(void * modPtr)
{{
    {module_name} * {module_name_l} = ({module_name}*)modPtr;
}}


static void pushCurrToPrev(void * modPtr)
{{
  {module_name} * mi = ({module_name}*)modPtr;
  memcpy(mi->outputPortsPrev, mi->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * {module_name_u}_OUTCOUNT);
  memcpy(mi->outputMIDIPortsPrev, mi->outputMIDIPortsCurr, sizeof(MIDIData) * {module_name_u}_MIDI_OUTCOUNT * MIDI_STREAM_BUFFER_SIZE);
  CONTROL_PUSH_TO_PREV(mi);
}}

static void * getOutputAddr(void * modPtr, ModularPortID port)
{{
  if (port < {module_name_u}_OUTCOUNT) return PREV_PORT_ADDR(modPtr, port);
  else if ((port - {module_name_u}_OUTCOUNT) < {module_name_u}_MIDI_OUTCOUNT) return PREV_MIDI_PORT_ADDR(modPtr, port - {module_name_u}_OUTCOUNT);

  return NULL;
}}

static void * getInputAddr(void * modPtr, ModularPortID port)
{{
    if (port < {module_name_u}_INCOUNT) {{return IN_PORT_ADDR(modPtr, port);}}
    else if ((port - {module_name_u}_INCOUNT) < {module_name_u}_MIDI_INCOUNT) return IN_MIDI_PORT_ADDR(modPtr, port - {module_name_u}_INCOUNT);

  return NULL;
}}

static ModulePortType getInputType(void * modPtr, ModularPortID port)
{{
  if (port < {module_name_u}_INCOUNT) return ModulePortType_VoltStream;
  else if ((port - {module_name_u}_INCOUNT) < {module_name_u}_MIDI_INCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}}

static ModulePortType getOutputType(void * modPtr, ModularPortID port)
{{
  if (port < {module_name_u}_OUTCOUNT) return ModulePortType_VoltStream;
  else if ((port - {module_name_u}_OUTCOUNT) < {module_name_u}_MIDI_OUTCOUNT) return ModulePortType_MIDIStream;
  return ModulePortType_None;
}}

static ModulePortType getControlType(void * modPtr, ModularPortID port)
{{
  if (port < {module_name_u}_CONTROLCOUNT) return ModulePortType_VoltControl;
  else if ((port - {module_name_u}_CONTROLCOUNT) < {module_name_u}_MIDI_CONTROLCOUNT) return ModulePortType_MIDIControl;
  else if ((port - {module_name_u}_CONTROLCOUNT - {module_name_u}_MIDI_CONTROLCOUNT) < {module_name_u}_BYTE_CONTROLCOUNT) return ModulePortType_ByteArrayControl;
  return ModulePortType_None;
}}

static U4 getInCount(void * modPtr)
{{
  return {module_name_u}_INCOUNT + {module_name_u}_MIDI_INCOUNT;
}}

static U4 getOutCount(void * modPtr)
{{
  return {module_name_u}_OUTCOUNT + {module_name_u}_MIDI_OUTCOUNT;
}}

static U4 getControlCount(void * modPtr)
{{
  return {module_name_u}_CONTROLCOUNT + {module_name_u}_MIDI_CONTROLCOUNT + {module_name_u}_BYTE_CONTROLCOUNT;
}}


static void setControlVal(void * modPtr, ModularPortID id, void* val, unsigned int len)
{{
    if (id < {module_name_u}_CONTROLCOUNT)
  {{
    Volt v = *(Volt*)val;
    switch (id)
    {{''')

for c in controls:
    print(f'''
        case {module_name_u}_CONTROL_{c}:
        v = CLAMPF(VOLTSTD_MOD_CV_MIN, VOLTSTD_MOD_CV_MAX, v);
        break;
    ''')
      
print(f'''
      default:
        break;
    }}

    (({module_name}*)modPtr)->controlsCurr[id] = v;
  }}

  else if ((id - {module_name_u}_CONTROLCOUNT) < {module_name_u}_MIDI_CONTROLCOUNT) 
  {{
    int p = id - {module_name_u}_CONTROLCOUNT;
    bool good = MIDI_PushRingBuffer(GET_MIDI_CONTROL_RING_BUFFER(modPtr, p), *(MIDIData*)val, &((({module_name}*)modPtr)->midiRingWrite[p]), &((({module_name}*)modPtr)->midiRingRead[p]));
    if (!good) printf("dropped midi packet\\n");
  }}
  else if ((id - ({module_name_u}_CONTROLCOUNT + {module_name_u}_MIDI_CONTROLCOUNT)) < {module_name_u}_BYTE_CONTROLCOUNT)
  {{
    int p = id - ({module_name_u}_CONTROLCOUNT + {module_name_u}_MIDI_CONTROLCOUNT);
    {module_name}* s = ({module_name}*)modPtr;
    AtomicHelpers_TryGetLockSpin(&s->byteArrayLock[p]);

    if (val && len)
    {{
      char* newBuff = malloc(len);
      memcpy(newBuff, val, len);
      s->byteArrayDirty[p] = 1;
      if (s->byteArrayControlStorage[p])
      {{
          free(s->byteArrayControlStorage[p]);
      }}
      s->byteArrayControlStorage[p] = newBuff;
      s->byteArrayLen[p] = len;
    }}
    else
    {{
      s->byteArrayControlStorage[p] = NULL;
      s->byteArrayLen[p] = 0;
      s->byteArrayDirty[p] = 1;
    }}

    AtomicHelpers_FreeLock(&s->byteArrayLock[p]);
  }}
}}


static void getControlVal(void * modPtr, ModularPortID id, void* ret, unsigned int* len)
{{
  if (id < {module_name_u}_CONTROLCOUNT)
  {{
    *len = sizeof(Volt);
    *(Volt*)ret = (({module_name}*)modPtr)->controlsCurr[id];
  }}
  else if ((id - {module_name_u}_CONTROLCOUNT) < {module_name_u}_MIDI_CONTROLCOUNT)
  {{
    ModularPortID port = id - {module_name_u}_CONTROLCOUNT;
    *len = sizeof(MIDIData);  
    *(MIDIData*)ret = MIDI_PeakRingBuffer(GET_MIDI_CONTROL_RING_BUFFER(modPtr, port), &((({module_name}*)modPtr)->midiRingRead[port]));
  }}
  else if ((id - ({module_name_u}_CONTROLCOUNT + {module_name_u}_MIDI_CONTROLCOUNT)) < {module_name_u}_BYTE_CONTROLCOUNT)
  {{
    ModularPortID port = id - ({module_name_u}_CONTROLCOUNT + {module_name_u}_MIDI_CONTROLCOUNT);
    {module_name}* s = ({module_name}*)modPtr;
    AtomicHelpers_TryGetLockSpin(&s->byteArrayLock[port]);
    *len = s->byteArrayLen[port];
    if (s->byteArrayControlStorage[port])
    {{
      memcpy(ret, s->byteArrayControlStorage[port], *len);
    }}
    AtomicHelpers_FreeLock(&s->byteArrayLock[port]);
  }}
}}

static void linkToInput(void * modPtr, ModularPortID port, void * readAddr)
{{
  {module_name} * mi = ({module_name}*)modPtr;
  if (port < {module_name_u}_INCOUNT) mi->inputPorts[port] = (R4*)readAddr;
  else if ((port - {module_name_u}_INCOUNT) < {module_name_u}_MIDI_INCOUNT) mi->inputMIDIPorts[port - {module_name_u}_INCOUNT] = (MIDIData*)readAddr;
}}

static void initTables()
{{
    
}}

''')
