#include "HIDtest.h"
#include "UIKeyboardKey.h"

void HIDtest::Setup() {
    UI keypadUI("Keypad", Color(0xFFFFFF));

    keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), 0x07), Point(1, 2)); // D
    keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), 0x08), Point(2, 2)); // F

    keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), 0x0D), Point(5, 2)); // J
    keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), 0x0E), Point(6, 2)); // K
    
    keypadUI.Start();
    Exit();
}

void HIDtest::Loop() {}