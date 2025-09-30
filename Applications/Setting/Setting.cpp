#include "Setting.h"
#include "BrightnessControl.h"

#include "USB/USB.h" 

Setting::Setting()
{
  name = "Setting";
  nameColor = Color(0x00FFFF);
}

void Setting::Start() {
  // TODO: Let's assume all dimension are even atm. (No device with odd dimension should exist. Srsly why does Samson
  // Conspiracy exists?) Also assume at least 4x4

  // Brightness Control
  UIButton brightnessBtn;
  brightnessBtn.SetName("Brightness");
  brightnessBtn.SetColor(Color(0xFFFFFF));
  brightnessBtn.SetSize(Dimension(2, 2));
  brightnessBtn.OnPress([]() -> void { MatrixOS::LED::NextBrightness(); });
  brightnessBtn.OnHold([]() -> void { BrightnessControl().Start(); });
  AddUIComponent(brightnessBtn, Point(3, 3));

  // Rotation control and canvas
  UIButton nothingBtn;
  nothingBtn.SetName("This does nothing");
  nothingBtn.SetColor(Color(0x00FF00));
  nothingBtn.SetSize(Dimension(2, 1));
  nothingBtn.OnPress([]() -> void {});
  AddUIComponent(nothingBtn, origin + Point(0, -1));

  UIButton rotateRightBtn;
  rotateRightBtn.SetName("Rotate to this side");
  rotateRightBtn.SetColor(Color(0x00FF00));
  rotateRightBtn.SetSize(Dimension(1, 2));
  rotateRightBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  AddUIComponent(rotateRightBtn, origin + Point(2, 0));

  UIButton rotateDownBtn;
  rotateDownBtn.SetName("Rotate to this side");
  rotateDownBtn.SetColor(Color(0x00FF00));
  rotateDownBtn.SetSize(Dimension(2, 1));
  rotateDownBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(DOWN); });
  AddUIComponent(rotateDownBtn, origin + Point(0, 2));

  UIButton rotateLeftBtn;
  rotateLeftBtn.SetName("Rotate to this side");
  rotateLeftBtn.SetColor(Color(0x00FF00));
  rotateLeftBtn.SetSize(Dimension(1, 2));
  rotateLeftBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(LEFT); });
  AddUIComponent(rotateLeftBtn, origin + Point(-1, 0));

#if DEVICE_STORAGE == 1
  // MSC Mode Button
  UIButton mscModeBtn;
  mscModeBtn.SetName("USB Storage Mode");
  mscModeBtn.SetSize(Dimension(1, 1));
  mscModeBtn.OnPress([]() -> void {
    if (Device::Storage::Available()) {
      MatrixOS::SYS::ExecuteAPP("203 Systems", "MSC Mode");
    }
  });

  // Dynamic color based on storage availability
  mscModeBtn.SetColorFunc([]() -> Color {
    return Color(0xFF8000).DimIfNot(Device::Storage::Available());
  });

  AddUIComponent(mscModeBtn, Point(Device::x_size - 1, Device::y_size - 1));
#endif

  // Device Control
  UIButton deviceIdBtn;
  deviceIdBtn.SetName("Device ID");
  deviceIdBtn.SetColor(Color(0x00FFFF));
  deviceIdBtn.SetSize(Dimension(1, 1));
  deviceIdBtn.OnPress([]() -> void {
    MatrixOS::UserVar::device_id =
        MatrixOS::UIUtility::NumberSelector8x8(MatrixOS::UserVar::device_id, 0x00FFFF, "Device ID", 0, 255);
  });
  AddUIComponent(deviceIdBtn, Point(Device::x_size - 2, Device::y_size - 1));

  UIButton enterDfuBtn;
  enterDfuBtn.SetName("Enter Bootloader Mode");
  enterDfuBtn.SetColor(Color(0xFF0000));
  enterDfuBtn.OnPress([]() -> void { MatrixOS::SYS::Bootloader(); });
  AddUIComponent(enterDfuBtn, Point(0, Device::y_size - 1));

  UIButton resetDevice;
  resetDevice.SetName("Factory Reset Device");
  resetDevice.SetColor(Color(0xFF00FF));
  resetDevice.OnPress([]() -> void { Setting::ResetConfirm(); });
  // resetDevice.SetEnabled(MatrixOS::UserVar::developer_mode);
  AddUIComponent(resetDevice, Point(0, Device::y_size - 2));

  // Infomation
  UIButton osVersionBtn;
  osVersionBtn.SetName("Matrix OS Version");
  osVersionBtn.SetColor(Color(0x00FF30));
  osVersionBtn.OnPress([]() -> void {
    MatrixOS::UIUtility::TextScroll("Matrix OS " + MATRIXOS_VERSION_STRING, Color(0x00FFFF));
  });
  AddUIComponent(osVersionBtn, Point(1, Device::y_size - 1));

  UIButton deviceNameBtn;
  deviceNameBtn.SetName("Device Name");
  deviceNameBtn.SetColor(Color(0x00FF30));
  deviceNameBtn.OnPress([]() -> void {
    MatrixOS::UIUtility::TextScroll(Device::name, Color(0x00FFFF));
  });
  AddUIComponent(deviceNameBtn, Point(2, Device::y_size - 1));

  UIButton deviceSerialBtn;
  deviceSerialBtn.SetName("Device Serial");
  deviceSerialBtn.SetColor(Color(0x00FF30));
  deviceSerialBtn.OnPress([]() -> void {
    MatrixOS::UIUtility::TextScroll(Device::GetSerial(), Color(0x00FFFF));
  });
  AddUIComponent(deviceSerialBtn, Point(3, Device::y_size - 1));

  UIButton deviceSettingsBtn;
  deviceSettingsBtn.SetName("Device Settings");
  deviceSettingsBtn.SetColor(Color(0xFFFFFF));
  deviceSettingsBtn.OnPress([]() -> void {
    Device::DeviceSettings();
  });
  AddUIComponent(deviceSettingsBtn, Point(0, 0));

  UIToggle uiAnimationToggle;
  uiAnimationToggle.SetName("UI Animation");
  uiAnimationToggle.SetColor(Color(0xFFFF00));
  uiAnimationToggle.SetValuePointer(&MatrixOS::UserVar::ui_animation);
  uiAnimationToggle.OnPress([]() -> void { MatrixOS::UserVar::ui_animation.Save(); });
  AddUIComponent(uiAnimationToggle, Point(7, 0));

  UIToggle fastScrollToggle;
  fastScrollToggle.SetName("Fast Text");
  fastScrollToggle.SetColor(Color(0x5A39BD));
  fastScrollToggle.SetValuePointer(&MatrixOS::UserVar::fast_scroll);
  fastScrollToggle.OnPress([]() -> void { MatrixOS::UserVar::fast_scroll.Save(); });
  AddUIComponent(fastScrollToggle, Point(6, 0));

  UI::Start();
}

