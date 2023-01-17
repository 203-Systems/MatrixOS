/*
This is the example application for Matrix OS
Remember to include this header file in the UserApplications.h in the Applications folder
*/

#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"

#define APPLICATION_NAME "HID Test"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0xFFFF00)
#define APPLICATION_VERSION 1
#define APPLICATION_CLASS HIDtest

class HIDtest : public Application {
 public:
  void Setup() override;
  void Loop() override;
  void KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo);
};

#include "applications/RegisterApplication.h"