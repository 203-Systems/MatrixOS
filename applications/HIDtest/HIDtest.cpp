#include "HIDtest.h"
#include "UIKeyboardKey.h"
#include "UIDPad.h"
#include "UIGamepadKey.h"
#include "UIGamepadAxis.h"

void HIDtest::Setup() {
    UI keypadUI("", Color(0xFFFFFF));

    UIKeyboardKey Dkey(Color(0xFF0000), KEY_D);
    keypadUI.AddUIComponent(Dkey, Point(1, 1));

    UIKeyboardKey Fkey(Color(0xFF0000), KEY_F);
    keypadUI.AddUIComponent(Fkey, Point(2, 1));

    UIKeyboardKey Jkey(Color(0xFF0000), KEY_J);
    keypadUI.AddUIComponent(Jkey, Point(5, 1));

    UIKeyboardKey Kkey(Color(0xFF0000), KEY_K);
    keypadUI.AddUIComponent(Kkey, Point(6, 1));

    UIDPad Dpad(Color(0x00FF00));
    keypadUI.AddUIComponent(Dpad, Point(0, 4));

    UIGamepadKey Akey(Color(0x00FF00), GAMEPAD_A);
    keypadUI.AddUIComponent(Akey, Point(6, 6));

    UIGamepadKey Bkey(Color(0x00FF00), GAMEPAD_B);
    keypadUI.AddUIComponent(Bkey, Point(7, 5));

    UIGamepadKey Xkey(Color(0x00FF00), GAMEPAD_X);
    keypadUI.AddUIComponent(Xkey, Point(5, 5));

    UIGamepadKey Ykey(Color(0x00FF00), GAMEPAD_Y);
    keypadUI.AddUIComponent(Ykey, Point(6, 4));

    UIGamepadKey L1key(Color(0x00FFFF), GAMEPAD_L1);
    keypadUI.AddUIComponent(L1key, Point(0, 3));

    UIGamepadKey R1key(Color(0x00FFFF), GAMEPAD_R1);
    keypadUI.AddUIComponent(R1key, Point(7, 3));

    UIGamepadAxis L2pad(Color(0x00FFFF), GAMEPAD_AXIS_LEFT_TRIGGER, 127);
    keypadUI.AddUIComponent(L2pad, Point(1, 3));

    UIGamepadAxis R2pad(Color(0x00FFFF), GAMEPAD_AXIS_RIGHT_TRIGGER, 127);
    keypadUI.AddUIComponent(R2pad, Point(6, 3));

    UIGamepadKey Startkey(Color(0x0000FF), GAMEPAD_START);
    keypadUI.AddUIComponent(Startkey, Point(3, 7));

    UIGamepadKey Selectkey(Color(0x0000FF), GAMEPAD_SELECT);
    keypadUI.AddUIComponent(Selectkey, Point(4, 7));    
    
    keypadUI.Start();
    Exit();
}

void HIDtest::Loop() {
    // Do nothing
}