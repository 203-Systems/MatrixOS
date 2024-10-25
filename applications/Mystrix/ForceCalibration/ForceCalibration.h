#pragma once

#include "MatrixOS.h"
#include "ui/UI.h"
#include "applications/Application.h"

namespace Device::KeyPad::FSR
{
  extern Fract16 (*low_thresholds)[x_size][y_size];
  extern Fract16 (*high_thresholds)[x_size][y_size];

  void SaveLowCalibration();
  void SaveHighCalibration();
  void ClearLowCalibration();
  void ClearHighCalibration();
  int16_t GetLowOffset();
  int16_t GetHighOffset();
  void SetLowOffset(int16_t offset);
  void SetHighOffset(int16_t offset);
  uint16_t GetRawReading(uint8_t x, uint8_t y);
  uint32_t GetScanCount();
}

class ForceCalibration : public Application {
  public:
  enum CalibrationType: uint8_t {
    Low,
    High
  };
  static Application_Info info;
  void Setup() override;  

  void LowCalibration();
  void HighCalibration();

  void ClearLowCalibration();
  void ClearHighCalibration();

  void SetOffset(CalibrationType type);

  void ForceGridVisualizer();

 private:
  Timer renderTimer;

  CreateSavedVar("ForceCalibration", lowCalibrationSaved, bool, false);
  CreateSavedVar("ForceCalibration", highCalibrationSaved, bool, false);
};

inline Application_Info ForceCalibration::info = {
    .name = "Force Calibration",
    .author = "203 Systems",
    .color =  Color(0xFFFFFF),
    .version = 1,
    // #if MLOG_LEVEL == LOG_LEVEL_VERBOSE  // When in debug mode, show factory app 
    // .visibility = true,
    // #else
    .visibility = false,
    // #endif
};

REGISTER_APPLICATION(ForceCalibration);