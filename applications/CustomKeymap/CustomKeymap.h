/*
This is the example application for Matrix OS
Remember to include this header file in the UserApplications.h in the Applications folder
*/

#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"

#define APPLICATION_NAME "Custom Keymap"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0xFFFF00)
#define APPLICATION_VERSION 1
#define APPLICATION_CLASS CustomKeymap

class CustomKeymap : public Application {
 public:
  void Setup() override;
  void Loop() override;
  private:
  void* keymap = NULL;
  uint8_t currentLayer = 0;
};

#include "applications/RegisterApplication.h"