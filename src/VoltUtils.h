#ifndef VOLT_UTILS_H_
#define VOLT_UTILS_H_

#include "comm/Common.h"

//////////////////////////
// DEFAULT VOLT BOUNDS  //
//////////////////////////

/*
different synth racks come with different standards for voltages on in and output
set these ranges to emulate some type of rack
*/

#define VOLTSTD_MOD_CV_MAX        10.f
#define VOLTSTD_MOD_CV_MIN        (-VOLTSTD_MOD_CV_MAX)
#define VOLTSTD_MOD_CV_RANGE      (VOLTSTD_MOD_CV_MAX - VOLTSTD_MOD_CV_MIN)
#define VOLTSTD_MOD_CV_MID        ((VOLTSTD_MOD_CV_MAX + VOLTSTD_MOD_CV_MIN) / 2)

#define VOLTSTD_PITCH_CV_MAX      5.f
#define VOLTSTD_PITCH_CV_MIN      0.f

#define VOLTSTD_C3_VOLTAGE        0.f
#define VOLTSTD_C3_FREQ           130.8128

#define VOLTSTD_GATE_HIGH         5.f
#define VOLTSTD_GATE_LOW          0.f
#define VOLTSTD_GATE_ERROR        0.01f
#define VOLTSTD_GATE_HIGH_THRESH  (VOLTSTD_GATE_HIGH - VOLTSTD_GATE_ERROR)
#define VOLTSTD_GATE_LOW_THRESH   (VOLTSTD_GATE_LOW + VOLTSTD_GATE_ERROR)

#define VOLTSTD_AUD_MAX           5.f
#define VOLTSTD_AUD_MIN           (-VOLTSTD_AUD_MAX)
#define VOLTSTD_AUD_RANGE         (VOLTSTD_AUD_MAX - VOLTSTD_AUD_MIN)

#define VOLTSTD_VOLT_PER_DB_AMP   0.2f
#define VOLTSTD_VOLT_PER_DB_ATN   0.1f

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

/*
takes volts in any range, with 0v as C3 and 1v per oct, ret a freq
*/
R4 VoltUtils_voltToFreq(R4 volts) __attribute__((hot));

/*
takes signal in CV mod range and returns a amplitude moltiplyer
*/
R4 VoltUtils_voltDbToAmpl(R4 volts) __attribute__((hot));

/*
takes signal in CV mod range and reurns amplitude multiplyer less than one
*/
R4 VoltUtils_voltDbToAttenuverterMult(R4 volts) __attribute__((hot));
#endif