bool Setting::CustomKeyEvent(KeyEvent* keyEvent) {;
  Point xy = MatrixOS::KeyPad::ID2XY(keyEvent->id);

  if (xy && keyEvent->info.state == RELEASED)  // IF XY is valid, means it's on the main grid
  {
    if ((konami == 0 || konami == 1) && (xy == origin + Point(0, -1) || xy == origin + Point(1, -1)))
    {
      konami++;
      return false;
    }
    else if ((konami == 2 || konami == 3) && (xy == origin + Point(0, 2) || xy == origin + Point(1, 2)))
    {
      konami++;
      return true;
    }
    else if ((konami == 4 || konami == 6) && (xy == origin + Point(-1, 0) || xy == origin + Point(-1, 1)))
    {
      konami++;
      return true;
    }
    else if ((konami == 5 || konami == 7) && (xy == origin + Point(2, 0) || xy == origin + Point(2, 1)))
    {
      konami++;
      if (konami == 8)
      {
        UI ab("A & B", Color(0xFF0000));

        UIButton aBtn;
        aBtn.SetName("A");
        aBtn.SetColor(Color(0xFF0000));
        aBtn.SetSize(Dimension(2, 2));
        aBtn.OnPress([&]() -> void {
          if (konami == 9)
          {
            MatrixOS::SYS::ExecuteAPP("203 Systems", "REDACTED");
          }
          else
          {
            ab.Exit();
          }
        });
        ab.AddUIComponent(aBtn, origin + Point(-2, 0));

        UIButton bBtn;
        bBtn.SetName("B");
        bBtn.SetColor(Color(0xFF0000));
        bBtn.SetSize(Dimension(2, 2));
        bBtn.OnPress([&]() -> void {
          if (konami == 8)
            konami++;
          else
            ab.Exit();
        });
        ab.AddUIComponent(bBtn, origin + Point(2, 0));

        ab.Start();
      }
      return true;
    }
    else
    {
      konami = 0;
      return false;
    }
  }
  return false;
}


void Setting::ResetConfirm() {
  UI confirmResetUI("Confirm Factory Reset", Color(0xFF00FF));

  confirmResetUI.SetPreRenderFunc([]() -> void {
    // C
    MatrixOS::LED::SetColor(Point(0, 0), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(1, 0), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(2, 0), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(0, 1), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(0, 2), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(0, 3), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(1, 3), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(2, 3), Color(0xFF00FF));

    // L
    MatrixOS::LED::SetColor(Point(3, 0), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(Point(3, 1), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(Point(3, 2), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(Point(3, 3), Color(0xFFFFFF));
    MatrixOS::LED::SetColor(Point(4, 3), Color(0xFFFFFF));

    // R
    MatrixOS::LED::SetColor(Point(5, 0), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(6, 0), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(5, 1), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(7, 1), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(5, 2), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(6, 2), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(5, 3), Color(0xFF00FF));
    MatrixOS::LED::SetColor(Point(7, 3), Color(0xFF00FF));
  });

  UIButton cancelResetBtn;
  cancelResetBtn.SetName("Cancel");
  cancelResetBtn.SetColor(Color(0xFFFFFFF));
  cancelResetBtn.SetSize(Dimension(2, 2));
  cancelResetBtn.OnPress([&]() -> void { confirmResetUI.Exit(); });
  confirmResetUI.AddUIComponent(cancelResetBtn, Point(1, 5));

  UIButton confirmResetBtn;
  confirmResetBtn.SetName("Confirm");
  confirmResetBtn.SetColor(Color(0xFF0000));
  confirmResetBtn.SetSize(Dimension(2, 2));
  confirmResetBtn.OnPress([]() -> void {
    Device::NVS::Clear();
    MatrixOS::SYS::Reboot();
  });
  confirmResetUI.AddUIComponent(confirmResetBtn, Point(5, 5));

  confirmResetUI.Start();
}
