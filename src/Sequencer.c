#include "Sequencer.h"

#include "VoltUtils.h"

//////////////
// DEFINES  //
//////////////

#define IN_PORT_ADDR(mod, port)           (((Sequencer*)(mod))->inputPorts[port]);

#define PREV_PORT_ADDR(mod, port)         (((Sequencer*)(mod))->outputPortsPrev + MODULE_BUFFER_SIZE * (port))
#define CURR_PORT_ADDR(mod, port)         (((Sequencer*)(mod))->outputPortsCurr + MODULE_BUFFER_SIZE * (port))

#define IN_PORT_CLKIN(seq)                ((seq)->inputPorts[SEQ_IN_PORT_CLKIN])

#define OUT_PORT_GATE(seq)                (CURR_PORT_ADDR(seq, SEQ_OUT_PORT_GATE))
#define OUT_PORT_TRIG(seq)                (CURR_PORT_ADDR(seq, SEQ_OUT_PORT_TRIG))
#define OUT_PORT_PITCH(seq)               (CURR_PORT_ADDR(seq, SEQ_OUT_PORT_PITCH))

#define GET_CONTROL_CURR_NOTE_ON(seq, n)    ((seq)->controlsCurr[SEQ_CONTROL_NOTE_ON(n)])
#define GET_CONTROL_PREV_NOTE_ON(seq, n)    ((seq)->controlsPrev[SEQ_CONTROL_NOTE_ON(n)])
#define GET_CONTROL_CURR_NOTE_PITCH(seq, n) ((seq)->controlsCurr[SEQ_CONTROL_NOTE_PITCH(n)])
#define GET_CONTROL_PREV_NOTE_PITCH(seq, n) ((seq)->controlsPrev[SEQ_CONTROL_NOTE_PITCH(n)])


#define GET_CONTROL_CURR_NOTE_LEN(seq)      ((seq)->controlsCurr[SEQ_CONTROL_NOTE_LEN])
#define GET_CONTROL_PREV_NOTE_LEN(seq)      ((seq)->controlsPrev[SEQ_CONTROL_NOTE_LEN])
#define GET_CONTROL_CURR_SEQ_LEN(seq)       ((seq)->controlsCurr[SEQ_CONTROL_SEQ_LEN])
#define GET_CONTROL_PREV_SEQ_LEN(seq)       ((seq)->controlsPrev[SEQ_CONTROL_SEQ_LEN])
#define GET_CONTROL_CURR_RAND(seq)          ((seq)->controlsCurr[SEQ_CONTROL_RAND])
#define GET_CONTROL_PREV_RAND(seq)          ((seq)->controlsPrev[SEQ_CONTROL_RAND])


#define SET_CONTROL_CURR_NOTE_ON(seq, n, v)    ((seq)->controlsCurr[SEQ_CONTROL_NOTE_ON(n)] = (v))
#define SET_CONTROL_PREV_NOTE_ON(seq, n, v)    ((seq)->controlsPrev[SEQ_CONTROL_NOTE_ON(n)] = (v))
#define SET_CONTROL_CURR_NOTE_PITCH(seq, n, v) ((seq)->controlsCurr[SEQ_CONTROL_NOTE_PITCH(n)] = (v))
#define SET_CONTROL_PREV_NOTE_PITCH(seq, n, v) ((seq)->controlsPrev[SEQ_CONTROL_NOTE_PITCH(n)] = (v))

#define SET_CONTROL_CURR_NOTE_LEN(seq, v)      ((seq)->controlsCurr[SEQ_CONTROL_NOTE_LEN] = (v))
#define SET_CONTROL_PREV_NOTE_LEN(seq, v)      ((seq)->controlsPrev[SEQ_CONTROL_NOTE_LEN] = (v))
#define SET_CONTROL_CURR_SEQ_LEN(seq, v)       ((seq)->controlsCurr[SEQ_CONTROL_SEQ_LEN] = (v))
#define SET_CONTROL_PREV_SEQ_LEN(seq, v)       ((seq)->controlsPrev[SEQ_CONTROL_SEQ_LEN] = (v))
#define SET_CONTROL_CURR_RAND(seq, v)          ((seq)->controlsCurr[SEQ_CONTROL_RAND] = (v))
#define SET_CONTROL_PREV_RAND(seq, v)          ((seq)->controlsPrev[SEQ_CONTROL_RAND] = (v))

#define CONTROL_PUSH_TO_PREV(vco)         for (U4 i = 0; i < SEQ_CONTROLCOUNT; i++) {(vco)->controlsPrev[i] = (vco)->controlsCurr[i];}

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static void free_seq(void * modPtr);
static void updateState(void * modPtr);
static void pushCurrToPrev(void * modPtr);
static R4 * getOutputAddr(void * modPtr, U4 port);
static R4 * getInputAddr(void * modPtr, U4 port);
static U4 getInCount(void * modPtr);
static U4 getOutCount(void * modPtr);
static U4 getControlCount(void * modPtr);
static void setControlVal(void * modPtr, U4 id, R4 val);
static R4 getControlVal(void * modPtr, U4 id);
static void linkToInput(void * modPtr, U4 port, R4 * readAddr);

//////////////////////
//  DEFAULT VALUES  //
//////////////////////

static Module vtable = {
  .freeModule = free_seq,
  .updateState = updateState,
  .pushCurrToPrev = pushCurrToPrev,
  .getOutputAddr = getOutputAddr,
  .getInputAddr = getInputAddr,
  .getInCount = getInCount,
  .getOutCount = getOutCount,
  .getContolCount = getControlCount,
  .setControlVal = setControlVal,
  .getControlVal = getControlVal,
  .linkToInput = linkToInput,
};

