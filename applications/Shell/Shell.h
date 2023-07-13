#pragma once

#include "MatrixOS.h"
#include "ui/UI.h"
#include "applications/Application.h"

class Shell : public Application {
  public:
  static Application_Info info;

  uint8_t current_page = 0;

  void Loop() override;

  void AddCommonBarInUI(UI* ui);
  void ApplicationLauncher();
  void HiddenApplicationLauncher();
};

inline Application_Info Shell::info = {
    .name = "Shell",
    .author = "203 Electronics",
    .color = Color(0x00FFAA),
    .version = 1,
    .visibility = false,
};

REGISTER_APPLICATION(Shell);
