#include "Device.h"

namespace Device::KeyPad::FSR
{
  static Fract16 low_thresholds_storage[X_SIZE][Y_SIZE];
  static Fract16 high_thresholds_storage[X_SIZE][Y_SIZE];

  Fract16 (*low_thresholds)[X_SIZE][Y_SIZE] = &low_thresholds_storage;
  Fract16 (*high_thresholds)[X_SIZE][Y_SIZE] = &high_thresholds_storage;

  CreateSavedVar("ForceCalibration", lowOffset, int16_t, 0);
  CreateSavedVar("ForceCalibration", highOffset, int16_t, 0);

  void Init() {
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      for (uint8_t y = 0; y < Y_SIZE; y++)
      {
        (*low_thresholds)[x][y] = keypad_config.low_threshold;
        (*high_thresholds)[x][y] = keypad_config.high_threshold;
      }
    }
  }

  void SaveLowCalibration() {}

  void SaveHighCalibration() {}

  void ClearLowCalibration()
  {
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      for (uint8_t y = 0; y < Y_SIZE; y++)
      {
        (*low_thresholds)[x][y] = keypad_config.low_threshold;
      }
    }
  }

  void ClearHighCalibration()
  {
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      for (uint8_t y = 0; y < Y_SIZE; y++)
      {
        (*high_thresholds)[x][y] = keypad_config.high_threshold;
      }
    }
  }

  int16_t GetLowOffset()
  {
    return lowOffset;
  }

  int16_t GetHighOffset()
  {
    return highOffset;
  }

  void SetLowOffset(int16_t offset)
  {
    lowOffset.Set(offset);
  }

  void SetHighOffset(int16_t offset)
  {
    highOffset.Set(offset);
  }

  uint32_t GetScanCount()
  {
    return 0;
  }

  uint16_t GetRawReading(uint8_t x, uint8_t y)
  {
    (void)x;
    (void)y;
    return 0;
  }

  void Start() {}

  IRAM_ATTR bool Scan() {
    return false;
  }
}
