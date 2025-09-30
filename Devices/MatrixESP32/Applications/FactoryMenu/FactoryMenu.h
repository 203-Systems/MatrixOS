#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include "Application.h"

#include "esp_efuse.h"
#include "esp_efuse_table.h"

#if defined(FACTORY_CONFIG)
#define EFUSE_BURNER
#endif

#if MLOG_LEVEL == LOG_LEVEL_VERBOSE  // When in debug mode, show factory app 
#define APPLICATION_VISIBLITY true
#else
#define APPLICATION_VISIBLITY false
#endif

class FactoryMenu : public Application {
  public:
  inline static Application_Info info = {
      .name = "Factory Menu",
      .author = "203 Systems",
      .color =  Color(0xFFFFFF),
      .version = 1,
      #if MLOG_LEVEL == LOG_LEVEL_VERBOSE  // When in debug mode, show factory app 
      .visibility = true,
      #else
      .visibility = false,
      #endif
  };
  void Setup(const vector<string>& args) override;

  void LEDTester();
  void KeyPadTester();
  void TouchBarTester();

  void KeyPadSettings();

  void EFuseBurner();
};


