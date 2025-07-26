#pragma once

#include "MatrixOS.h"
#include "ui/UI.h"
#include "Application.h"

class Shell : public Application {
  public:
  inline static Application_Info info = {
      .name = "Shell",
      .author = "203 Systems",
      .color = Color(0x00FFAA),
      .version = 1,
      .visibility = false,
  };

  void Setup() override;
  void Loop() override;

  void ApplicationLauncher();
  void HiddenApplicationLauncher();
  void LaunchAnimation(Point origin, Color color);
};


