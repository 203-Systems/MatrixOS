#include "Companion.h"

void Companion::Setup() {
  canvasLedLayer = MatrixOS::LED::CurrentLayer();

  for(uint8_t i = 0; i < 5; i++)
  {
    if(MatrixOS::HID::RawHID::Send(std::vector<uint8_t>{0x01, 0x01}))
    {
      break;
    }
    MatrixOS::SYS::DelayMs(100);
  } 
}

void Companion::Loop() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  {
    KeyEventHandler(keyEvent.id, &keyEvent.info);
  }

  HIDReportHandler();
}

void Companion::KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo) {
  if (keyID == FUNCTION_KEY)
  {
    if (keyInfo->state == PRESSED)
    {
      ActionMenu();
    }
  }

  Point xy = MatrixOS::KEYPAD::ID2XY(keyID);

  if (xy && xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)
  {
    if (keyInfo->state == PRESSED)
    {
      MatrixOS::HID::RawHID::Send(std::vector<uint8_t>{0x10, (uint8_t)xy.x, (uint8_t)xy.y, 0xFF});
    }
    else if (keyInfo->state == RELEASED)
    {
      MatrixOS::HID::RawHID::Send(std::vector<uint8_t>{0x10, (uint8_t)xy.x, (uint8_t)xy.y, 0x00});
    }
  }
}

void Companion::HIDReportHandler() {
  uint8_t* report;
  uint8_t report_size;

  while (1)
  {
    report_size = MatrixOS::HID::RawHID::Get(&report);
    
    if (report_size == 0)
    {
      return;
    }

    // MLOGE("Companion", "Got report size %d", report_size);
    // for (uint8_t i = 0; i < report_size; i++)
    // {
    //   MatrixOS::USB::CDC::Printf("%02X ", report[i]);
    // }
    // MatrixOS::USB::CDC::Print("\n");

    // else
    // {
    //     break;
    // }
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

void Companion::ActionMenu() {
  MLOGD("Companion", "Enter Action Menu");

  UI actionMenu("Action Menu", Color(0x00FFAA), true);

  UIButton brightnessBtn;
  brightnessBtn.SetName("Brightness");
  brightnessBtn.SetColor(Color(0xFFFFFF));
  brightnessBtn.SetSize(Dimension(2, 2));
  brightnessBtn.OnPress([&]() -> void { MatrixOS::LED::NextBrightness(); });
  brightnessBtn.OnHold([&]() -> void { BrightnessControl().Start(); });
  actionMenu.AddUIComponent(brightnessBtn, Point(3, 3));

  UIButton clearCanvasBtn;
  clearCanvasBtn.SetName("Clear Canvas");
  clearCanvasBtn.SetColor(Color(0x00FF00));
  clearCanvasBtn.SetSize(Dimension(2, 1));
  clearCanvasBtn.OnPress([&]() -> void { MatrixOS::LED::Fill(0, canvasLedLayer); });
  actionMenu.AddUIComponent(clearCanvasBtn, Point(3, 2));

  UIButton rotatRightBtn;
  rotatRightBtn.SetName("Rotate to this side");
  rotatRightBtn.SetColor(Color(0x00FF00));
  rotatRightBtn.SetSize(Dimension(1, 2));
  rotatRightBtn.OnPress([&]() -> void { MatrixOS::SYS::Rotate(RIGHT); });
  actionMenu.AddUIComponent(rotatRightBtn, Point(5, 3));

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

  actionMenu.SetLoopFunc([&]() -> void {  // Keep buffer updated even when action menu is currently open
    HIDReportHandler();
  });

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

  MatrixOS::LED::CopyLayer(canvasLedLayer, 0);

  MatrixOS::HID::RawHID::Send(std::vector<uint8_t>{0x01, 0x00});
  uiOpened = true;
  actionMenu.Start();
  uiOpened = false;
  MatrixOS::HID::RawHID::Send(std::vector<uint8_t>{0x01, 0x01});

  MLOGD("Companion", "Exit Action Menu");
}

void Companion::End() {
  MatrixOS::HID::RawHID::Send(std::vector<uint8_t>{0x01, 0x00});
}