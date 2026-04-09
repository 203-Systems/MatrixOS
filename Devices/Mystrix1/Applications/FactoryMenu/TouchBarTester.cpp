#include "FactoryMenu.h"

void FactoryMenu::TouchBarTester() {
#ifdef FAMILY_MYSTRIX
  InputSnapshot fnSnap = {};
  bool touchbarTested[32];
  memset(touchbarTested, false, 32);

  MatrixOS::LED::Fill(0);
  Device::touchbarEnable.TempSet(true);
  while (!(MatrixOS::Input::GetState(InputId::FunctionKey(), &fnSnap) &&
           fnSnap.inputClass == InputClass::Keypad && fnSnap.keypad.Active()))
  {
    // Left
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = Point(-1, i);
      Point ledXY = Point(0, i);
      uint8_t testedIndex = i;

      KeypadInfo keypadState = MatrixOS::Input::GetKeypadState(xy);
      touchbarTested[testedIndex] |= keypadState.Active();

      MatrixOS::LED::SetColor(ledXY, keypadState.Active() ? Color(0x00FF00) : Color(0xFFFFFF * touchbarTested[testedIndex]));
    }

    // Right
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = Point(8, i);
      Point ledXY = Point(7, i);
      uint8_t testedIndex = i + 8;

      KeypadInfo keypadState = MatrixOS::Input::GetKeypadState(xy);
      touchbarTested[testedIndex] |= keypadState.Active();

      MatrixOS::LED::SetColor(ledXY, keypadState.Active() ? Color(0x00FF00) : Color(0xFFFFFF * touchbarTested[testedIndex]));
    }

    // Top (When Matrix is rotated)
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = Point(i, -1);
      Point ledXY = Point(i, 0);
      uint8_t testedIndex = i + 16;

      KeypadInfo keypadState = MatrixOS::Input::GetKeypadState(xy);
      touchbarTested[testedIndex] |= keypadState.Active();

      MatrixOS::LED::SetColor(ledXY, keypadState.Active() ? Color(0x00FF00) : Color(0xFFFFFF * touchbarTested[testedIndex]));
    }

    // Bottom (When Matrix is rotated)
    for (uint8_t i = 0; i < 8; i++)
    {
      Point xy = Point(i, 8);
      Point ledXY = Point(i, 7);
      uint8_t testedIndex = i + 24;

      KeypadInfo keypadState = MatrixOS::Input::GetKeypadState(xy);
      touchbarTested[testedIndex] |= keypadState.Active();

      MatrixOS::LED::SetColor(ledXY, keypadState.Active() ? Color(0x00FF00) : Color(0xFFFFFF * touchbarTested[testedIndex]));
    }
    MatrixOS::LED::Update();
  }
  Device::touchbarEnable.Load();
  MatrixOS::Input::ClearState();
  MatrixOS::LED::Fill(0);
#endif
}
