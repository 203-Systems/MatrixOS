#include "Gamepad.h"
#include "UIKeyboardKey.h"
#include "UIDPad.h"
#include "UIGamepadKey.h"
#include "UIGamepadAxis.h"

void Gamepad::Setup(const vector<string>& args) {
  UI gamepadUI("", Color::White);

  UIKeyboardKey sKey(Color(0xFF0000), KEY_S);
  gamepadUI.AddUIComponent(sKey, Point(0, 1));

  UIKeyboardKey dKey(Color(0xFF0000), KEY_D);
  gamepadUI.AddUIComponent(dKey, Point(1, 1));

  UIKeyboardKey fKey(Color(0xFF0000), KEY_F);
  gamepadUI.AddUIComponent(fKey, Point(2, 1));

  UIKeyboardKey jKey(Color(0xFF0000), KEY_J);
  gamepadUI.AddUIComponent(jKey, Point(5, 1));

  UIKeyboardKey kKey(Color(0xFF0000), KEY_K);
  gamepadUI.AddUIComponent(kKey, Point(6, 1));

  UIKeyboardKey lKey(Color(0xFF0000), KEY_L);
  gamepadUI.AddUIComponent(lKey, Point(7, 1));

  UIDPad dPad(Color(0x00FF00));
  gamepadUI.AddUIComponent(dPad, Point(0, 4));

  UIGamepadKey aKey(Color(0x00FF00), 0);
  gamepadUI.AddUIComponent(aKey, Point(6, 6));

  UIGamepadKey bKey(Color(0x00FF00), 1);
  gamepadUI.AddUIComponent(bKey, Point(7, 5));

  UIGamepadKey xKey(Color(0x00FF00), 2);
  gamepadUI.AddUIComponent(xKey, Point(5, 5));

  UIGamepadKey yKey(Color(0x00FF00), 3);
  gamepadUI.AddUIComponent(yKey, Point(6, 4));

  UIGamepadKey l1Key(Color(0x00FFFF), 10);
  gamepadUI.AddUIComponent(l1Key, Point(0, 3));

  UIGamepadKey r1Key(Color(0x00FFFF), 11);
  gamepadUI.AddUIComponent(r1Key, Point(7, 3));

  UIGamepadAxis l2Pad(Color(0x00FFFF), GAMEPAD_AXIS_LEFT_TRIGGER, 32767);
  gamepadUI.AddUIComponent(l2Pad, Point(1, 3));

  UIGamepadAxis r2Pad(Color(0x00FFFF), GAMEPAD_AXIS_RIGHT_TRIGGER, 32767);
  gamepadUI.AddUIComponent(r2Pad, Point(6, 3));

  UIGamepadKey selectKey(Color(0x0000FF), 6);
  gamepadUI.AddUIComponent(selectKey, Point(3, 7));

  UIGamepadKey startKey(Color(0x0000FF), 7);
  gamepadUI.AddUIComponent(startKey, Point(4, 7));

  gamepadUI.SetKeyEventHandler([&](InputEvent* inputEvent) -> bool {
    if (inputEvent->id.IsFunctionKey())
    {
      if (inputEvent->keypad.state == KeypadState::Pressed)
      {
        ActionMenu();
      }
      return true; // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
  });

  gamepadUI.Start();
  Exit();
}

void Gamepad::ActionMenu() {
  UI actionMenu("Action Menu", Color(0x00FF00), true);

  UIButton systemSettingBtn;
  systemSettingBtn.SetName("System Setting");
  systemSettingBtn.SetColor(Color::White);
  systemSettingBtn.OnPress([&]() -> void { MatrixOS::SYS::OpenSetting(); });
  actionMenu.AddUIComponent(systemSettingBtn, Point(7, 7));

  actionMenu.SetKeyEventHandler([&](InputEvent* inputEvent) -> bool {
    if (inputEvent->id.IsFunctionKey())
    {
      if (inputEvent->keypad.state == KeypadState::Hold)
      {
        Exit();
      }
      else if (inputEvent->keypad.state == KeypadState::Released)
      {
        actionMenu.Exit();
      }
      return true; // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
  });

  actionMenu.Start();
}

void Gamepad::Loop() {
  // Do nothing
}