#include "HIDtest.h"
#include "UIKeyboardKey.h"

void HIDtest::Setup() {
    // UI keypadUI("", Color(0xFFFFFF));

    // keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), 0x07), Point(1, 2)); // D
    // keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), 0x09), Point(2, 2)); // F

    // keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), 0x0D), Point(5, 2)); // J
    // keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), 0x0E), Point(6, 2)); // K
    
    // keypadUI.Start();
    // Exit();

    MatrixOS::LED::SetColor(Point(1, 2), Color(0xFF0000));
    MatrixOS::LED::SetColor(Point(2, 2), Color(0xFF0000));
    MatrixOS::LED::SetColor(Point(5, 2), Color(0xFF0000));
    MatrixOS::LED::SetColor(Point(6, 2), Color(0xFF0000));
    MatrixOS::LED::Update();
}

void HIDtest::Loop() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEventHandler(keyEvent.id, &keyEvent.info); }
}

void HIDtest::KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo) {
  Point xy = MatrixOS::KEYPAD::ID2XY(KeyID);
  if (xy)  // IF XY is vaild, means it's on the main grid
  {
    uint8_t keycode = 0;
    if(xy == Point(1, 2))
    {
        keycode = 0x07;
    }
    else if(xy == Point(2, 2))
    {
        keycode = 0x09;
    }
    else if(xy == Point(5, 2))
    {
        keycode = 0x0D;
    }
    else if(xy == Point(6, 2))
    {
        keycode = 0x0E;
    }

    if(keycode != 0)
    {
        if(keyInfo->state == PRESSED)
        {
            MatrixOS::HID::KeyboardPress(keycode);
            MatrixOS::LED::SetColor(xy, Color(0xFFFFFF));
            MatrixOS::LED::Update();
        }
        else if(keyInfo->state == RELEASED)
        {
            MatrixOS::HID::KeyboardRelease(keycode);
            MatrixOS::LED::SetColor(xy, Color(0xFF0000));
            MatrixOS::LED::Update();
        }
    }
  }
}
