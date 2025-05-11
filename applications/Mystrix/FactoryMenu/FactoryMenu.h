#pragma once

#include "MatrixOS.h"
#include "ui/UI.h"
#include "applications/Application.h"

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
  static Application_Info info;
  void Setup() override;

  void LEDTester();
  void KeyPadTester();
  void TouchBarTester();

  void KeyPadSettings();

  void EFuseBurner();
};

inline Application_Info FactoryMenu::info = {
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

REGISTER_APPLICATION(FactoryMenu);
