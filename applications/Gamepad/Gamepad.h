/*
This is the example application for Matrix OS
Remember to include this header file in the UserApplications.h in the Applications folder
*/

#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"
#include "applications/BrightnessControl/BrightnessControl.h"


class Gamepad : public Application {
 public:
  static Application_Info info;

  void Setup() override;
  void Loop() override;
  void KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo);

  void ActionMenu();
};

inline Application_Info Gamepad::info = {
    .name = "Gamepad",
    .author = "203 Electronics",
    .color = Color(0x00FF00),
    .version = 1,
    .visibility = true,
};

REGISTER_APPLICATION(Gamepad);