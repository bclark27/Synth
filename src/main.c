#include "comm/Common.h"
#include "ModularSynth.h"
#include "AudioSettings.h"
#include "AudioDevice.h"
#include <time.h>
#include <xmmintrin.h>   // SSE
#include <pmmintrin.h>   // SSE3
#include "comm/IPC.h"
/*
ModularSynth_update();
memcpy(signalBuffers[idx], synthOutput, sizeof(R4) * STREAM_BUFFER_SIZE);
*/

#define PATH "/home/ben/projects/github/my/Synth/config/drone"
#define SCREEN_WIDTH        300
#define SCREEN_HEIGHT       300
#define FPS                 60

void timetest()
{
  ModularSynth_init();
  ModularSynth_readConfig("/home/ben/projects/github/my/Synth/config/synth1");

  struct timeval stop, start;
  gettimeofday(&start, NULL);
  
  int iters = 2000;
  for (int i = 0; i < iters; i++)
  {
    ModularSynth_update();
  }

  gettimeofday(&stop, NULL);
  long unsigned int t = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
  double s = t / 1000000.0;
  printf("took %lf seconds\n", s);
  double speed = s / iters;
  printf("seconds per iter: %lf\n", speed);
  double timeToBeat = STREAM_BUFFER_SIZE / (double)SAMPLE_RATE;
  printf("time to beat: %lf\n", timeToBeat);
  printf("went %.3lfx faster then needed\n", timeToBeat / speed);
}


ColorStates defaultPadColor(int x, int y)
{
  return ColorStates_WHITE;


  int val = x + y * 8;
  val -= (y / 2) * 3;
  
  if (y % 2 != 0)
  {
    val -= 5;
  }

  if (val % 7 == 0)
    return 24;
  
  return ColorStates_WHITE;
}

void initColors()
{
  for (int x = 0; x < 8; x++)
    for (int y = 0; y < 8; y++)
    {
      AbletonPkt_Cmd_Pad cmd_p = { 
        .x=x, 
        .y=y,
        .setColor=true,
        .color=defaultPadColor(x,y),
      };
      IPC_PostMessage(MSG_TYPE_ABL_CMD_PAD, &cmd_p, sizeof(AbletonPkt_Cmd_Pad));
    }
}

float knobs[9] = {0,0,0,0,0,0,0,0,0};

void knobHelper(char* modName, char* cvName, int knobNum, int direction, float div)
{
  int t = ModularSynth_getControlTypeByName(modName, cvName);
  if (t != ModulePortType_VoltControl) return;
  
  ModularSynth_getControlByName(modName, cvName, &knobs[knobNum]);
  knobs[knobNum] += direction / div;
  ModularSynth_setControlByName(modName, cvName, &knobs[knobNum]);

  const int spacing = 8;
  char* str = malloc(68);
  int size = snprintf(str, 68, "%.3f", knobs[knobNum]);
  AbletonPkt_Cmd_Text cmd_t =
  {
    .x=knobNum*spacing,
    .y=0,
  };
  memcpy(cmd_t.text, str, size);
  free(str);
  cmd_t.length = size;
  IPC_PostMessage(MSG_TYPE_ABL_CMD_TEXT, &cmd_t, sizeof(AbletonPkt_Cmd_Text));

  str = malloc(68);
  size = snprintf(str, 68, "%s", modName);
  AbletonPkt_Cmd_Text cmd_t2 =
  {
    .x=knobNum*spacing,
    .y=1,
  };
  memcpy(cmd_t2.text, str, size);
  free(str);
  cmd_t2.length = size;
  IPC_PostMessage(MSG_TYPE_ABL_CMD_TEXT, &cmd_t2, sizeof(AbletonPkt_Cmd_Text));

  str = malloc(68);
  size = snprintf(str, 68, "%s", cvName);
  AbletonPkt_Cmd_Text cmd_t3 =
  {
    .x=knobNum*spacing,
    .y=2,
  };
  memcpy(cmd_t3.text, str, size);
  free(str);
  cmd_t3.length = size;
  IPC_PostMessage(MSG_TYPE_ABL_CMD_TEXT, &cmd_t3, sizeof(AbletonPkt_Cmd_Text));
}

