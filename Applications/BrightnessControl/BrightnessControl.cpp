#include "BrightnessControl.h"
#include "UI4pxFloat.h"

#define GLOBAL_BRIGHTNESS_COLOR Color(0x00FF00)
#define PARTITION_BRIGHTNESS_COLOR Color(0x9800FF)

namespace MatrixOS::LED
{
  extern vector<float> ledBrightnessMultiplier;
}

void BrightnessControl::Start() {
  name = "Brightness Control";
  nameColor = Color::White;

  
  // Brightness Control
  threshold = Device::LED::brightness_level[sizeof(Device::LED::brightness_level) / sizeof(Device::LED::brightness_level[0]) - 1]; //Get the last element
#ifndef FINE_LED_BRIGHTNESS
  map = Device::LED::brightness_level;
  map_length = sizeof(Device::LED::brightness_level) / sizeof(Device::LED::brightness_level[0]);
#else
  map = Device::LED::brightness_fine_level;
  map_length = sizeof(Device::LED::brightness_fine_level) / sizeof(Device::LED::brightness_fine_level[0]);
#endif

  
  // Main Brightness Selector
  Dimension brightnessSelectorDimension = Dimension(8, map_length / 8 + bool(map_length % 8));

  float multiplier = (float)threshold / 100;
  int32_t displayValue = ((float)MatrixOS::UserVar::brightness / multiplier);
  
  UISelector brightnessSelector;
  brightnessSelector.SetName("Brightness Selector");
  brightnessSelector.SetDimension(brightnessSelectorDimension);
  brightnessSelector.SetCount(map_length);
  brightnessSelector.SetLitMode(UISelectorLitMode::LIT_ALWAYS);
  brightnessSelector.SetIndividualColorFunc([&](uint16_t index) -> Color { return (map[index] > threshold ? Color(0xFF0000) : Color::White).DimIfNot(map[index] <= MatrixOS::UserVar::brightness.value); });
  brightnessSelector.OnChange([&](uint8_t value) -> void {
    uint8_t brightness = map[value];
    MatrixOS::LED::SetBrightness(brightness);
    displayValue = brightness / multiplier;
  });
  AddUIComponent(brightnessSelector,  origin + Point(-3, 4) - Point(0, brightnessSelectorDimension.y - 1));

  // Number Display
  UI4pxNumber brightnessDisplay;
  brightnessDisplay.SetName("Brightness");
  brightnessDisplay.SetColorFunc([&](uint16_t digit) -> Color { return digit % 2 ? Color::White : (MatrixOS::UserVar::brightness.value > threshold) ? Color(0xFF0000) : GLOBAL_BRIGHTNESS_COLOR; });
  brightnessDisplay.SetDigits(3);
  brightnessDisplay.SetValuePointer((int32_t*)&displayValue);
  AddUIComponent(brightnessDisplay, origin + Point(-4, -3));

  // LED Partition multiplier

  // Selector UI
  uint16_t ledPartitionSelected = 0;

  UI4pxFloat multiplierDisplay;
  multiplierDisplay.SetName("Brightness Multiplier");
  multiplierDisplay.SetColor(PARTITION_BRIGHTNESS_COLOR);
  multiplierDisplay.SetValuePointer(&MatrixOS::LED::ledBrightnessMultiplier[ledPartitionSelected]);
  multiplierDisplay.SetEnabled(false);
  AddUIComponent(multiplierDisplay, origin + Point(-3, -3));

  vector<float> ledBrightnessMultiplier = {0.0, 
                                            0.1, 0.25, 0.5, 0.75,
                                            1.0, 
                                            1.2, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0, 8.0,
                                            std::numeric_limits<float>::infinity()};

  UIItemSelector<float> multiplierSelector;
  multiplierSelector.SetName("Brightness Multiplier");
  multiplierSelector.SetDimension(Dimension(8, 2));
  multiplierSelector.SetColor(PARTITION_BRIGHTNESS_COLOR);
  multiplierSelector.SetLitMode(UISelectorLitMode::LIT_LESS_EQUAL_THAN);
  multiplierSelector.SetItemPointer(&MatrixOS::LED::ledBrightnessMultiplier[ledPartitionSelected]);
  multiplierSelector.SetCount(ledBrightnessMultiplier.size());
  multiplierSelector.SetItems(ledBrightnessMultiplier.data());
  multiplierSelector.OnChange([&](float value) -> void {
    MatrixOS::LED::SetBrightnessMultiplier(Device::LED::partitions[ledPartitionSelected].name, value);
  });
  
  multiplierSelector.SetEnabled(false);
  AddUIComponent(multiplierSelector, origin + Point(-3, 3));

  UISelector ledPartitionSelector;
  ledPartitionSelector.SetName("LED Partition Selector");
  ledPartitionSelector.SetValuePointer(&ledPartitionSelected);
  ledPartitionSelector.SetIndividualColorFunc([&](uint16_t index) -> Color { return index == 0 ? GLOBAL_BRIGHTNESS_COLOR : PARTITION_BRIGHTNESS_COLOR; });
  ledPartitionSelector.SetIndividualNameFunc([&](uint16_t index) -> string { return index == 0 ? "Global" : Device::LED::partitions[index].name; });
  ledPartitionSelector.SetDimension(Dimension(Device::LED::partitions.size(), 1));
  ledPartitionSelector.OnChange([&](uint16_t ledPartitionSelected) -> void {
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

      multiplierDisplay.SetValuePointer(&MatrixOS::LED::ledBrightnessMultiplier[ledPartitionSelected]);
      multiplierSelector.SetItemPointer(&MatrixOS::LED::ledBrightnessMultiplier[ledPartitionSelected]);

      multiplierDisplay.SetEnabled(true);
      multiplierSelector.SetEnabled(true);

      MatrixOS::LED::FillPartition(Device::LED::partitions[ledPartitionSelected].name, PARTITION_BRIGHTNESS_COLOR);
    }
  });
  ledPartitionSelector.SetEnabled(Device::LED::partitions.size() > 1);
  AddUIComponent(ledPartitionSelector, origin + Point(-3, 4 - brightnessSelectorDimension.y));


  UI::Start();
}
