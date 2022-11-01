#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"
#include "applications/Application.h"

#define APPLICATION_NAME "Shell"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0xFFFFFF)
#define APPLICATION_VERSION 0
#define APPLICATION_CLASS Shell
#define APPLICATION_VISIBLITY false

class Shell : public Application {
  string name = "Matrix OS Shell";
  string author = "203 Electronics";
  uint32_t version = 10000;

  uint8_t current_page = 0;

  void Loop() override;

  void AddCommonBarInUI(UI* ui);
  void ApplicationLauncher();
};

#include "applications/RegisterApplication.h"