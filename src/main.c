#include "comm/Common.h"
#include "ModularSynth.h"
#include "AudioSettings.h"
#include "AudioDevice.h"
#include "comm/IPC.h"

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

float knobs[9] = {0,0,0,0,0,0,0,0,0};

void knobHelper(char* modName, char* cvName, int knobNum, int direction, float div)
{
  knobs[knobNum] = ModularSynth_getControlByName(modName, cvName);
  knobs[knobNum] += direction / div;
  ModularSynth_setControlByName(modName, cvName, knobs[knobNum]);

  char* str = malloc(68);
  int size = snprintf(str, 68, "%.3f", knobs[knobNum]);
  AbletonPkt_Cmd_Text cmd_t =
  {
    .x=knobNum*9,
    .y=0,
  };
  memcpy(cmd_t.text, str, size);
  free(str);
  cmd_t.length = size;
  IPC_PostMessage(MSG_TYPE_ABL_CMD_TEXT, &cmd_t, sizeof(AbletonPkt_Cmd_Text));
}

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
    int id = knob->id;
    int dir = knob->direction;

    switch (id)
    {
      case 0:
      {
        knobHelper("osc1", "Freq", id, dir, 100);
        break;
      }
      case 1:
      {
        knobHelper("osc1", "Waveform", id, dir, 10);
        break;
      }
      case 2:
      {
        knobHelper("osc1", "Unison", id, dir, 10);
        break;
      }
      case 3:
      {
        knobHelper("osc1", "Detune", id, dir, 1000);
        break;
      }
      case 4:
      {
        knobHelper("f1", "Freq", id, dir, 10);
        break;
      }
      case 5:
      {
        knobHelper("f1", "Q", id, dir, 10);
        break;
      }
      default:
      {
        break;
      }
    }
  }
}


int main(void)
{
  // timetest();
  // return 0;
  IPC_StartService("Controller"); 
  IPC_ConnectToService("PushEvents", OnPushEvent);
  ModularSynth_init();
  ModularSynth_readConfig("/home/ben/projects/github/my/Synth/config/synth2");
  AudioDevice_init();
  AudioDevice_LoopForever();
  return 0;

  ModularSynth_free();
}
