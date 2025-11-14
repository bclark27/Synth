#include "comm/Common.h"
#include "ModularSynth.h"
#include "AudioSettings.h"
#include "AudioDevice.h"
#include "comm/IPC.h"
/*
ModularSynth_update();
memcpy(signalBuffers[idx], synthOutput, sizeof(R4) * STREAM_BUFFER_SIZE);
*/

#define PATH "/home/ben/projects/github/my/Synth/config/synth3"
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
        knobHelper("pk", "Attack", id, dir, 100);
        break;
      }
      case 1:
      {
        knobHelper("pk", "Decay", id, dir, 100);
        break;
      }
      case 2:
      {
        knobHelper("pk", "Sustain", id, dir, 100);
        break;
      }
      case 3:
      {
        knobHelper("pk", "Release", id, dir, 100);
        break;
      }
      case 4:
      {
        knobHelper("pk", "FltEnvAmt", id, dir, 100);
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
        knobHelper("pk", "Unison", id, dir, 100);
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
  initColors();
  ModularSynth_init();
  ModularSynth_readConfig(PATH);
  AudioDevice_init();
  AudioDevice_LoopForever();
  return 0;

  ModularSynth_free();
}
