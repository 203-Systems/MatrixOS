#include "BrightnessControl.h"
#include "UITwoToneSelector.h"
#include "UI4pxNumberWithColorFunc.h"

void BrightnessControl::Setup() {
  name = "Brightness Control";
  nameColor = Color(0xFFFFFF);

  // Brightness Control
  threshold = Device::brightness_level[sizeof(Device::brightness_level) / sizeof(Device::brightness_level[0]) - 1];
#ifndef FINE_LED_BRIGHTNESS
  map = Device::brightness_level;
  map_length = sizeof(Device::brightness_level) / sizeof(Device::brightness_level[0]);
#else
  map = Device::fine_brightness_level;
  map_length = sizeof(Device::fine_brightness_level) / sizeof(Device::fine_brightness_level[0]);
#endif

  Dimension dimension = Dimension(8, map_length / 8 + bool(map_length % 8));
  AddUIComponent(new UITwoToneSelector(dimension, map_length, Color(0xFFFFFF), Color(0xFF0000),
                                       (uint8_t*)&MatrixOS::UserVar::brightness.value, threshold, map,
                                       [&](uint8_t value) -> void { MatrixOS::UserVar::brightness.Set(value); }),
                 origin + Point(-3, 4) - Point(0, dimension.y - 1));

  // Number Display
  AddUIComponent(new UI4pxNumberWithColorFunc(
                     [&]() -> Color {
                       return (MatrixOS::UserVar::brightness.value > threshold) ? Color(0xFF0000) : Color(0x00FF00);
                     },
                     3, (int32_t*)&MatrixOS::UserVar::brightness.value, Color(0xFFFFFF)),
                 origin + Point(-4, -3));
}
