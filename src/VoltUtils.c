#include "VoltUtils.h"

static R4 fasterPow(R4 a, R4 b)
{
  union {
    double d;
    int x[2];
  } u = { a };
  u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
  u.x[0] = 0;

  return u.d;
}

inline R4 VoltUtils_voltToFreq(R4 volts)
{
  return VOLTSTD_C3_FREQ * pow(2, volts - VOLTSTD_C3_VOLTAGE);
}

inline R4 VoltUtils_voltDbToAmpl(R4 volts)
{
  return fasterPow(10, (volts - VOLTSTD_CV_MID) * VOLTSTD_VOLT_PER_DB_AMP);
}

inline R4 VoltUtils_voltDbToAtten(R4 volts)
{
  return volts <= VOLTSTD_CV_MIN ? 0 : fasterPow(10, (volts - VOLTSTD_CV_MAX) * VOLTSTD_VOLT_PER_DB_ATN);
}
