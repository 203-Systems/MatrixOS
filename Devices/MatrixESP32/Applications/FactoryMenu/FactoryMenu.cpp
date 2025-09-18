#include "FactoryMenu.h"

void FactoryMenu::Setup(const vector<string>& args) {
  MatrixOS::SYS::Rotate(Direction::UP, true);

  UI factoryMenu("Factory Menu", Color(0xFFFFFF));

  UIButton ledTestBtn;
  ledTestBtn.SetName("LED Test");
  ledTestBtn.SetColor(Color(0xFFFFFF));
  ledTestBtn.OnPress([&]() -> void { LEDTester(); });
  factoryMenu.AddUIComponent(ledTestBtn, Point(0, 0));

  UIButton keypadTestBtn;
  keypadTestBtn.SetName("Keypad Test");
  keypadTestBtn.SetColor(Color(0xFFFFFF));
  keypadTestBtn.OnPress([&]() -> void { KeyPadTester(); });
  factoryMenu.AddUIComponent(keypadTestBtn, Point(1, 0));

  UIButton touchBarTest;
  touchBarTest.SetName("Touch Bar Test");
  touchBarTest.SetColor(Color(0xFFFFFF));
  touchBarTest.OnPress([&]() -> void { TouchBarTester(); });
  factoryMenu.AddUIComponent(touchBarTest, Point(2, 0));

  UIButton burnEfuseBtn;
  burnEfuseBtn.SetName("Burn EFuse");
  burnEfuseBtn.SetColorFunc([&]() -> Color { return esp_efuse_block_is_empty(EFUSE_BLK3) ? Color(0xFF0000) : Color(0x00FF00); });
  burnEfuseBtn.OnPress([&]() -> void { EFuseBurner(); });
  factoryMenu.AddUIComponent(burnEfuseBtn, Point(0, 7));

  UIButton usbConnection;
  usbConnection.SetName("USB Connection");
  usbConnection.SetColorFunc([&]() -> Color { return MatrixOS::USB::Connected() ? Color(0x00FF00) : Color(0xFF0000); });
  usbConnection.OnPress([&]() -> void {});
  factoryMenu.AddUIComponent(usbConnection, Point(7, 7));

  Color deviceColor = Color(0xFFFFFF);
  #ifdef FAMILY_MYSTRIX
  if(Device::deviceInfo.Model[3] == 'S')
  {
    deviceColor = Color(0x00FFFF);
  }
  else if(Device::deviceInfo.Model[3] == 'P')
  {
    deviceColor = Color(0xFF00FF);
  }
  #endif

  UIButton deviceVersionBtn;
  deviceVersionBtn.SetName("Device Version");
  deviceVersionBtn.SetColor(deviceColor);
  deviceVersionBtn.SetSize(Dimension(4, 1));
  deviceVersionBtn.OnPress([&]() -> void {});
  factoryMenu.AddUIComponent(deviceVersionBtn, Point(2, 7));

  factoryMenu.Start();
  Exit();
}