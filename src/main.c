
#include "raylib/raylib.h"
#include "comm/Common.h"
#include "ModularSynth.h"
#include "AudioSettings.h"
#include "comm/IPC.h"
#include <math.h>

#define SCREEN_WIDTH        300
#define SCREEN_HEIGHT       300
#define FPS                 60
ModularID vco1;
ModularID vco2;
ModularSynth * synth;
float vco1freq = 0;
float vco2freq = 0;


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
      vco1freq += knob->direction / div;
      ModularSynth_setControl(vco1, VCO_CONTROL_FREQ, vco1freq);

      char* str = malloc(68);
      int size = snprintf(str, 68, "%.3f", vco1freq);
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
      vco2freq += knob->direction / div;
      ModularSynth_setControl(vco2, VCO_CONTROL_FREQ, vco2freq);

      char* str = malloc(68);
      int size = snprintf(str, 68, "%.3f", vco2freq);
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



FullConfig cfg;

int main(void)
{
  /*
  ConfigParser_Parse(&cfg, "/home/ben/projects/github/my/Synth/config/synth2");
  ConfigParser_Print(&cfg);
  ConfigParser_Write(&cfg, "/home/ben/projects/github/my/Synth/config/synth3");
  return 0;
  
  
  */

 
 
  IPC_StartService("Controller"); 
  IPC_ConnectToService("PushEvents", OnPushEvent);
  
  initColors();
  
  synth = ModularSynth_init();
  ModularSynth_readConfig("/home/ben/projects/github/my/Synth/config/synth1");
  //return 0;
 
  // ModularID vco0 = ModularSynth_addModuleByName(synth, "VCO", strdup("vco0"));
  // ModularID attn0 = ModularSynth_addModuleByName(synth, "Attenuator", strdup("attn0"));

  // ModularSynth_addConnectionByName(synth, "vco0", "Sqr", "attn0", "Audio");
  // ModularSynth_addConnectionByName(synth, "attn0", "Audio", "__OUTPUT__", "Left");
  // ModularSynth_addConnectionByName(synth, "attn0", "Audio", "__OUTPUT__", "Right");

  // ModularSynth_removeConnectionByName(synth, "__OUTPUT__", "Right");

  // printf("%s\n", ModularSynth_PrintFullModuleInfo(synth, vco0));
  // printf("%s\n", ModularSynth_PrintFullModuleInfo(synth, attn0));

  R4 * signal = ModularSynth_getLeftChannel();




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

  while (WindowShouldClose() == false)
  {
    if (IsAudioStreamProcessed(synthStream))
    {
      ModularSynth_update();
      UpdateAudioStream(synthStream, signal, STREAM_BUFFER_SIZE);
    }


    // BeginDrawing();
    // ClearBackground(BLACK);

    // DrawText(TextFormat("Freq: %f", 100), 100, 100, 20, RED);

    // for (U4 t = 0; t < MIN(SCREEN_WIDTH, STREAM_BUFFER_SIZE); t++)
    // {
    //   const int upper = SCREEN_HEIGHT / 4;
    //   const int lower = 3 * SCREEN_HEIGHT / 4;
    //   int y = MAP(-1, 1, lower, upper, signal[t]);

    //   DrawPixel(t, y, GREEN);
    //   DrawPixel(t, MAP(-1, 1, lower, upper, 0), RED);
    // }

    // EndDrawing();
  }

  UnloadAudioStream(synthStream);
  CloseAudioDevice();
  CloseWindow();

  ModularSynth_free();
}
