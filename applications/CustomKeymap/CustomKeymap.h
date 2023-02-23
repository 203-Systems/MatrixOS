#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"
#include "UAD.h"

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
   UAD uad;
   void KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo);
};

#include "applications/RegisterApplication.h"