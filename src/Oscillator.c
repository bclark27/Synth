#include "Oscillator.h"

#include "comm/Common.h"
#include "AudioSettings.h"

//////////////
// DEFINES  //
//////////////

#undef  ANALOG
#define HARMONICS 64

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

R4 fastSin(R4 x, R4 shape) __attribute__((hot));
R4 analogSin(R4 x, R4 shape) __attribute__((hot));
R4 squareWave(R4 x, R4 shape) __attribute__((hot));
R4 sawWave(R4 x, R4 shape) __attribute__((hot));
R4 triWave(R4 x, R4 shape) __attribute__((hot));

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
  Oscillator_setWaveform(osc, waveform);

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

void Oscillator_setWaveform(Oscillator * osc, Waveform waveform)
{
  switch (waveform)
  {
    case Waveform_sin:
    osc->waveformFunc = analogSin;
    break;

    case Waveform_sqr:
    osc->waveformFunc = squareWave;
    break;

    case Waveform_saw:
    osc->waveformFunc = sawWave;
    break;

    case Waveform_tri:
    osc->waveformFunc = triWave;
    break;

  }
}

void Oscillator_sampleAndUpdate(Oscillator * osc, R4 * samples, U4 samplesSize)
{
  for (U4 i = 0; i < samplesSize; i++)
  {
    samples[i] = osc->waveformFunc(PI2 * osc->phase, osc->pulsWidth);

    osc->phase += osc->stride;
    if (osc->phase >= 1)
    {
      osc->phase -= 1;
    }
  }
}

void Oscillator_sampleAndUpdateFreqSweep(Oscillator * osc, R4 * samples, U4 samplesSize, R4 startFreq, R4 endFreq)
{

  R4 Ss = startFreq / SAMPLE_RATE;
  R4 Se = endFreq / SAMPLE_RATE;
  R4 SDelta = (Se - Ss) / samplesSize;
  R4 currStride = osc->stride;

  for (U4 i = 0; i < samplesSize; i++)
  {
    samples[i] = osc->waveformFunc(PI2 * osc->phase, osc->pulsWidth);

    osc->phase += currStride;
    if (osc->phase >= 1)
    {
      osc->phase -= 1;
    }

    currStride += SDelta;
  }

  osc->stride = Se;
}

void Oscillator_sampleAndUpdateStrideTable(Oscillator * osc, R4 * samples, U4 samplesSize, R4 * strideTable)
{
  for (U4 i = 0; i < samplesSize; i++)
  {
    samples[i] = osc->waveformFunc(PI2 * osc->phase, osc->pulsWidth);

    osc->phase += strideTable[i];
    if (osc->phase >= 1)
    {
      osc->phase -= 1;
    }
  }

  osc->stride = strideTable[samplesSize - 1];
}

void Oscillator_sampleWithStrideAndPWTable(Oscillator * osc, R4 * samples, U4 samplesSize, R4 * strideTable, R4 * pwTable)
{
  for (U4 i = 0; i < samplesSize; i++)
  {
    samples[i] = osc->waveformFunc(PI2 * osc->phase, pwTable[i]);

    osc->phase += strideTable[i];
    if (osc->phase >= 1)
    {
      osc->phase -= 1;
    }
  }

  osc->pulsWidth = pwTable[samplesSize - 1];
  osc->stride = strideTable[samplesSize - 1];
}

R4 Oscillator_singleSampleAndUpdate(Oscillator * osc)
{
  R4 sample = osc->waveformFunc(PI2 * osc->phase, osc->pulsWidth);

  osc->phase += osc->stride;
  if (osc->phase >= 1)
  {
    osc->phase -= 1;
  }

  return sample;
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

R4 fastSin(R4 x, R4 shape)
{
  return sinf(x);

  // R4 sinVal;
  //
  // while (x > PI2)
  // {
  //   x -= PI2;
  // }
  //
  // if (x < PI)
  // {
  //   sinVal = (16 * x * (PI - x)) / (5 * PI * PI - 4 * x * (PI - x));
  // }
  // else
  // {
  //   x = x - PI;
  //   sinVal = -(16 * x * (PI - x)) / (5 * PI * PI - 4 * x * (PI - x));
  // }
  //
  // return (sinVal + 1) / 2;
}

R4 analogSin(R4 x, R4 shape)
{
  R4 sinVal = fastSin(x + RAND_DOUBLE / 100, shape);
  return (sinVal + 1) / 2;
}

R4 squareWave(R4 x, R4 shape)
{
#ifdef ANALOG
  R4 sum = 0;

  for (U4 i = 0; i < HARMONICS; i++)
  {
    U4 term = i * 2 + 1;
    sum += fastSin(term * x) / term;
  }
  return (sum + 1) / 2;
#else
  // return (int)(x / PI);
  R4 percent = x / PI2;
  return percent < shape;

#endif
}

R4 sawWave(R4 x, R4 shape)
{
#ifdef ANALOG
  R4 sum = 0;
  for (U4 i = 1; i <= HARMONICS; i++)
  {
    sum += fastSin(i * x, shape) / i;
  }
  return (TWO_ON_PI * sum + 1.01) / 2;
#else
  R4 val = -x / PI + 1;
  return (val + 1) / 2;
#endif
}

R4 triWave(R4 x, R4 shape)
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
