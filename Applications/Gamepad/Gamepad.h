/*
This is the example application for Matrix OS
Remember to include this header file in the UserApplications.h in the Applications folder
*/

#pragma once

#include "MatrixOS.h"
#include "Application.h"


class Gamepad : public Application {
 public:
  inline static Application_Info info = {
      .name = "Gamepad",
      .author = "203 Systems",
      .color = Color(0x52BD00),
      .version = 1,
      .visibility = true,
  };

  void Setup(const vector<string>& args) override;
  void Loop() override;
  void KeyEventHandler(KeyEvent& keyEvent);

  void ActionMenu();
};


