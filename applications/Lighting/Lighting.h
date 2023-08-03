#pragma once

#include "MatrixOS.h"
#include "ui/UIInterfaces.h"
#include "applications/Application.h"

class Lighting : public Application {
 public:
  static Application_Info info;
  void Setup() override;
  void Loop() override;

  void KeyEvent(uint16_t keyID, KeyInfo* keyInfo);

  CreateSavedVar("Lighting", color, Color, Color(0xFFFFFF));
};

inline Application_Info Lighting::info = {
    .name = "Lighting",
    .author = "203 Electronics",
    .color = Color(0xFF00FF),
    .version = 1,
    .visibility = true,
};

REGISTER_APPLICATION(Lighting);
