#pragma once
// #ifdef GRID_8x8

// This technically not an APP but a UI so it coexists with an active APP.

#include "MatrixOS.h"
#include "ui/UI.h"

class Setting : public UI {
  public:
  // string name = "Setting";
  // Color nameColor = Color(0x00FFFF);

   Setting();

   const Point origin = Point((Device::x_size - 1) / 2, (Device::y_size - 1) / 2);

   void Start();

   static void RotateClockwise(EDirection rotation);
   static void NextBrightness();

   bool CustomKeyEvent(KeyEvent* keyEvent);

  private:
  uint8_t konami = 0;
};
