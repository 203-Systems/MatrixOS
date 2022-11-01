#pragma once

#include "MatrixOS.h"
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

  uint8_t color_index = 0;
  const Color colors[7] = {Color(0xFFFFFF), Color(0xFF0000), Color(0xFFFF00), Color(0x00FF00),
                           Color(0x00FFFF), Color(0x0000FF), Color(0xFF00FF)};
};

#include "applications/RegisterApplication.h"