void OnPushEvent(MessageType t, void* d, MessageSize s)
{
  if (t == MSG_TYPE_ABL_BUTTON)
  {
    AbletonPkt_button* btn = d;
    if (btn->btnId == 86)
    {
      ModularSynth_exportConfig(PATH);
    }
  }

  if (t == MSG_TYPE_ABL_PAD)
  {
    AbletonPkt_pad* pad = d;

    if (pad->isRelease)
    {
      MIDIData md = {
        .type=MIDIDataType_NoteOff,
        .data1=pad->id+36,
        .data2=pad->padVelocity,
      };
      //printf("Pushing: %02x\n", md.type);
      ModularSynth_setControlByName("mdin", "MidiIn", &md);

      AbletonPkt_Cmd_Pad cmd_p = { 
        .x=pad->padX, 
        .y=pad->padY,
        .setColor=true,
        .color=defaultPadColor(pad->padX,pad->padY),
      };
      IPC_PostMessage(MSG_TYPE_ABL_CMD_PAD, &cmd_p, sizeof(AbletonPkt_Cmd_Pad));
    }
    else if (pad->isPress)
    {
      MIDIData md = {
        .type=MIDIDataType_NoteOn,
        .data1=pad->id+36,
        .data2=pad->padVelocity,
      };
      //printf("Pushing: %02x\n", md.type);
      ModularSynth_setControlByName("mdin", "MidiIn", &md);

      AbletonPkt_Cmd_Pad cmd_p = { 
        .x=pad->padX, 
        .y=pad->padY,
        .setColor=true,
        .color=ColorStates_LIGHT_GREEN,
      };
      IPC_PostMessage(MSG_TYPE_ABL_CMD_PAD, &cmd_p, sizeof(AbletonPkt_Cmd_Pad));
    }
    else if (pad->isHold)
    {
      MIDIData md = {
        .type=MIDIDataType_AfterTouch,
        .data1=pad->id+36,
        .data2=pad->padVelocity,
      };
      //ModularSynth_setControlByName("mdin", "MidiIn", &md);
    }
  }

  if (t == MSG_TYPE_ABL_BUTTON)
  {
    AbletonPkt_button* btn = d;

    AbletonPkt_Cmd_Button cmd_b = {
      .id = btn->btnId,
      .blink = btn->isPress ? BlinkStates_BlinkShot4 : BlinkStates_BlinkOff,
    };

    IPC_PostMessage(MSG_TYPE_ABL_CMD_BUTTON, &cmd_b, sizeof(AbletonPkt_Cmd_Button));
  }

  if (t == MSG_TYPE_ABL_KNOB)
  {
    AbletonPkt_knob* knob = d;
    int id = knob->id;
    int dir = knob->direction;

    switch (id)
    {
      case 0:
      {
        knobHelper("lfo", "Freq", id, dir, 100);
        break;
      }
      case 1:
      {
        knobHelper("pk", "Attack", id, dir, 100);
        break;
      }
      case 2:
      {
        knobHelper("pk", "Decay", id, dir, 100);
        break;
      }
      case 3:
      {
        knobHelper("pk", "Sustain", id, dir, 100);
        break;
      }
      case 4:
      {
        knobHelper("pk", "Release", id, dir, 100);
        break;
      }
      case 5:
      {
        knobHelper("pk", "FltFreq", id, dir, 100);
        break;
      }
      case 6:
      {
        knobHelper("pk", "Detune", id, dir, 100);
        break;
      }
      case 7:
      {
        knobHelper("pk", "Unison", id, dir, 12);
        break;
      }
      default:
      {
        break;
      }
    }
  }
}

