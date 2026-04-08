#pragma once

#include "MatrixOS.h"
#include "UI/UI.h"

class BrightnessControl : public UI {
public:
  // string name = "Setting";
  // Color nameColor = Color(0x00FFFF);

  uint8_t* map;
  uint16_t map_length;
  uint16_t threshold;

  const Point origin = Point((Device::xSize - 1) / 2, (Device::ySize - 1) / 2);

  void Start();
};
