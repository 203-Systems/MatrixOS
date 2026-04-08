#include "FactoryMenu.h"

void FactoryMenu::TouchBarTester() {
#ifdef FAMILY_MYSTRIX
  bool touchbarTested[32];
  memset(touchbarTested, false, 32);

  MatrixOS::LED::Fill(0);
  Device::touchbarEnable.TempSet(true);
  while (!MatrixOS::KeyPad::GetKey(FUNCTION_KEY)->Active()) // TODO Factor in the rotation or limit rotation
  {
    // Left
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = Point(-1, i);
      Point ledXY = Point(0, i);
      uint8_t testedIndex = i;

      KeyInfo* keyInfo = MatrixOS::KeyPad::GetKey(xy);
      if (keyInfo == nullptr)
        continue;
      touchbarTested[testedIndex] |= keyInfo->Active();

      MatrixOS::LED::SetColor(ledXY, keyInfo->Active() ? Color(0x00FF00) : Color(0xFFFFFF * touchbarTested[testedIndex]));
    }

    // Right
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = Point(8, i);
      Point ledXY = Point(7, i);
      uint8_t testedIndex = i + 8;

      KeyInfo* keyInfo = MatrixOS::KeyPad::GetKey(xy);
      if (keyInfo == nullptr)
        continue;
      touchbarTested[testedIndex] |= keyInfo->Active();

      MatrixOS::LED::SetColor(ledXY, keyInfo->Active() ? Color(0x00FF00) : Color(0xFFFFFF * touchbarTested[testedIndex]));
    }

    // Top (When Matrix is rotated)
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = Point(i, -1);
      Point ledXY = Point(i, 0);
      uint8_t testedIndex = i + 16;

      KeyInfo* keyInfo = MatrixOS::KeyPad::GetKey(xy);
      if (keyInfo == nullptr)
        continue;
      touchbarTested[testedIndex] |= keyInfo->Active();

      MatrixOS::LED::SetColor(ledXY, keyInfo->Active() ? Color(0x00FF00) : Color(0xFFFFFF * touchbarTested[testedIndex]));
    }

    // Bottom (When Matrix is rotated)
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = Point(i, 8);
      Point ledXY = Point(i, 7);
      uint8_t testedIndex = i + 24;

      KeyInfo* keyInfo = MatrixOS::KeyPad::GetKey(xy);
      if (keyInfo == nullptr)
        continue;
      touchbarTested[testedIndex] |= keyInfo->Active();

      MatrixOS::LED::SetColor(ledXY, keyInfo->Active() ? Color(0x00FF00) : Color(0xFFFFFF * touchbarTested[testedIndex]));
    }
    MatrixOS::LED::Update();
  }
  Device::touchbarEnable.Load();
  MatrixOS::KeyPad::Clear();
  MatrixOS::LED::Fill(0);
#endif
}