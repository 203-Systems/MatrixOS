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

class ForceGridVisualizer : public Application {
  public:
  static Application_Info info;
  void Setup() override;  
  void Loop() override;
  void Render();

  Timer renderTimer;
  Point activeKey = Point::Invalid();
};

inline Application_Info ForceGridVisualizer::info = {
    .name = "Force Grid Visualizer",
    .author = "203 Electronics",
    .color =  Color(0x00FF00),
    .version = 1,
    // #if MLOG_LEVEL == LOG_LEVEL_VERBOSE  // When in debug mode, show factory app 
    // .visibility = true,
    // #else
    .visibility = false,
    // #endif
};

REGISTER_APPLICATION(ForceGridVisualizer);