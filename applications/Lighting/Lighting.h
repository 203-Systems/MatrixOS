#pragma once

#include "MatrixOS.h"
#include "UI/UIInterfaces.h"
#include "applications/Application.h"

#define APPLICATION_NAME "Lighting"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0xFF00FF)
#define APPLICATION_VERSION 1
#define APPLICATION_CLASS Lighting

class Lighting : public Application {
 public:
  void Setup() override;
  void Loop() override;

  void KeyEvent(uint16_t keyID, KeyInfo* keyInfo);

  CreateSavedVar(APPLICATION_NAME, color, Color, Color(0xFFFFFF));
};

#include "applications/RegisterApplication.h"