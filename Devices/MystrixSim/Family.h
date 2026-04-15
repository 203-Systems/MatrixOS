#pragma once

#include "Device.h"
#include "Framework.h"

#define GRID_TYPE_8x8
#define FAMILY_MYSTRIXSIM

struct DeviceInfo {
  char model[4];
  char revision[4];
  uint8_t productionYear;
  uint8_t productionMonth;
};
