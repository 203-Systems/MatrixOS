#include "FactoryMenu.h"

void FactoryMenu::Setup() {
  MatrixOS::SYS::Rotate(EDirection::UP, true);

  UI factoryMenu("Factory Menu", Color(0xFFFFFF));

  UIButton ledTestBtn("LED Test", Color(0xFFFFFF), [&]() -> void { LEDTester(); });
  factoryMenu.AddUIComponent(ledTestBtn, Point(0, 0));

  UIButton keypadTestBtn("Keypad Test", Color(0xFFFFFF), [&]() -> void { KeyPadTester(); });
  factoryMenu.AddUIComponent(keypadTestBtn, Point(1, 0));

  UIButton touchBarTest("Touch Bar Test", Color(0xFFFFFF), [&]() -> void { TouchBarTester(); });
  factoryMenu.AddUIComponent(touchBarTest, Point(2, 0));

  UIButton keypadSettingBtn("Keypad Settings", Color(0x00FFFF), [&]() -> void { KeyPadSettings(); });
  factoryMenu.AddUIComponent(keypadSettingBtn, Point(7, 0));

  UIButtonWithColorFunc burnEfuseBtn(
      "Burn EFuse", [&]() -> Color { return esp_efuse_block_is_empty(EFUSE_BLK3) ? Color(0xFF0000) : Color(0x00FF00); },
      [&]() -> void { EFuseBurner(); });
  factoryMenu.AddUIComponent(burnEfuseBtn, Point(0, 7));

  UIButtonWithColorFunc usbConnection(
      "USB Connection", [&]() -> Color { return MatrixOS::USB::Connected() ? Color(0x00FF00) : Color(0xFF0000); },
      [&]() -> void {});
  factoryMenu.AddUIComponent(usbConnection, Point(7, 7));

  Color deviceColor = Color(0xFFFFFF);
  if(Device::deviceInfo.Model[3] == 'S')
  {
    deviceColor = Color(0x00FFFF);
  }
  else if(Device::deviceInfo.Model[3] == 'P')
  {
    deviceColor = Color(0xFF00FF);
  }

  UIButtonLarge deviceVersionBtn("Device Version", deviceColor, Dimension(4, 1), [&]() -> void {});
  factoryMenu.AddUIComponent(deviceVersionBtn, Point(2, 7));

  factoryMenu.Start();
  Exit();
}