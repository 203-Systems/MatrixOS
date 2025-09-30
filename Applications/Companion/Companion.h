/*
This is the example application for Matrix OS
Remember to include this header file in the UserApplications.h in the Applications folder
*/

#pragma once

#include "MatrixOS.h"
#include "Application.h"
#include "UI/UI.h"


class Companion : public Application {
 public:
  inline static Application_Info info = {
      .name = "Companion",
      .author = "203 Systems",
      .color = Color(0x0000FF),
      .version = 1,
      .visibility = true,
  };

  uint8_t canvasLedLayer;
  bool uiOpened = false;
  bool inited = false;

  void Setup(const vector<string>& args) override;
  void Loop() override;
  void End() override;

  void KeyEventHandler(KeyEvent& keyEvent);
  void HIDReportHandler();
  void ActionMenu();
};


