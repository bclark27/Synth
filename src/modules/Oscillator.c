#include "Oscillator.h"

#include <immintrin.h>  // AVX

//////////////
// DEFINES  //
//////////////

#undef  ANALOG
#define HARMONICS 32
#define SIN_TABLE_SIZE  65536
#define SIN_TABLE_MASK (SIN_TABLE_SIZE - 1)  // works if TABLE_SIZE is power of 2
#define EPSILON (0.0005f)
#define POW1_TABLE_SIZE 2048

////////////
// TABLES //
////////////

static float sinTable[SIN_TABLE_SIZE];
static float pow1Table[POW1_TABLE_SIZE];
static bool tableInitDone = false;

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

static inline R4 triWave(R4 x) __attribute__((hot));
static inline R4 FastSinf(R4 x) __attribute__((hot));
static inline void fillDetuneTable(float *detuneMul, int unison, float detuneAmt);
static void initTables();
static inline float fastPow1Table(float v);

//////////////////////
// PUBLIC FUNCTIONS //
//////////////////////

Oscillator * Oscillator_init(Waveform waveform)
{
  Oscillator * osc = calloc(1, sizeof(Oscillator));

  Oscillator_initInPlace(osc, waveform);

  return osc;
}

void Oscillator_initInPlace(Oscillator * osc, Waveform waveform)
{
  if (!tableInitDone)
  {
    initTables();
    tableInitDone = true;
  }
  osc->waveform = waveform;
  osc->prevUnison = 127;
  osc->prevDetune = -999;
  memset(osc->phase, 0, sizeof(osc->phase));
}

void Oscillator_free(Oscillator * osc)
{
  free(osc);
}

// IMPORTANT the samples range should be just [-VOLTSTD_AUD_MAX, VOLTSTD_AUD_MAX], not 0,1 or -1,1 or something
void Oscillator_sampleWithStrideAndPWTable(Oscillator * osc, R4 * samples, U4 samplesSize, R4 * strideTable, R4 * pwTable, U1 unison, R4 detune, bool* phaseCompleted)
{
  if (unison == 1)
  {
    switch (osc->waveform)
    {
      case Waveform_saw:
      {
        for (U4 i = 0; i < samplesSize; i++)
        {
          samples[i] = (1 - osc->phase[0]) * 2 * VOLTSTD_AUD_MAX - VOLTSTD_AUD_MAX;
          
          osc->phase[0] += strideTable[i];
          phaseCompleted[i] = osc->phase[0] >= 1;
          if (phaseCompleted[i])
          {
            osc->phase[0] -= 1;
          }
        }
        break;
      }

      case Waveform_sqr:
      {
        for (U4 i = 0; i < samplesSize; i++)
        {
          samples[i] = (osc->phase[0] < pwTable[i]) * 2 * VOLTSTD_AUD_MAX - VOLTSTD_AUD_MAX;
          
          osc->phase[0] += strideTable[i];
          phaseCompleted[i] = osc->phase[0] >= 1;
          if (phaseCompleted[i])
          {
            osc->phase[0] -= 1;
          }
        }
        break;
      }

      case Waveform_tri:
      {
        for (U4 i = 0; i < samplesSize; i++)
        {
          samples[i] = triWave(PI2 * osc->phase[0]) * 2 * VOLTSTD_AUD_MAX - VOLTSTD_AUD_MAX;
          
          osc->phase[0] += strideTable[i];
          phaseCompleted[i] = osc->phase[0] >= 1;
          if (phaseCompleted[i])
          {
            osc->phase[0] -= 1;
          }
        }
        break;
      }

      case Waveform_sin:
      {
        for (U4 i = 0; i < samplesSize; i++)
        {
          samples[i] = FastSinf(osc->phase[0]) * VOLTSTD_AUD_MAX;
          // Increment phase by stride
          osc->phase[0] += strideTable[i];

          // Wrap phase back to [0,1)
          phaseCompleted[i] = osc->phase[0] >= 1;
          if (phaseCompleted[i])
              osc->phase[0] -= 1.0f;
        }
        break;
      }
    }
  }
  else
  {
    detune = fmaxf(detune, 1);
    bool change = fabsf(osc->prevUnison - unison) > EPSILON ||
                  fabsf(osc->prevDetune - detune) > EPSILON;
    if (change)
    {
      osc->prevUnison = unison;
      osc->prevDetune = detune;
      fillDetuneTable(osc->detuneTable, unison, detune);
    }
    R4 thisSample;
    switch (osc->waveform)
    {
      case Waveform_saw:
      {
        for (U4 i = 0; i < samplesSize; i++)
        {
          thisSample = 0;
          for (int k = 0; k < unison; k++)
          {
            thisSample += (1 - osc->phase[k]);
            osc->phase[k] += strideTable[i] * osc->detuneTable[k];
            if (osc->phase[k] >= 1.0f)
                osc->phase[k] -= 1.0f;
          }

          samples[i] = (thisSample / unison) * 2 * VOLTSTD_AUD_MAX - VOLTSTD_AUD_MAX;
        }
        break;
      }

      case Waveform_sqr:
      {
        for (U4 i = 0; i < samplesSize; i++)
        {
          thisSample = 0;
          for (int k = 0; k < unison; k++)
          {
            thisSample += (osc->phase[k] < pwTable[i]);
            osc->phase[k] += strideTable[i] * osc->detuneTable[k];
            if (osc->phase[k] >= 1.0f)
                osc->phase[k] -= 1.0f;
          }

          samples[i] = (thisSample / unison) * 2 * VOLTSTD_AUD_MAX - VOLTSTD_AUD_MAX;
        }
        break;
      }

      case Waveform_tri:
      {
        for (U4 i = 0; i < samplesSize; i++)
        {
          thisSample = 0;
          for (int k = 0; k < unison; k++)
          {
            thisSample += triWave(PI2 * osc->phase[k]);
            osc->phase[k] += strideTable[i] * osc->detuneTable[k];
            if (osc->phase[k] >= 1.0f)
                osc->phase[k] -= 1.0f;
          }

          samples[i] = (thisSample / unison) * 2 * VOLTSTD_AUD_MAX - VOLTSTD_AUD_MAX;
        }
        break;
      }

      case Waveform_sin:
      {
        for (U4 i = 0; i < samplesSize; i++)
        {
          thisSample = 0;
          for (int k = 0; k < unison; k++)
          {
            thisSample += FastSinf(osc->phase[k]);
            osc->phase[k] += strideTable[i] * osc->detuneTable[k];
            if (osc->phase[k] >= 1.0f)
                osc->phase[k] -= 1.0f;
          }

          samples[i] = (thisSample / unison) * VOLTSTD_AUD_MAX;
        }
        break;
      }
    }
  }
}

