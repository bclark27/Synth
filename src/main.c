
#include "raylib/raylib.h"
#include "comm/Common.h"
#include "ModularSynth.h"
#include "AudioSettings.h"
#include <math.h>

#define SCREEN_WIDTH        2048
#define SCREEN_HEIGHT       786
#define FPS                 60

static void timeTest(void)
{
  ModularSynth * synth = ModularSynth_init();

  ModularID clk0 = ModularSynth_addModule(synth, ModuleType_Clock);
  ModularID clkMult = ModularSynth_addModule(synth, ModuleType_ClockMult);
  ModularID seq = ModularSynth_addModule(synth, ModuleType_Sequencer);
  ModularID adsr = ModularSynth_addModule(synth, ModuleType_ADSR);
  ModularID attn = ModularSynth_addModule(synth, ModuleType_Attenuator);
  ModularID vco1 = ModularSynth_addModule(synth, ModuleType_VCO);

  ModularSynth_setControl(synth, vco1, VCO_CONTROL_FREQ, -1);

  // connect clock to mult
  ModularSynth_addConnection(synth, clk0, CLOCK_OUT_PORT_CLOCK, clkMult, CLKMULT_IN_PORT_CLKIN);

  // connect mult to seq
  ModularSynth_addConnection(synth, clkMult, CLKMULT_OUT_PORT_EGHT, seq, SEQ_IN_PORT_CLKIN);

  // connect the gate out of the seq into the adsr gate in
  ModularSynth_addConnection(synth, seq, SEQ_OUT_PORT_GATE, adsr, ADSR_IN_PORT_GATE);

  // connect adsr envelope out to the attenuator volume modulator
  ModularSynth_addConnection(synth, adsr, ADSR_OUT_PORT_ENV, attn, ATTN_IN_PORT_VOL);

  // connect the pitch out of the seq to the input freq of the vco
  ModularSynth_addConnection(synth, seq, SEQ_OUT_PORT_PITCH, vco1, VCO_IN_PORT_FREQ);

  // connect the vco output to the attenuator audio in
  ModularSynth_addConnection(synth, vco1, VCO_OUT_PORT_SAW, attn, ATTN_IN_PORT_AUD);

  // connect attn out to the master out
  ModularSynth_addConnection(synth, attn, ATTN_OUT_PORT_AUD, OUT_MODULE_ID, OUTPUT_IN_PORT_LEFT);

  struct timeval stop, start;
  gettimeofday(&start, NULL);

  for (int i = 0; i < 150; i++)
  {
    ModularSynth_update(synth);
  }

  gettimeofday(&stop, NULL);
  double t = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
  printf("took %lf s\n", t / 1000000);

  ModularSynth_free(synth);
}




int main(void)
{
  timeTest();
  exit(0);



  ModularSynth * synth = ModularSynth_init();

  ModularID clk0 = ModularSynth_addModule(synth, ModuleType_Clock);
  ModularID clkMult = ModularSynth_addModule(synth, ModuleType_ClockMult);
  ModularID seq = ModularSynth_addModule(synth, ModuleType_Sequencer);
  ModularID adsr = ModularSynth_addModule(synth, ModuleType_ADSR);
  ModularID attn = ModularSynth_addModule(synth, ModuleType_Attenuator);
  ModularID vco1 = ModularSynth_addModule(synth, ModuleType_VCO);

  ModularSynth_setControl(synth, vco1, VCO_CONTROL_FREQ, -1);

  // connect clock to mult
  ModularSynth_addConnection(synth, clk0, CLOCK_OUT_PORT_CLOCK, clkMult, CLKMULT_IN_PORT_CLKIN);

  // connect mult to seq
  ModularSynth_addConnection(synth, clkMult, CLKMULT_OUT_PORT_EGHT, seq, SEQ_IN_PORT_CLKIN);

  // connect the gate out of the seq into the adsr gate in
  ModularSynth_addConnection(synth, seq, SEQ_OUT_PORT_GATE, adsr, ADSR_IN_PORT_GATE);

  // connect adsr envelope out to the attenuator volume modulator
  ModularSynth_addConnection(synth, adsr, ADSR_OUT_PORT_ENV, attn, ATTN_IN_PORT_VOL);

  // connect the pitch out of the seq to the input freq of the vco
  ModularSynth_addConnection(synth, seq, SEQ_OUT_PORT_PITCH, vco1, VCO_IN_PORT_FREQ);

  // connect the vco output to the attenuator audio in
  ModularSynth_addConnection(synth, vco1, VCO_OUT_PORT_SAW, attn, ATTN_IN_PORT_AUD);

  // connect attn out to the master out
  ModularSynth_addConnection(synth, attn, ATTN_OUT_PORT_AUD, OUT_MODULE_ID, OUTPUT_IN_PORT_LEFT);

  // ModularSynth_addConnection(synth, adsr, ADSR_OUT_PORT_ENV, OUT_MODULE_ID, OUTPUT_IN_PORT_LEFT);


  R4 * signal = ModularSynth_getLeftChannel(synth);




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


  while (WindowShouldClose() == false)
  {
    if (IsAudioStreamProcessed(synthStream))
    {
      ModularSynth_update(synth);
      UpdateAudioStream(synthStream, signal, STREAM_BUFFER_SIZE);
    }


    BeginDrawing();
    ClearBackground(BLACK);

    DrawText(TextFormat("Freq: %f", 100), 100, 100, 20, RED);

    for (U4 t = 0; t < MIN(SCREEN_WIDTH, STREAM_BUFFER_SIZE); t++)
    {
      const int upper = SCREEN_HEIGHT / 4;
      const int lower = 3 * SCREEN_HEIGHT / 4;
      int y = MAP(-1, 1, lower, upper, signal[t]);

      DrawPixel(t, y, GREEN);
      DrawPixel(t, MAP(-1, 1, lower, upper, 0), RED);
    }

    EndDrawing();
  }

  UnloadAudioStream(synthStream);
  CloseAudioDevice();
  CloseWindow();

  ModularSynth_free(synth);
}
