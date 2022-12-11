
#include "raylib/raylib.h"
#include "comm/Common.h"
#include "ModuleFactory.h"
#include "AudioSettings.h"
#include <math.h>

#define SCREEN_WIDTH        1024
#define SCREEN_HEIGHT       786
#define FPS                 60


int main(void)
{
  Module * vco0 = ModuleFactory_createModule(ModuleType_VCO);
  Module * vco1 = ModuleFactory_createModule(ModuleType_VCO);
  Module * mix0 = ModuleFactory_createModule(ModuleType_Mixer);

  R4 * vco0Sig = vco0->getOutputAddr(vco0, VCO_OUT_PORT_SAW);
  R4 * vco1Sig = vco1->getOutputAddr(vco1, VCO_OUT_PORT_SQR);
  R4 * mixSig = mix0->getOutputAddr(mix0, MIXER_OUT_PORT_SUM);

  vco0->setControlVal(vco0, VCO_CONTROL_FREQ, -1);
  vco0->setControlVal(vco0, VCO_CONTROL_FREQ, -2);
  mix0->setControlVal(mix0, MIXER_CONTROL_VOL, -1);

  mix0->linkToInput(mix0, MIXER_IN_PORT_AUDIO0, vco0Sig);
  mix0->linkToInput(mix0, MIXER_IN_PORT_AUDIO1, vco1Sig);



  // R4 tmp[STREAM_BUFFER_SIZE];
  // vco0->linkToInput(vco0, 0, tmp);
  //
  // struct timeval stop, start;
  //
  // gettimeofday(&start, NULL);
  // for (U4 i = 0; i < 600; i++)
  // {
  //   vco0->updateState(vco0);
  //   mix0->updateState(mix0);
  //
  //   vco0->pushCurrToPrev(vco0);
  //   mix0->pushCurrToPrev(mix0);
  // }
  // gettimeofday(&stop, NULL);
  // double t = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
  // printf("took %lf s\n", t / 1000000);
  // exit(0);





  R4 signal[STREAM_BUFFER_SIZE];
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
      vco0->updateState(vco0);
      vco1->updateState(vco1);
      mix0->updateState(mix0);

      vco0->pushCurrToPrev(vco0);
      vco1->pushCurrToPrev(vco1);
      mix0->pushCurrToPrev(mix0);

      for (U4 t = 0; t < STREAM_BUFFER_SIZE; t++)
      {
        signal[t] = mixSig[t] / 5;//(sig0[t] + sig1[t] + sig2[t]) / 3;
      }

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
    }

    EndDrawing();
  }

  UnloadAudioStream(synthStream);
  CloseAudioDevice();
  CloseWindow();

  ModuleFactory_destroyModule(vco0);
  ModuleFactory_destroyModule(mix0);
}
