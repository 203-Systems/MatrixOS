#include "HIDtest.h"
#include "UIKeyboardKey.h"

void HIDtest::Setup() {
    UI keypadUI("", Color(0xFFFFFF));

    keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), KEY_D), Point(1, 2)); // D
    keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), KEY_F), Point(2, 2)); // F

    keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), KEY_J), Point(5, 2)); // J
    keypadUI.AddUIComponent(new UIKeyboardKey(Color(0xFF0000), KEY_K), Point(6, 2)); // K
    
    keypadUI.Start();
    Exit();
}

void HIDtest::Loop() {
    // Do nothing
}