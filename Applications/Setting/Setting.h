#pragma once

// This technically not an APP but a UI so it coexists with an active APP.

#include "MatrixOS.h"
#include "UI/UI.h"

class Setting {
public:
  Setting() = default;

  const Point origin = Point((Device::xSize - 1) / 2, (Device::ySize - 1) / 2);

  void SystemSetting();

  static void RotateClockwise(Direction rotation);
  static void NextBrightness();
  static void ResetConfirm();

  bool CustomKeyEvent(KeyEvent* keyEvent);

private:
  void SecretMenu();
  uint8_t konami = 0;
};
