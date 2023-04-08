#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"
#include "UAD.h"

class CustomKeymap : public Application {
 public:
  static Application_Info info;
  
  void Setup() override;
  void Loop() override;
  private:
   UAD uad;
   void KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo);
};

inline Application_Info CustomKeymap::info = {
    .name = "Custom Keymap",
    .author = "203 Electronics",
    .color =  Color(0xFFFF00),
    .version = 1,
    .visibility = true,
};

REGISTER_APPLICATION(CustomKeymap);

