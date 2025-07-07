#pragma once

#include "MatrixOS.h"
#include "ui/UI.h"
#include "applications/Application.h"

class Shell : public Application {
  public:
  static Application_Info info;

  void Setup() override;
  void Loop() override;

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