void inputMidiChord()
{
  MIDIData md = {
      .type=MIDIDataType_NoteOn,
      .data1=0,
      .data2=100,
    };
  
  int offset = 48;
  md.data1 = 0 + offset;
  ModularSynth_setControlByName("mdin", "MidiIn", &md);
  md.data1 = 4 + offset;
  ModularSynth_setControlByName("mdin", "MidiIn", &md);
  md.data1 = 7 + offset;
  ModularSynth_setControlByName("mdin", "MidiIn", &md);
  md.data1 = 9 + offset;
  ModularSynth_setControlByName("mdin", "MidiIn", &md);
  md.data1 = 12 + offset;
  ModularSynth_setControlByName("mdin", "MidiIn", &md);
}

static inline double now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}


#define N (1 << 26)
void SIMDTesting()
{
  float *A = aligned_alloc(16, N * sizeof(float));
    float *B = aligned_alloc(16, N * sizeof(float));
    float *C = aligned_alloc(16, N * sizeof(float));

    for (size_t i = 0; i < N; i++) {
        A[i] = 0.001f * (float)(i & 0xFF);
        B[i] = 0.002f * (float)((i * 7) & 0xFF);
    }

    // --------------------------------------------------------------
    // A: Scalar C[i] = A[i] + B[i]
    // --------------------------------------------------------------
    double startA = now_ms();

    for (size_t i = 0; i < N; i++) {
        C[i] = A[i] + B[i];
    }

    double endA = now_ms();
    printf("Scalar A+B = %.3f ms\n", endA - startA);

    // --------------------------------------------------------------
    // B: SIMD C[i] = A[i] + B[i]
    // --------------------------------------------------------------
    double startB = now_ms();

    size_t i = 0;
    for (; i + 4 <= N; i += 4) {
        __m128 a = _mm_load_ps(&A[i]);
        __m128 b = _mm_load_ps(&B[i]);
        __m128 r = _mm_add_ps(a, b);
        _mm_store_ps(&C[i], r);
    }

    // Remainder
    for (; i < N; i++) {
        C[i] = A[i] + B[i];
    }

    double endB = now_ms();
    printf("SIMD A+B   = %.3f ms\n", endB - startB);

    free(A);
    free(B);
    free(C);
    return 0;
}

void ModuleTesting()
{
  const int iterCount = 100000000;
  double start;
  double end;
  start = now_ms();
  float sum = 0;
  for (int i = 0; i < iterCount; i++)
  {
    sum += MAP(0, 10, 0, 2, (float)i);
  }
  end = now_ms();
  double t0 = end - start;
  printf("A: %lf, %f\n", t0, sum);

  start = now_ms();
  sum = 0;
  for (int i = 0; i < iterCount; i++)
  {
    sum += CLAMPF(0, 10, (float)i) / 10.f;
  }
  end = now_ms();
  double t1 = end - start;
  printf("B: %lf, %f\n", t1, sum);

  return;






  const int vcoCount = 500;
  VCO vcos[vcoCount];
  float inputBuff[MODULE_BUFFER_SIZE];
  for (int i = 0; i < vcoCount; i++)
  {
    VCO_initInPlace(&vcos[i], malloc(4));
    vcos[i].module.linkToInput(&vcos[i], VCO_IN_PORT_FREQ, inputBuff);
    vcos[i].module.linkToInput(&vcos[i], VCO_IN_PORT_PW, inputBuff);
  }

  double startA = now_ms();

  for (int i = 0; i < iterCount; i++)
  {
    for (int k = 0; k < vcoCount; k++)
    {
      vcos[k].module.updateState(&vcos[k]);
      vcos[k].module.pushCurrToPrev(&vcos[k]);
    }
  }

  double endA = now_ms();
  printf("VCO time %.3f ms\n", endA - startA);
}

int main(void)
{
  // ModuleTesting();
  // return 1;
  // timetest();
  // return 0;
  IPC_StartService("Controller"); 
  IPC_ConnectToService("PushEvents", OnPushEvent);
  initColors();
  ModularSynth_init();
  ModularSynth_readConfig(PATH);
  //ModularSynth_exportConfig(PATH);
  //while (1){ModularSynth_update();}
  //inputMidiChord();
  AudioDevice_init();
  AudioDevice_LoopForever();
  return 0;

  ModularSynth_free();
}