/////////////////////////
//  PRIVATE FUNCTIONS  //
/////////////////////////

static inline R4 triWave(R4 x)
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

static inline R4 FastSinf(R4 x)
{
  float t = x * SIN_TABLE_SIZE;
  uint32_t idx = (uint32_t)t & SIN_TABLE_MASK;
  float frac = t - idx;
  float s0 = sinTable[idx];
  float s1 = sinTable[(idx + 1) & SIN_TABLE_MASK];
  return s0 + frac * (s1 - s0);
}

static inline void fillDetuneTable(float *detuneMul, int unison, float detuneAmt)
{
    if (unison <= 1) {
        detuneMul[0] = 1.0f;
        return;
    }

    // Work in log2 space so detune is symmetrical on a ratio scale
    float logMax =  log2f(detuneAmt);
    float logMin = -logMax;

    bool hasCenter = (unison % 2 != 0);
    int count = unison;
    float step = (logMax - logMin) / (float)(count - 1);

    for (int i = 0; i < count; i++) {
        float pos = logMin + i * step;   // evenly spaced on log scale
        detuneMul[i] = fastPow1Table(pos);//powf(2.0f, pos);  // convert back to frequency multiplier
    }

    // If odd number, center voice will be exactly 1.0
}


static void initTables()
{
  float inc = 2.f / POW1_TABLE_SIZE;
  float v = -1;
  for (int i = 0; i < POW1_TABLE_SIZE; i++)
  {
    pow1Table[i] = powf(2.0f, v);
    v += inc;
  }

  inc = PI2 / SIN_TABLE_SIZE;
  v = 0;
  for (int i = 0; i < SIN_TABLE_SIZE; i++)
  {
    sinTable[i] = sinf(v);
    v += inc;
  }
}

static inline float fastPow1Table(float v)
{
  v = CLAMPF(-1, 1, v);
  float idx = (v + 1) / 2 * (POW1_TABLE_SIZE - 1);
  int i = (int)idx;
  float frac = idx - i;
  float ret = pow1Table[i] * (1.0f - frac)
       + pow1Table[i + 1] * frac;
  return ret;
}
