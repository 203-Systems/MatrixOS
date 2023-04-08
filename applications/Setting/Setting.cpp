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
  UIButtonLarge brightnessBtn(
      "Brightness", Color(0xFFFFFF), Dimension(2, 2), [&]() -> void { MatrixOS::SYS::NextBrightness(); },
      [&]() -> void { BrightnessControl().Start(); });
  AddUIComponent(brightnessBtn, Point(3, 3));

  // Rotation control and canvas
  UIButtonLarge nothingBtn("This does nothing", Color(0x00FF00), Dimension(2, 1), []() -> void {});
  AddUIComponent(nothingBtn, origin + Point(0, -1));

  UIButtonLarge rotatRightBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2),
                              [&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  AddUIComponent(rotatRightBtn, origin + Point(2, 0));

  UIButtonLarge rotateDownBtn("Rotate to this side", Color(0x00FF00), Dimension(2, 1),
                              [&]() -> void { MatrixOS::SYS::Rotate(DOWN); });
  AddUIComponent(rotateDownBtn, origin + Point(0, 2));

  UIButtonLarge rotateLeftBtn("Rotate to this side", Color(0x00FF00), Dimension(1, 2),
                              [&]() -> void { MatrixOS::SYS::Rotate(LEFT); });
  AddUIComponent(rotateLeftBtn, origin + Point(-1, 0));

  // Device Control
  UIButton deviceIdBtn("Device ID", Color(0x00FFFF), []() -> void {
    MatrixOS::UserVar::device_id =
        MatrixOS::UIInterface::NumberSelector8x8(MatrixOS::UserVar::device_id, 0x00FFFF, "Device ID", 0, 255);
  });  // TODO This forces 8x8
  AddUIComponent(deviceIdBtn, Point(Device::x_size - 1, Device::y_size - 1));

  UIButton enterDfuBtn("Enter DFU Mode", Color(0xFF0000), []() -> void { MatrixOS::SYS::Bootloader(); });
  AddUIComponent(enterDfuBtn, Point(0, Device::y_size - 1));

  // UIButton clearConfigBtn("Clear Device Config", Color(0xFF00FF), []() -> void {})
  // AddUIComponent(clearConfigBtn, Point(0, Device::y_size - 2));

  // Infomation
  UIButton osVersionBtn("Matrix OS Version", Color(0x00FF30), []() -> void {
    MatrixOS::UIInterface::TextScroll("Matrix OS " MATRIXOS_VERSION_STRING, Color(0x00FFFF));
  });
  AddUIComponent(osVersionBtn, Point(1, Device::y_size - 1));

  UIButton deviceNameBtn("Device Name", Color(0x00FF30),
                         []() -> void { MatrixOS::UIInterface::TextScroll(Device::name, Color(0x00FFFF)); });
  AddUIComponent(deviceNameBtn, Point(2, Device::y_size - 1));

  UIButton deviceSerialBtn("Device Serial", Color(0x00FF30),
                           []() -> void { MatrixOS::UIInterface::TextScroll(Device::GetSerial(), Color(0x00FFFF)); });
  AddUIComponent(deviceSerialBtn, Point(3, Device::y_size - 1));

#ifdef DEVICE_SETTING
#include "DeviceSetting.h"
#endif

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

        UIButtonLarge aBtn("A", Color(0xFF0000), Dimension(2, 2), [&]() -> void {
          if (konami == 9)
            MatrixOS::SYS::ExecuteAPP("203 Electronics", "REDACTED");
          else
            ab.Exit();
        });
        ab.AddUIComponent(aBtn, origin + Point(-2, 0));

        UIButtonLarge bBtn("B", Color(0xFF0000), Dimension(2, 2), [&]() -> void {
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
