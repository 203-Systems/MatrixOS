#include "Setting.h"
#include "applications/BrightnessControl/BrightnessControl.h"

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

  UIButton rotatRightBtn;
  rotatRightBtn.SetName("Rotate to this side");
  rotatRightBtn.SetColor(Color(0x00FF00));
  rotatRightBtn.SetSize(Dimension(1, 2));
  rotatRightBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  AddUIComponent(rotatRightBtn, origin + Point(2, 0));

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

  // Device Control
  UIButton deviceIdBtn;
  deviceIdBtn.SetName("Device ID");
  deviceIdBtn.SetColor(Color(0x00FFFF));
  deviceIdBtn.SetSize(Dimension(1, 1));
  deviceIdBtn.OnPress([]() -> void {
    MatrixOS::UserVar::device_id =
        MatrixOS::UIInterface::NumberSelector8x8(MatrixOS::UserVar::device_id, 0x00FFFF, "Device ID", 0, 255);
  });
  AddUIComponent(deviceIdBtn, Point(Device::x_size - 1, Device::y_size - 1));

  UIButton enterDfuBtn;
  enterDfuBtn.SetName("Enter DFU Mode");
  enterDfuBtn.SetColor(Color(0xFF0000));
  enterDfuBtn.OnPress([]() -> void { MatrixOS::SYS::Bootloader(); });
  AddUIComponent(enterDfuBtn, Point(0, Device::y_size - 1));

  UIButton resetDevice;
  resetDevice.SetName("Reset Device");
  resetDevice.SetColor(Color(0xFF0000));
  resetDevice.OnPress([]() -> void {
    Device::NVS::Clear();
    Device::Reboot();
  });
  resetDevice.SetEnabled(MatrixOS::UserVar::developer_mode);
  AddUIComponent(resetDevice, Point(0, Device::y_size - 2));

  // Infomation
  UIButton osVersionBtn;
  osVersionBtn.SetName("Matrix OS Version");
  osVersionBtn.SetColor(Color(0x00FF30));
  osVersionBtn.OnPress([]() -> void {
    MatrixOS::UIInterface::TextScroll("Matrix OS " MATRIXOS_VERSION_STRING, Color(0x00FFFF));
  });
  AddUIComponent(osVersionBtn, Point(1, Device::y_size - 1));

  UIButton deviceNameBtn;
  deviceNameBtn.SetName("Device Name");
  deviceNameBtn.SetColor(Color(0x00FF30));
  deviceNameBtn.OnPress([]() -> void {
    MatrixOS::UIInterface::TextScroll(Device::name, Color(0x00FFFF));
  });
  AddUIComponent(deviceNameBtn, Point(2, Device::y_size - 1));

  UIButton deviceSerialBtn;
  deviceSerialBtn.SetName("Device Serial");
  deviceSerialBtn.SetColor(Color(0x00FF30));
  deviceSerialBtn.OnPress([]() -> void {
    MatrixOS::UIInterface::TextScroll(Device::GetSerial(), Color(0x00FFFF));
  });
  AddUIComponent(deviceSerialBtn, Point(3, Device::y_size - 1));

  UIButton deviceSettingsBtn;
  deviceSettingsBtn.SetName("Device Settings");
  deviceSettingsBtn.SetColor(Color(0xFFFFFF));
  deviceSettingsBtn.OnPress([]() -> void {
    Device::DeviceSettings();
  });
  AddUIComponent(deviceSettingsBtn, Point(0, 0));


  UI::Start();
}

bool Setting::CustomKeyEvent(KeyEvent* keyEvent) {
  MLOGD("Konami", "Custom key event");
  Point xy = MatrixOS::KEYPAD::ID2XY(keyEvent->id);

  if (xy && keyEvent->info.state == RELEASED)  // IF XY is vaild, means it's on the main grid
  {
    if ((konami == 0 || konami == 1) && (xy == origin + Point(0, -1) || xy == origin + Point(1, -1)))
    {
      konami++;
      MLOGD("Konami", "Up prssed, %d", konami);
      return false;
    }
    else if ((konami == 2 || konami == 3) && (xy == origin + Point(0, 2) || xy == origin + Point(1, 2)))
    {
      konami++;
      MLOGD("Konami", "Down prssed, %d", konami);
      return true;
    }
    else if ((konami == 4 || konami == 6) && (xy == origin + Point(-1, 0) || xy == origin + Point(-1, 1)))
    {
      konami++;
      MLOGD("Konami", "Left prssed, %d", konami);
      return true;
    }
    else if ((konami == 5 || konami == 7) && (xy == origin + Point(2, 0) || xy == origin + Point(2, 1)))
    {
      konami++;
      MLOGD("Konami", "Right prssed, %d", konami);
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
            MatrixOS::UserVar::developer_mode = true;
            MatrixOS::SYS::ExecuteAPP("203 Electronics", "REDACTED");
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
      MLOGD("Konami", "Cleared");
      konami = 0;
      return false;
    }
  }
  return false;
}
