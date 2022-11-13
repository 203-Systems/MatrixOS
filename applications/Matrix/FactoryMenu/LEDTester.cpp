#include "FactoryMenu.h"

void FactoryMenu::LEDTester() {
  uint32_t led_counter = 0;
  const Color colors[8] = {Color(0xFFFFFF), Color(0xFF0000), Color(0xFFFF00), Color(0x00FF00),
                           Color(0x00FFFF), Color(0x0000FF), Color(0xFF00FF), Color(0x000000)};
  MatrixOS::LED::Fill(0);
  while (!MatrixOS::KEYPAD::GetKey(FUNCTION_KEY)->active())
  {
    uint8_t led_index = led_counter % Device::numsOfLED;
    uint8_t led_color_index = led_counter / Device::numsOfLED % (sizeof(colors) / sizeof(Color));

    MatrixOS::LED::SetColor(led_index, colors[led_color_index]);
    MatrixOS::LED::Update();
    led_counter++;
    MatrixOS::KEYPAD::Clear();
    MatrixOS::SYS::DelayMs(30);
  }
  MatrixOS::KEYPAD::Clear();
  MatrixOS::LED::Fill(0);
}