#pragma once

#include "MatrixOS.h"
#include "UAD.h"
#include "applications/Application.h"
#include "UILayerControl.h"

class CustomControlMap : public Application {
 public:
  static Application_Info info;

  void Setup() override;
  void Loop() override;

 private:
  UAD uad;
  bool menuLock = false;

  void KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo);
  void Reload();
  void ActionMenu();
  void HIDReportHandler();
};

inline Application_Info CustomControlMap::info = {
    .name = "Custom Control Map",
    .author = "203 Systems",
    .color = Color(0xFFFF00),
    .version = 1,
    .visibility = true,
};

REGISTER_APPLICATION(CustomControlMap);
