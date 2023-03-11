#include "FactoryMenu.h"

void FactoryMenu::TouchBarTester() {
  bool touchbar_tested[32];
  memset(touchbar_tested, false, 32);

  MatrixOS::LED::Fill(0);
  Device::touchbar_enable.TempSet(true);
  while (!MatrixOS::KEYPAD::GetKey(FUNCTION_KEY)->active())  // TODO Factor in the rotation or limit rotation
  {
    // Left
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = Point(-1, i);
      Point led_xy = Point(0, i);
      uint8_t tested_index = i;

      KeyInfo* keyInfo = MatrixOS::KEYPAD::GetKey(xy);
      if(keyInfo == nullptr)
        continue;
      touchbar_tested[tested_index] |= keyInfo->active();

      MatrixOS::LED::SetColor(led_xy,
                              keyInfo->active() ? Color(0x00FF00) : Color(0xFFFFFF * touchbar_tested[tested_index]));
    }

    // Right
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = Point(8, i);
      Point led_xy = Point(7, i);
      uint8_t tested_index = i + 8;

      KeyInfo* keyInfo = MatrixOS::KEYPAD::GetKey(xy);
      if(keyInfo == nullptr)
          continue;
      touchbar_tested[tested_index] |= keyInfo->active();

      MatrixOS::LED::SetColor(led_xy,
                              keyInfo->active() ? Color(0x00FF00) : Color(0xFFFFFF * touchbar_tested[tested_index]));
    }

    //Top (When Matrix is rotated)
    for(uint8_t i = 0; i < 8; i++)
    {
        Point xy = Point(i, -1);
        Point led_xy = Point(i, 0);
        uint8_t tested_index = i + 16;

        KeyInfo* keyInfo = MatrixOS::KEYPAD::GetKey(xy);
        if(keyInfo == nullptr)
          continue;
        touchbar_tested[tested_index] |= keyInfo->active();

        MatrixOS::LED::SetColor(led_xy, keyInfo->active() ? Color(0x00FF00) : Color(0xFFFFFF *
        touchbar_tested[tested_index]));
    }

    //Bottom (When Matrix is rotated)
    for(uint8_t i = 0; i < 8; i++)
    {
        Point xy = Point(i, 8);
        Point led_xy = Point(i, 7);
        uint8_t tested_index = i + 24;

        KeyInfo* keyInfo = MatrixOS::KEYPAD::GetKey(xy);
        if(keyInfo == nullptr)
          continue;
        touchbar_tested[tested_index] |= keyInfo->active();

        MatrixOS::LED::SetColor(led_xy, keyInfo->active() ? Color(0x00FF00) : Color(0xFFFFFF *
        touchbar_tested[tested_index]));
    }
    MatrixOS::LED::Update();
  }
  Device::touchbar_enable.Load();
  MatrixOS::KEYPAD::Clear();
  MatrixOS::LED::Fill(0);
}