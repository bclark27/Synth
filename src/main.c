
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

float knob0 = 0;
float knob1 = 0;
float knob2 = 0;
float knob3 = 0;

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
      knob0 += knob->direction / div;
      ModularSynth_setControlByName("osc1", "Freq", knob0);

      char* str = malloc(68);
      int size = snprintf(str, 68, "%.3f", knob0);
      AbletonPkt_Cmd_Text cmd_t =
      {
        .x=knob->id*9,
        .y=0,
      };
      memcpy(cmd_t.text, str, size);
      cmd_t.length = size;
      IPC_PostMessage(MSG_TYPE_ABL_CMD_TEXT, &cmd_t, sizeof(AbletonPkt_Cmd_Text));
    }

    if (knob->id == 1)
    {
      knob1 += knob->direction / div;
      ModularSynth_setControlByName("osc1", "Waveform", knob1);

      char* str = malloc(68);
      int size = snprintf(str, 68, "%.3f", knob1);
      AbletonPkt_Cmd_Text cmd_t =
      {
        .x=knob->id*9,
        .y=0,
      };
      memcpy(cmd_t.text, str, size);
      cmd_t.length = size;
      IPC_PostMessage(MSG_TYPE_ABL_CMD_TEXT, &cmd_t, sizeof(AbletonPkt_Cmd_Text));
    }

    if (knob->id == 2)
    {
      knob2 += knob->direction / div;
      ModularSynth_setControlByName("osc1", "Unison", knob2);

      char* str = malloc(68);
      int size = snprintf(str, 68, "%.3f", knob2);
      AbletonPkt_Cmd_Text cmd_t =
      {
        .x=knob->id*9,
        .y=0,
      };
      memcpy(cmd_t.text, str, size);
      cmd_t.length = size;
      IPC_PostMessage(MSG_TYPE_ABL_CMD_TEXT, &cmd_t, sizeof(AbletonPkt_Cmd_Text));
    }

    if (knob->id == 3)
    {
      knob3 += knob->direction / div;
      ModularSynth_setControlByName("osc1", "Detune", knob3);

      char* str = malloc(68);
      int size = snprintf(str, 68, "%.3f", knob3);
      AbletonPkt_Cmd_Text cmd_t =
      {
        .x=knob->id*9,
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

  R4 * audioBuffer = ModularSynth_getLeftChannel();

  while (!WindowShouldClose()) 
  {
    if (IsAudioStreamProcessed(synthStream))
    {
      UpdateAudioStream(synthStream, audioBuffer, STREAM_BUFFER_SIZE);
      ModularSynth_update();
    }
  }

  UnloadAudioStream(synthStream);
  CloseAudioDevice();
  CloseWindow();

  ModularSynth_free();
}
