#include "Companion.h"

void Companion::Setup(const vector<string>& args) {
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
  while (MatrixOS::KeyPad::Get(&keyEvent))
  {
    KeyEventHandler(keyEvent);
  }

  HIDReportHandler();
}

void Companion::KeyEventHandler(KeyEvent& keyEvent) {
  if (keyEvent.ID() == FUNCTION_KEY)
  {
    if (keyEvent.State() == PRESSED)
    {
      ActionMenu();
    }
  }

  Point xy = MatrixOS::KeyPad::ID2XY(keyEvent.ID());

  if (xy && xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)
  {
    if (keyEvent.State() == PRESSED)
    {
      MatrixOS::HID::RawHID::Send(std::vector<uint8_t>{0x10, (uint8_t)xy.x, (uint8_t)xy.y, 0xFF});
    }
    else if (keyEvent.State() == RELEASED)
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