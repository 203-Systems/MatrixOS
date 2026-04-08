#include "Device.h"

namespace Device::KeyPad::FSR
{
static Fract16 lowThresholdsStorage[X_SIZE][Y_SIZE];
static Fract16 highThresholdsStorage[X_SIZE][Y_SIZE];

Fract16 (*lowThresholds)[X_SIZE][Y_SIZE] = &lowThresholdsStorage;
Fract16 (*highThresholds)[X_SIZE][Y_SIZE] = &highThresholdsStorage;

CreateSavedVar("ForceCalibration", lowOffset, int16_t, 0);
CreateSavedVar("ForceCalibration", highOffset, int16_t, 0);

void Init() {
  for (uint8_t x = 0; x < X_SIZE; x++)
  {
    for (uint8_t y = 0; y < Y_SIZE; y++)
    {
      (*lowThresholds)[x][y] = keypadConfig.lowThreshold;
      (*highThresholds)[x][y] = keypadConfig.highThreshold;
    }
  }
}

void SaveLowCalibration() {}

void SaveHighCalibration() {}

void ClearLowCalibration() {
  for (uint8_t x = 0; x < X_SIZE; x++)
  {
    for (uint8_t y = 0; y < Y_SIZE; y++)
    {
      (*lowThresholds)[x][y] = keypadConfig.lowThreshold;
    }
  }
}

void ClearHighCalibration() {
  for (uint8_t x = 0; x < X_SIZE; x++)
  {
    for (uint8_t y = 0; y < Y_SIZE; y++)
    {
      (*highThresholds)[x][y] = keypadConfig.highThreshold;
    }
  }
}

int16_t GetLowOffset() {
  return lowOffset;
}

int16_t GetHighOffset() {
  return highOffset;
}

void SetLowOffset(int16_t offset) {
  lowOffset.Set(offset);
}

void SetHighOffset(int16_t offset) {
  highOffset.Set(offset);
}

uint32_t GetScanCount() {
  return 0;
}

uint16_t GetRawReading(uint8_t x, uint8_t y) {
  (void)x;
  (void)y;
  return 0;
}

void Start() {}

IRAM_ATTR bool Scan() {
  return false;
}
} // namespace Device::KeyPad::FSR
