#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include "applications/Application.h"
#include "ulp_fsr_keypad.h"

// #if MLOG_LEVEL == LOG_LEVEL_VERBOSE  // When in debug mode, show factory app 
#define APPLICATION_VISIBLITY true
// #else
// #define APPLICATION_VISIBLITY false
// #endif

class ForceCalibration : public Application {
  constexpr static uint16_t kCalibrationTime = 1024;
  enum class State {
    NotCalibrated,
    WaitingToStablize,
    Capturing,
    Calibrated,
  };
  public:
  static Application_Info info;
  void Setup() override;  
  void Loop() override;

  void KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo);
  void Calibrate(Point key);
  void Render();
  void Save();

 private:
  Timer renderTimer;
  Timer calibrateTimer;
  uint16_t (*result)[8][3] = (uint16_t(*)[8][3]) & ulp_result;

  State state[8][8];
  uint16_t calibrationData[8][8];

  Point currentCalibrateKey = Point::Invalid();
  uint32_t calibrationStateTimestamp;
  uint16_t calibrationProgress = 0;
  uint16_t calibrationBuffer[kCalibrationTime];
  uint16_t MiddleOfThree(uint16_t a, uint16_t b, uint16_t c);
  uint16_t KeypadScanCountCache = 0;
};

inline Application_Info ForceCalibration::info = {
    .name = "Force Calibration",
    .author = "203 Electronics",
    .color =  Color(0x00FF00),
    .version = 1,
    // #if MLOG_LEVEL == LOG_LEVEL_VERBOSE  // When in debug mode, show factory app 
    .visibility = true,
    // #else
    // .visibility = false,
    // #endif
};

REGISTER_APPLICATION(ForceCalibration);