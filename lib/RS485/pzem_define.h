#ifndef _PZEM004T_V2_DEFINE_H_
#define _PZEM004T_V2_DEFINE_H_

#include "Arduino.h"

#define SCALE_V               (0.1)
#define SCALE_A               (0.001)
#define SCALE_P               (0.1)
#define SCALE_E               (1)
#define SCALE_H               (0.1)
#define SCALE_PF              (0.01)

#define PZEM_GET_VALUE(unit, scale)  ((float)(data[unit]) * scale)
#define PZEM_GET_VALUE2(unitHigh, unitLow, scale)  ((float)(((uint32_t)data[unitHigh] << 16) | data[unitLow]) * scale)

enum {
  REG_VOLTAGE = 0,
  REG_CURRENT_H,
  REG_CURRENT_L,
  REG_POWER_H,
  REG_POWER_L,
  REG_ENERGY_H,
  REG_ENERGY_L,
  REG_FREQ,
  REG_PF,
  REG_RESERVED,
  REG_TOTAL
};

typedef struct dataPzem{
  float volt;
  float ampe;
  float power;
  float energy;
  float freq;
  float powerFactor;
};

#endif