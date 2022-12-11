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

#define VOLTSTD_CV_MAX          5.f
#define VOLTSTD_CV_MIN          -5.f
#define VOLTSTD_CV_RANGE        (VOLTSTD_CV_MAX - VOLTSTD_CV_MIN)
#define VOLTSTD_CV_MID          (VOLTSTD_CV_RANGE / 2 + VOLTSTD_CV_MAX)

#define VOLTSTD_C3_VOLTAGE      0.f
#define VOLTSTD_C3_FREQ         130.8128

#define VOLTSTD_GATE_ON         5.f
#define VOLTSTD_GATE_OFF        0.f

#define VOLTSTD_AUD_MAX         5.f
#define VOLTSTD_AUD_MIN         -5.f
#define VOLTSTD_AUD_RANGE       (VOLTSTD_AUD_MAX - VOLTSTD_AUD_MIN)

#define VOLTSTD_VOLT_PER_DB     0.2f

////////////////////////
//  PUBLIC FUNCTIONS  //
////////////////////////

R4 VoltUtils_voltToFreq(R4 volts) __attribute__((hot));
R4 VoltUtils_voltDbToAmpl(R4 volts) __attribute__((hot));
R4 VoltUtils_voltDbToAtten(R4 volts) __attribute__((hot));
#endif
