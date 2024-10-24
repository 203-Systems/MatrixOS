#include "Gamepad.h"
#include "UIKeyboardKey.h"
#include "UIDPad.h"
#include "UIGamepadKey.h"
#include "UIGamepadAxis.h"

void Gamepad::Setup() {
    UI gamepadUI("", Color(0xFFFFFF));

    UIKeyboardKey Skey(Color(0xFF0000), KEY_S);
    gamepadUI.AddUIComponent(Skey, Point(0, 1));

    UIKeyboardKey Dkey(Color(0xFF0000), KEY_D);
    gamepadUI.AddUIComponent(Dkey, Point(1, 1));

    UIKeyboardKey Fkey(Color(0xFF0000), KEY_F);
    gamepadUI.AddUIComponent(Fkey, Point(2, 1));

    UIKeyboardKey Jkey(Color(0xFF0000), KEY_J);
    gamepadUI.AddUIComponent(Jkey, Point(5, 1));

    UIKeyboardKey Kkey(Color(0xFF0000), KEY_K);
    gamepadUI.AddUIComponent(Kkey, Point(6, 1));

    UIKeyboardKey Lkey(Color(0xFF0000), KEY_L);
    gamepadUI.AddUIComponent(Lkey, Point(7, 1));

    UIDPad Dpad(Color(0x00FF00));
    gamepadUI.AddUIComponent(Dpad, Point(0, 4));

    UIGamepadKey Akey(Color(0x00FF00), GAMEPAD_A);
    gamepadUI.AddUIComponent(Akey, Point(6, 6));

    UIGamepadKey Bkey(Color(0x00FF00), GAMEPAD_B);
    gamepadUI.AddUIComponent(Bkey, Point(7, 5));

    UIGamepadKey Xkey(Color(0x00FF00), GAMEPAD_X);
    gamepadUI.AddUIComponent(Xkey, Point(5, 5));

    UIGamepadKey Ykey(Color(0x00FF00), GAMEPAD_Y);
    gamepadUI.AddUIComponent(Ykey, Point(6, 4));

    UIGamepadKey L1key(Color(0x00FFFF), GAMEPAD_L1);
    gamepadUI.AddUIComponent(L1key, Point(0, 3));

    UIGamepadKey R1key(Color(0x00FFFF), GAMEPAD_R1);
    gamepadUI.AddUIComponent(R1key, Point(7, 3));

    UIGamepadAxis L2pad(Color(0x00FFFF), GAMEPAD_AXIS_LEFT_TRIGGER, 127);
    gamepadUI.AddUIComponent(L2pad, Point(1, 3));

    UIGamepadAxis R2pad(Color(0x00FFFF), GAMEPAD_AXIS_RIGHT_TRIGGER, 127);
    gamepadUI.AddUIComponent(R2pad, Point(6, 3));

    UIGamepadKey Selectkey(Color(0x0000FF), GAMEPAD_SELECT);
    gamepadUI.AddUIComponent(Selectkey, Point(3, 7));    

    UIGamepadKey Startkey(Color(0x0000FF), GAMEPAD_START);
    gamepadUI.AddUIComponent(Startkey, Point(4, 7));
    
    gamepadUI.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == PRESSED)
      {
        ActionMenu();
      }
      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
  });

    gamepadUI.Start();
    Exit();
}


void Gamepad::ActionMenu() {
  UI actionMenu("Action Menu", Color(0x00FF00), true);

  UIButton brightnessBtn;
  brightnessBtn.SetName("Brightness");
  brightnessBtn.SetColor(Color(0xFFFFFF));
  brightnessBtn.SetSize(Dimension(2, 2));
  brightnessBtn.OnPress([&]() -> void { MatrixOS::LED::NextBrightness(); });
  brightnessBtn.OnHold([&]() -> void { BrightnessControl().Start(); });
  actionMenu.AddUIComponent(brightnessBtn, Point(3, 3));

  UIButton rotateUpBtn;
  rotateUpBtn.SetName("This Does Nothing");
  rotateUpBtn.SetColor(Color(0x00FF00));
  rotateUpBtn.SetSize(Dimension(2, 1));
  actionMenu.AddUIComponent(rotateUpBtn, Point(3, 2));

  UIButton rotateRightBtn;
  rotateRightBtn.SetName("Rotate to this side");
  rotateRightBtn.SetColor(Color(0x00FF00));
  rotateRightBtn.SetSize(Dimension(1, 2));
  rotateRightBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  actionMenu.AddUIComponent(rotateRightBtn, Point(5, 3));

  UIButton rotateDownBtn;
  rotateDownBtn.SetName("Rotate to this side");
  rotateDownBtn.SetColor(Color(0x00FF00));
  rotateDownBtn.SetSize(Dimension(2, 1));
  rotateDownBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(DOWN); });
  actionMenu.AddUIComponent(rotateDownBtn, Point(3, 5));

  UIButton rotateLeftBtn;
  rotateLeftBtn.SetName("Rotate to this side");
  rotateLeftBtn.SetColor(Color(0x00FF00));
  rotateLeftBtn.SetSize(Dimension(1, 2));
  rotateLeftBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(LEFT); });
  actionMenu.AddUIComponent(rotateLeftBtn, Point(2, 3));

UIButton systemSettingBtn;
  systemSettingBtn.SetName("System Setting");
  systemSettingBtn.SetColor(Color(0xFFFFFF));
  systemSettingBtn.OnPress([&]() -> void { MatrixOS::SYS::OpenSetting(); });
  actionMenu.AddUIComponent(systemSettingBtn, Point(7, 7));

  actionMenu.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    if (keyEvent->id == FUNCTION_KEY)
    {
      if (keyEvent->info.state == HOLD)
      {
        Exit();
      }
      else if (keyEvent->info.state == RELEASED)
      {
        actionMenu.Exit();
      }
      return true;  // Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
  });

  actionMenu.Start();
}

void Gamepad::Loop() {
    // Do nothing
}