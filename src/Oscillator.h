#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include "comm/Common.h"

/*
Oscillator returns wave signal from 0 to 1. caller must have
its own code to make custom range
*/

///////////
// TYPES //
///////////

typedef enum Waveform
{
  Waveform_sin,
  Waveform_sqr,
  Waveform_saw,
  Waveform_tri,
} Waveform;

typedef struct Oscillator
{
  Waveform waveform;
  R4 phase;
  R4 pulsWidth;
  R4 stride;
  R4 freq;
} Oscillator;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Oscillator * Oscillator_init(R4 freq, Waveform waveform);
void Oscillator_initInPlace(Oscillator * osc, R4 freq, Waveform waveform);
void Oscillator_free(Oscillator * osc);

void Oscillator_setFreq(Oscillator * osc, R4 freq);
void Oscillator_setPW(Oscillator * osc, R4 pw);
void Oscillator_sampleWithStrideAndPWTable(Oscillator * osc, R4 * samples, U4 samplesSize, R4 * strideTable, R4 * pwTable);

#endif
