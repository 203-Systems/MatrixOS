#include "BrightnessControl.h"
#include "UITwoToneSelector.h"
#include "UI4pxNumberWithColorFunc.h"
#include "UI4pxFloat.h"
#include "UILEDPartitionSelector.h"


#define GLOBAL_BRIGHTNESS_COLOR Color(0x00FF00)
#define PARTITION_BRIGHTNESS_COLOR Color(0x9800FF)

namespace MatrixOS::LED
{
  extern vector<float> ledBrightnessMultiplyer;
}

void BrightnessControl::Start() {
  name = "Brightness Control";
  nameColor = Color(0xFFFFFF);
  
  // Brightness Control
  threshold = Device::led_brightness_level[sizeof(Device::led_brightness_level) / sizeof(Device::led_brightness_level[0]) - 1]; //Get the last element
#ifndef FINE_LED_BRIGHTNESS
  map = Device::led_brightness_level;
  map_length = sizeof(Device::led_brightness_level) / sizeof(Device::led_brightness_level[0]);
#else
  map = Device::led_brightness_fine_level;
  map_length = sizeof(Device::led_brightness_fine_level) / sizeof(Device::led_brightness_fine_level[0]);
#endif

  
  // Main Brightness Selector
  Dimension brightnessSelectorDimension = Dimension(8, map_length / 8 + bool(map_length % 8));

  float multiplier = (float)threshold / 100;
  int32_t displayValue = ((float)MatrixOS::UserVar::brightness / multiplier);

  UITwoToneSelector brightnessSelector(brightnessSelectorDimension, map_length, Color(0xFFFFFF), Color(0xFF0000), (uint8_t*)&MatrixOS::UserVar::brightness.value, threshold, map,
                                       [&](uint8_t value) -> void {
                                         MatrixOS::LED::SetBrightness(value);
                                         displayValue = value / multiplier;
                                       });
  AddUIComponent(brightnessSelector,
                 origin + Point(-3, 4) - Point(0, brightnessSelectorDimension.y - 1));

  // Number Display
  UI4pxNumberWithColorFunc brightnessDisplay([&]() -> Color { return (MatrixOS::UserVar::brightness.value > threshold) ? Color(0xFF0000) : GLOBAL_BRIGHTNESS_COLOR; }, 3,
                                      (int32_t*)&displayValue, Color(0xFFFFFF));
  AddUIComponent(brightnessDisplay, origin + Point(-4, -3));

  // LED Partition multiplier

  // Selector UI
  uint8_t ledPartitionSelected = 0;

  UI4pxFloat multiplierDisplay(PARTITION_BRIGHTNESS_COLOR, &MatrixOS::LED::ledBrightnessMultiplyer[ledPartitionSelected]);
  multiplierDisplay.SetEnabled(false);
  AddUIComponent(multiplierDisplay, origin + Point(-3, -3));

  vector<float> ledBrightnessMultiplyer = {0.0, 
                                            0.1, 0.25, 0.5, 0.75,
                                            1.0, 
                                            1.2, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0, 8.0,
                                            std::numeric_limits<float>::infinity()};

  UINumItemSelector<float> multiplierSelector(Dimension(8, 2), PARTITION_BRIGHTNESS_COLOR, &MatrixOS::LED::ledBrightnessMultiplyer[ledPartitionSelected], ledBrightnessMultiplyer.size(), ledBrightnessMultiplyer.data(), [&](float value) -> void {
    MatrixOS::LED::SetBrightnessMultiplier(Device::led_partitions[ledPartitionSelected].name, value);
  });
  multiplierSelector.SetEnabled(false);
  AddUIComponent(multiplierSelector, origin + Point(-3, 3));

  UILEDPartitionSelector ledPartitionSelector(GLOBAL_BRIGHTNESS_COLOR, &ledPartitionSelected, PARTITION_BRIGHTNESS_COLOR, [&](uint8_t ledPartitionSelected) -> void {
    MatrixOS::LED::Fill(0);
    if(ledPartitionSelected == 0)
    {
      brightnessSelector.SetEnabled(true);
      brightnessDisplay.SetEnabled(true);

      multiplierDisplay.SetEnabled(false);
      multiplierSelector.SetEnabled(false);
    }
    else
    {
      brightnessSelector.SetEnabled(false);
      brightnessDisplay.SetEnabled(false);

      multiplierDisplay.value = &MatrixOS::LED::ledBrightnessMultiplyer[ledPartitionSelected];
      multiplierSelector.output = &MatrixOS::LED::ledBrightnessMultiplyer[ledPartitionSelected];

      multiplierDisplay.SetEnabled(true);
      multiplierSelector.SetEnabled(true);

      MatrixOS::LED::FillPartition(Device::led_partitions[ledPartitionSelected].name, PARTITION_BRIGHTNESS_COLOR);
    }
  });
  ledPartitionSelector.SetEnabled(Device::led_partitions.size() > 1);
  AddUIComponent(ledPartitionSelector, origin + Point(-3, 4 - brightnessSelectorDimension.y));


  UI::Start();
}
