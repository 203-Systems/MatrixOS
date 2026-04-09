#pragma once

// This technically not an APP but a UI so it coexists with an active APP.

#include "MatrixOS.h"
#include "UI/UI.h"

class Setting {
public:
  Setting() = default;

  const Point origin = Point((MatrixOS::Input::GetPrimaryGridCluster()->dimension.x - 1) / 2, (MatrixOS::Input::GetPrimaryGridCluster()->dimension.y - 1) / 2);

  void SystemSetting();

  static void RotateClockwise(Direction rotation);
  static void NextBrightness();
  static void ResetConfirm();

  bool CustomInputEvent(InputEvent* inputEvent);

private:
  void SecretMenu();
  uint8_t konami = 0;
};
