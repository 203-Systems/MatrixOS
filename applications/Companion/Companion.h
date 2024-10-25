/*
This is the example application for Matrix OS
Remember to include this header file in the UserApplications.h in the Applications folder
*/

#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"
#include "ui/UI.h"
#include "applications/BrightnessControl/BrightnessControl.h"


class Companion : public Application {
 public:
  static Application_Info info;

  uint8_t canvasLedLayer;
  bool uiOpened = false;
  bool inited = false;

  void Setup() override;
  void Loop() override;
  void End() override;

  void KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo);
  void HIDReportHandler();
  void ActionMenu();
};

inline Application_Info Companion::info = {
    .name = "Companion",
    .author = "203 Systems",
    .color = Color(0x0000FF),
    .version = 1,
    .visibility = true,
};

REGISTER_APPLICATION(Companion);