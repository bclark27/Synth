
#include "raylib/raylib.h"
#include "comm/Common.h"
#include "ModularSynth.h"
#include "AudioSettings.h"
#include "comm/IPC.h"
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sys/time.h>

/*
ModularSynth_update();
memcpy(signalBuffers[idx], synthOutput, sizeof(R4) * STREAM_BUFFER_SIZE);
*/


#define SCREEN_WIDTH        300
#define SCREEN_HEIGHT       300
#define FPS                 60
#define NUM_BUFFERS         2    // 3-lookahead

typedef struct {
  float buffers[NUM_BUFFERS][STREAM_BUFFER_SIZE];
  _Atomic int bufferReadyCount;
  _Atomic int writeIndex;
  _Atomic int readIndex;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int running;
  float phase;
} SynthData;

SynthData synth;


void* synthThread(void* arg) {
  SynthData* s = (SynthData*)arg;
  R4* synthOutput = ModularSynth_getLeftChannel();
  while (s->running) {
    // Wait until there is room to write
    pthread_mutex_lock(&s->mutex);
    while (s->bufferReadyCount == NUM_BUFFERS)
      pthread_cond_wait(&s->cond, &s->mutex);

    int idx = s->writeIndex;
    pthread_mutex_unlock(&s->mutex);

    ModularSynth_update();
    memcpy(s->buffers[idx], synthOutput, sizeof(R4) * STREAM_BUFFER_SIZE);

    // Mark buffer ready
    pthread_mutex_lock(&s->mutex);
    s->bufferReadyCount++;
    s->writeIndex = (s->writeIndex + 1) % NUM_BUFFERS;
    pthread_cond_signal(&s->cond);
    pthread_mutex_unlock(&s->mutex);
  }

  return NULL;
}

void timetest()
{
  ModularSynth_init();
  ModularSynth_readConfig("/home/ben/projects/github/my/Synth/config/synth1");

  struct timeval stop, start;
  gettimeofday(&start, NULL);
  
  int iters = 1000;
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

float osc1freq = 0;
float osc2freq = 0;

void OnPushEvent(MessageType t, void* d, MessageSize s)
{

  if (t == MSG_TYPE_ABL_PAD)
  {
    AbletonPkt_pad* pad = d;

    if (pad->isRelease)
    {
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
      
      AbletonPkt_Cmd_Pad cmd_p = { 
        .x=pad->padX, 
        .y=pad->padY,
        .setColor=true,
        .color=ColorStates_LIGHT_GREEN,
      };
      IPC_PostMessage(MSG_TYPE_ABL_CMD_PAD, &cmd_p, sizeof(AbletonPkt_Cmd_Pad));
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
    float div = 36.0f;
    if (knob->id == 0)
    {
      osc1freq += knob->direction / div;
      ModularSynth_setControlByName("osc1", "Freq", osc1freq);

      char* str = malloc(68);
      int size = snprintf(str, 68, "%.3f", osc1freq);
      AbletonPkt_Cmd_Text cmd_t =
      {
        .x=0,
        .y=0,
      };
      memcpy(cmd_t.text, str, size);
      cmd_t.length = size;
      IPC_PostMessage(MSG_TYPE_ABL_CMD_TEXT, &cmd_t, sizeof(AbletonPkt_Cmd_Text));
    }

    if (knob->id == 1)
    {
      osc2freq += knob->direction / div;
      ModularSynth_setControlByName("osc2", "Freq", osc2freq);

      char* str = malloc(68);
      int size = snprintf(str, 68, "%.3f", osc2freq);
      AbletonPkt_Cmd_Text cmd_t =
      {
        .x=9,
        .y=0,
      };
      memcpy(cmd_t.text, str, size);
      cmd_t.length = size;
      IPC_PostMessage(MSG_TYPE_ABL_CMD_TEXT, &cmd_t, sizeof(AbletonPkt_Cmd_Text));
    }
  }
}


int main(void)
{
  // timetest();
  // return 0;

  IPC_StartService("Controller"); 
  IPC_ConnectToService("PushEvents", OnPushEvent);
  
  initColors();
  
  ModularSynth_init();
  ModularSynth_readConfig("/home/ben/projects/github/my/Synth/config/synth2");
  
  InitAudioDevice();

  SetAudioStreamBufferSizeDefault(STREAM_BUFFER_SIZE);
  AudioStream synthStream = LoadAudioStream(
    SAMPLE_RATE,
    sizeof(float) * 8,
    1
  );

  SetMasterVolume(0.5);
  PlayAudioStream(synthStream);

  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Synth");
  SetTargetFPS(FPS);

  BeginDrawing();
  ClearBackground(BLACK);
  EndDrawing();

  pthread_mutex_init(&synth.mutex, NULL);
  pthread_cond_init(&synth.cond, NULL);
  synth.bufferReadyCount = 0;
  synth.writeIndex = 0;
  synth.readIndex = 0;
  synth.running = 1;
  synth.phase = 0;

  pthread_t thread;
  pthread_create(&thread, NULL, synthThread, &synth);

  while (!WindowShouldClose()) 
  {
    // Feed audio stream if ready
    if (IsAudioStreamProcessed(synthStream))
    {
      pthread_mutex_lock(&synth.mutex);
      while (synth.bufferReadyCount == 0)
          pthread_cond_wait(&synth.cond, &synth.mutex);

      int idx = synth.readIndex;
      synth.bufferReadyCount--;
      synth.readIndex = (synth.readIndex + 1) % NUM_BUFFERS;

      pthread_cond_signal(&synth.cond);
      pthread_mutex_unlock(&synth.mutex);

      UpdateAudioStream(synthStream, synth.buffers[idx], STREAM_BUFFER_SIZE);
    }
  }

  synth.running = 0;
  pthread_cond_broadcast(&synth.cond); // wake synth thread
  pthread_join(thread, NULL);
  pthread_mutex_destroy(&synth.mutex);
  pthread_cond_destroy(&synth.cond);

  UnloadAudioStream(synthStream);
  CloseAudioDevice();
  CloseWindow();

  ModularSynth_free();
}
