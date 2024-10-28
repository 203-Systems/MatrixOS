#include "CustomControlMap.h"
#include "sample.h"
#include "ui/UI.h"
#include "applications/BrightnessControl/BrightnessControl.h"

void CustomControlMap::Setup() {
  if(!uad.LoadUAD((uint8_t*)sample_uad, sizeof(sample_uad)))
  {
    MLOGE("CustomControlMap", "Failed to load UAD");
  }
}

void CustomControlMap::Loop() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEventHandler(keyEvent.id, &keyEvent.info); }

  HIDReportHandler();
}

void CustomControlMap::HIDReportHandler() {
  uint8_t* report;
  uint8_t report_size;

  while (1)
  {
    report_size = MatrixOS::HID::RawHID::Get(&report);
    
    if (report_size == 0)
    {
      return;
    }

    if (report[0] == 0x01)  // Key
    {
      MatrixOS::HID::RawHID::Send(std::vector<uint8_t>{0x01, (uint8_t)(uiOpened ? 0x00 : 0x01)});
    }
    if (report[0] == 0x20)  // LED
    {
      MatrixOS::LED::SetColor(Point(report[1], report[2]), Color(report[3], report[4], report[5]), uiOpened ? canvasLedLayer : 0);
    }
    else if (report[0] == 0x21)  // Clear LED
    {
      MatrixOS::LED::Fill(0, uiOpened ? canvasLedLayer : 0);
    }
    else if (report[0] == 0x30)  // Update Brightness
    {
      // MatrixOS::LED::SetBrightness(report[1] * 2.55);
    }
  }
}

void CustomControlMap::KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo) {
  // Reserve Function Key 
  if (keyID == FUNCTION_KEY && keyInfo->state == (menuLock ? HOLD : PRESSED))
  {
    ActionMenu(); 
  }

  if(keyInfo->state == KeyState::AFTERTOUCH) { return; } // Ignore Aftertouch for now for easy of debug. Remove later
  uad.KeyEvent(keyID, keyInfo);
}

void CustomControlMap::Reload()
{
  MatrixOS::SYS::ExecuteAPP(info.author, info.name); // Just relaunch the APP lol
}

void CustomControlMap::ActionMenu() {
  MatrixOS::LED::CopyLayer(0, 1);
  MLOGD("CustomControlMap", "Enter Action Menu");

  UI actionMenu("Action Menu", Color(0x00FFAA), true);

  UIButton brightnessBtn;
  brightnessBtn.SetName("Brightness");
  brightnessBtn.SetColor(Color(0xFFFFFF));
  brightnessBtn.SetSize(Dimension(2, 2));
  brightnessBtn.OnPress([&]() -> void { MatrixOS::LED::NextBrightness(); });
  brightnessBtn.OnHold([&]() -> void { BrightnessControl().Start(); });
  actionMenu.AddUIComponent(brightnessBtn, Point(3, 3));

  UIButton reloadBtn;
  reloadBtn.SetName("Reload");
  reloadBtn.SetColor(Color(0x00FF00));
  reloadBtn.SetSize(Dimension(2, 1));
  reloadBtn.OnPress([&]() -> void { Reload(); });
  actionMenu.AddUIComponent(reloadBtn, Point(3, 2));

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

  UIButton menuLockBtn;
  menuLockBtn.SetName("Touch Alt Key");
  menuLockBtn.SetColorFunc([&]() -> Color { return Color(0xA0FF00).DimIfNot(menuLock); });
  menuLockBtn.OnPress([&]() -> void { menuLock = !menuLock; });
  actionMenu.AddUIComponent(menuLockBtn, Point(0, 5));

  UILayerControl layerControl("Activated Layers", Color(0x00FFFF), Dimension(8, 2), &uad, UAD::LayerInfoType::ACTIVE);
  actionMenu.AddUIComponent(layerControl, Point(0, 0));

  UILayerControl passthroughControl("Layer Passthrough", Color(0xFF00FF), Dimension(8, 2), &uad, UAD::LayerInfoType::PASSTHROUGH);                       
  actionMenu.AddUIComponent(passthroughControl, Point(0, 6));                                        

  actionMenu.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool { 
    if(keyEvent->id == FUNCTION_KEY)
    {
      if(keyEvent->info.state == HOLD)
      { Exit(); }
      else if(keyEvent->info.state == RELEASED)
      { actionMenu.Exit(); }
      return true; //Block UI from to do anything with FN, basically this function control the life cycle of the UI
    }
    return false;
   });

  actionMenu.Start();
  MatrixOS::LED::CopyLayer(1, 0);

  uad.InitializeLayer(); // Reinitialize layer after exit action menu so layer led update correctly

  MLOGD("CustomControlMap", "Exit Action Menu");
}
