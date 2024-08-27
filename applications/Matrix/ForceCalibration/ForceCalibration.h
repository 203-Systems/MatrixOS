#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include "applications/Application.h"

namespace Device::KeyPad::FSR
{
  extern Fract16 (*low_thresholds)[x_size][y_size];
  extern Fract16 (*high_thresholds)[x_size][y_size];

  void SaveLowCalibration();
  void SaveHighCalibration();
  uint16_t GetRawReading(uint8_t x, uint8_t y);
  uint32_t GetScanCount();
}

class ForceCalibration : public Application {
  public:
  static Application_Info info;
  void Setup() override;  

  void LowCalibration();
  void HighCalibration();

 private:
  Timer renderTimer;

  CreateSavedVar("Low Calibration Saved", lowCalibrationSaved, bool, false);
  CreateSavedVar("High Calibration Saved", highCalibrationSaved, bool, false);
};

inline Application_Info ForceCalibration::info = {
    .name = "Force Calibration",
    .author = "203 Electronics",
    .color =  Color(0xFFFFFF),
    .version = 1,
    // #if MLOG_LEVEL == LOG_LEVEL_VERBOSE  // When in debug mode, show factory app 
    // .visibility = true,
    // #else
    .visibility = false,
    // #endif
};

REGISTER_APPLICATION(ForceCalibration);