#ifndef OSCILLATOR_H_
#define OSCILLATOR_H_

#include "Module.h"

/*
Oscillator returns wave signal from 0 to 1. caller must have
its own code to make custom range
*/

#define MAX_UNISON  8

///////////
// TYPES //
///////////

typedef enum Waveform
{
  Waveform_sin,
  Waveform_sqr,
  Waveform_saw,
  Waveform_tri,

  Wafeform_Count,
} Waveform;

typedef struct OscillatorVoice
{

} OscillatorVoice;

typedef struct Oscillator
{
  Waveform waveform;
  R4 phase[MAX_UNISON];
  R4 detuneTable[MAX_UNISON];

  U1 prevUnison;
  R4 prevDetune;
} Oscillator;

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

Oscillator * Oscillator_init(Waveform waveform);
void Oscillator_initInPlace(Oscillator * osc, Waveform waveform);
void Oscillator_free(Oscillator * osc);

void Oscillator_sampleWithStrideAndPWTable(Oscillator * osc, R4 * samples, U4 samplesSize, R4 * strideTable, R4 * pwTable, U1 unison, R4 detune, bool* phaseCompleted);

#endif
