#include "FactoryMenu.h"

void FactoryMenu::LEDTester() {
  InputSnapshot fnSnap = {};
  uint32_t ledCounter = 0;
  const Color colors[8] = {Color::White,    Color(0xFF0000), Color(0xFFFF00), Color(0x00FF00),
                           Color(0x00FFFF), Color(0x0000FF), Color(0xFF00FF), Color(0x000000)};
  MatrixOS::LED::Fill(0);
  uint32_t led_count = MatrixOS::LED::GetLEDCount();
  while (!(MatrixOS::Input::GetState(InputId::FunctionKey(), &fnSnap) &&
           fnSnap.inputClass == InputClass::Keypad && fnSnap.keypad.Active()))
  {
    uint8_t ledIndex = ledCounter % led_count;
    uint8_t ledColorIndex = ledCounter / led_count % (sizeof(colors) / sizeof(Color));

    MatrixOS::LED::SetColor(ledIndex, colors[ledColorIndex]);
    MatrixOS::LED::Update();
    ledCounter++;
    MatrixOS::Input::ClearInputBuffer();
    MatrixOS::SYS::DelayMs(30);
  }
  MatrixOS::Input::ClearInputBuffer();
  MatrixOS::LED::Fill(0);
}
