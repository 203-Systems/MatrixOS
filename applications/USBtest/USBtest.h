/*
This is the example application for Matrix OS
Remember to include this header file in the UserApplications.h in the Applications folder
*/

#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"

#define APPLICATION_NAME "USB Test"
#define APPLICATION_AUTHOR "203 Systems"
#define APPLICATION_COLOR Color(0xFFFF00)
#define APPLICATION_VERSION 1
#define APPLICATION_CLASS USBtest

class USBtest : public Application {
 public:
  static Application_Info info;

  void Setup() override;
  void Loop() override;
  void KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo);
};

inline Application_Info USBtest::info = {
    .name = "USB Test",
    .author = "203 Systems",
    .color = Color(0xFFFF00),
    .version = 1,
    .visibility = true,
};

REGISTER_APPLICATION(USBtest);