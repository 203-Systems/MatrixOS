#pragma once

#include "MatrixOS.h"
#include "ui/UI.h"
#include "applications/Application.h"

class Shell : public Application {
  public:
  static Application_Info info;

  uint8_t current_page = 0;

  void Setup() override;
  void Loop() override;

  void AddCommonBarInUI(UI* ui);
  void ApplicationLauncher();
  void HiddenApplicationLauncher();
  void LaunchAnimation(Point origin, Color color);
};

inline Application_Info Shell::info = {
    .name = "Shell",
    .author = "203 Systems",
    .color = Color(0x00FFAA),
    .version = 1,
    .visibility = false,
};

REGISTER_APPLICATION(Shell);