#define DEFAULT_NOTE_LEN  0.1f
#define DEFAULT_SEQ_LEN   4

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Module * Sequencer_init(void)
{
  Sequencer * seq = calloc(1, sizeof(Sequencer));

  // set vtable
  seq->module = vtable;

  //set controls
  for (U4 i = 0; i < SEQ_NOTE_COUNT_TOTAL; i += 1)
  {
    SET_CONTROL_CURR_NOTE_ON(seq, i, 1);
  }

  // for (U4 i = 0; i < SEQ_NOTE_COUNT_TOTAL; i += 1)
  // {
  //   SET_CONTROL_CURR_NOTE_PITCH(seq, i, i / (R4)12);
  // }

  SET_CONTROL_CURR_NOTE_PITCH(seq, 0, 0 / (R4)12);
  SET_CONTROL_CURR_NOTE_PITCH(seq, 1, 4 / (R4)12);
  SET_CONTROL_CURR_NOTE_PITCH(seq, 2, 7 / (R4)12);
  SET_CONTROL_CURR_NOTE_PITCH(seq, 3, 9 / (R4)12);


  SET_CONTROL_CURR_NOTE_LEN(seq, DEFAULT_NOTE_LEN);
  SET_CONTROL_CURR_SEQ_LEN(seq, DEFAULT_SEQ_LEN);




  seq->currentStepNum = 0;
  seq->gateOpenTime = 0;
  seq->clockIsHigh = 0;
  seq->gateHigh = 0;

  // push curr to prev
  CONTROL_PUSH_TO_PREV(seq);


  return (Module*)seq;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static void free_seq(void * modPtr)
{
  free(modPtr);
}

static void updateState(void * modPtr)
{
  Sequencer * seq = (Sequencer*)modPtr;
  U4 sequenceLen = MAX(MIN(SEQ_NOTE_COUNT_TOTAL, GET_CONTROL_CURR_SEQ_LEN(seq)), 1);

  for (U4 i = 0; i < MODULE_BUFFER_SIZE; i++)
  {

    // zero all outputs, only turn on if passed the tests bellow
    OUT_PORT_GATE(seq)[i] = VOLTSTD_GATE_LOW;
    OUT_PORT_TRIG(seq)[i] = VOLTSTD_GATE_LOW;
    OUT_PORT_PITCH(seq)[i] = VOLTSTD_C3_VOLTAGE;

    bool risingEdge = 0;
    bool isHigh = IN_PORT_CLKIN(seq) ? IN_PORT_CLKIN(seq)[i] > VOLTSTD_GATE_HIGH_THRESH : 0;

    // read clock in
    if (!seq->clockIsHigh && isHigh)
    { risingEdge = 1; }
    else
    { risingEdge = 0; }

    seq->clockIsHigh = isHigh;


    // if the clock has now risen
    if (risingEdge)
    {
      seq->currentStepNum = (seq->currentStepNum + 1) % sequenceLen;
    }

    if (!GET_CONTROL_CURR_NOTE_ON(seq, seq->currentStepNum)) continue;

    if (risingEdge)
    {
      OUT_PORT_TRIG(seq)[i] = VOLTSTD_GATE_HIGH;
      OUT_PORT_GATE(seq)[i] = VOLTSTD_GATE_HIGH;
      seq->gateOpenTime = 0;
      seq->gateHigh = 1;
    }

    if (!seq->gateHigh) continue;

    seq->gateOpenTime += SEC_PER_SAMPLE;

    if (seq->gateOpenTime >= GET_CONTROL_CURR_NOTE_LEN(seq))
    {
      seq->gateHigh = 0;
      continue;
    }

    OUT_PORT_GATE(seq)[i] = VOLTSTD_GATE_HIGH;
    OUT_PORT_PITCH(seq)[i] = GET_CONTROL_CURR_NOTE_PITCH(seq, seq->currentStepNum);

  }

  // push curr to prev
  CONTROL_PUSH_TO_PREV(seq);

}

static void pushCurrToPrev(void * modPtr)
{
  Sequencer * seq = (Sequencer *)modPtr;
  memcpy(seq->outputPortsPrev, seq->outputPortsCurr, sizeof(R4) * MODULE_BUFFER_SIZE * SEQ_OUTCOUNT);
}

static R4 * getOutputAddr(void * modPtr, U4 port)
{
  if (port >= SEQ_OUTCOUNT) return NULL;

  return PREV_PORT_ADDR(modPtr, port);
}

static R4 * getInputAddr(void * modPtr, U4 port)
{
  if (port >= SEQ_INCOUNT) return NULL;

  return IN_PORT_ADDR(modPtr, port);
}

static U4 getInCount(void * modPtr)
{
  return SEQ_INCOUNT;
}

static U4 getOutCount(void * modPtr)
{
  return SEQ_OUTCOUNT;
}

static U4 getControlCount(void * modPtr)
{
  return SEQ_CONTROLCOUNT;
}

static void setControlVal(void * modPtr, U4 id, R4 val)
{
  if (id >= SEQ_CONTROLCOUNT) return;

  Sequencer * seq = (Sequencer*)modPtr;
  seq->controlsCurr[id] = val;
}

static R4 getControlVal(void * modPtr, U4 id)
{
  if (id >= SEQ_CONTROLCOUNT) return 0;

  Sequencer * seq = (Sequencer*)modPtr;
  return seq->controlsCurr[id];
}

static void linkToInput(void * modPtr, U4 port, R4 * readAddr)
{
  if (port >= SEQ_INCOUNT) return;

  Sequencer * seq = (Sequencer *)modPtr;
  seq->inputPorts[port] = readAddr;
}
