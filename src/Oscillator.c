#include "Oscillator.h"

#include "comm/Common.h"
#include "AudioSettings.h"
#include <immintrin.h>  // AVX

//////////////
// DEFINES  //
//////////////

#undef  ANALOG
#define HARMONICS 32

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

R4 triWave(R4 x) __attribute__((hot));

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Oscillator * Oscillator_init(R4 freq, Waveform waveform)
{
  Oscillator * osc = calloc(1, sizeof(Oscillator));

  Oscillator_initInPlace(osc, freq, waveform);

  return osc;
}

void Oscillator_initInPlace(Oscillator * osc, R4 freq, Waveform waveform)
{
  osc->waveform = waveform;
  osc->phase = 0;
  osc->stride = freq / SAMPLE_RATE;
  osc->freq = freq;
  osc->pulsWidth = 0.5;
}

void Oscillator_free(Oscillator * osc)
{
  free(osc);
}

void Oscillator_setFreq(Oscillator * osc, R4 freq)
{
  osc->stride = freq / SAMPLE_RATE;
  osc->freq = freq;
}

void Oscillator_setPW(Oscillator * osc, R4 pw)
{
  osc->pulsWidth = pw;
}

void Oscillator_sampleWithStrideAndPWTable(Oscillator * osc, R4 * samples, U4 samplesSize, R4 * strideTable, R4 * pwTable)
{
  switch (osc->waveform)
  {
    case Waveform_saw:
    {
      for (U4 i = 0; i < samplesSize; i++)
      {
        samples[i] = ((-(PI2 * osc->phase) / PI + 1) + 1) / 2;//sawWave(PI2 * osc->phase, pwTable[i]);
        
        osc->phase += strideTable[i];
        if (osc->phase >= 1)
        {
          osc->phase -= 1;
        }
      }
      break;
    }

    case Waveform_sqr:
    {
      for (U4 i = 0; i < samplesSize; i++)
      {
        samples[i] = osc->phase < pwTable[i];//squareWave(PI2 * osc->phase, pwTable[i]);
        
        osc->phase += strideTable[i];
        if (osc->phase >= 1)
        {
          osc->phase -= 1;
        }
      }
      break;
    }

    case Waveform_tri:
    {
      for (U4 i = 0; i < samplesSize; i++)
      {
        samples[i] = triWave(PI2 * osc->phase);
        
        osc->phase += strideTable[i];
        if (osc->phase >= 1)
        {
          osc->phase -= 1;
        }
      }
      break;
    }

    case Waveform_sin:
    {
      for (U4 i = 0; i < samplesSize; i++)
      {
        samples[i] = sinf(PI2 * osc->phase);
        
        osc->phase += strideTable[i];
        if (osc->phase >= 1)
        {
          osc->phase -= 1;
        }
      }
      break;
    }
  }
  
  osc->pulsWidth = pwTable[samplesSize - 1];
  osc->stride = strideTable[samplesSize - 1];
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

R4 triWave(R4 x)
{
  R4 scaledX = TWO_ON_PI * x;
  if (x < PI_ON_TWO)
  {
    return (-fabsf(scaledX - 1) + 2) / 2;
  }
  else
  {
    return fabsf(scaledX - 3) / 2;
  }

}
