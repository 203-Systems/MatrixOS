#include "CustomControlMap.h"
#include "UI/UI.h"

void CustomControlMap::Setup(const vector<string>& args) {
  LoadUADfromNVS();
}

void CustomControlMap::Loop() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KeyPad::Get(&keyEvent))
  { KeyEventHandler(keyEvent); }

  HIDReportHandler();
}

void CustomControlMap::LoadUADfromNVS() {
  uadRT.UnloadUAD();
  vPortFree(uadData);

  uadSize = 0;
  
  size_t new_uad_size = MatrixOS::NVS::GetSize(UAD_NVS_HASH);

  MLOGD("CustomControlMap", "NVS UAD Size: %d", new_uad_size);
  if (new_uad_size == -1 || new_uad_size == 0)
  {
    MLOGD("CustomControlMap", "No UAD found in NVS");
    return;
  }

  uadData = (uint8_t*)pvPortMalloc(new_uad_size);
  if (uadData == nullptr)
  {
    MLOGE("CustomControlMap", "Failed to allocate memory for UAD");
    return;
  }

  if (MatrixOS::NVS::GetVariable(UAD_NVS_HASH, uadData, new_uad_size) != 0)
  {
    MLOGE("CustomControlMap", "Failed to read UAD from NVS");
    vPortFree(uadData);
    uadData = nullptr;
    return;
  }

  if (!uadRT.LoadUAD(uadData, new_uad_size))
  {
    MLOGE("CustomControlMap", "Failed to load UAD");
    vPortFree(uadData);
    uadData = nullptr;
    return;
  }

  uadSize = new_uad_size;
}

void CustomControlMap::SaveUADtoNVS() {
  MatrixOS::NVS::SetVariable(UAD_NVS_HASH, uadData, uadSize);
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

    bool write = report[0] & 0x80;
    uint8_t command = report[0] & 0x7F;

    
    MLOGD("CustomControlMap", "HID Report Size: %d - Command: %d - Write: %d", report_size, command, write);

    if (write)
    {
      switch (command)
      {
        case UAD_STATUS: // Get Ready for new UAD
          PrepNewUAD(report);
          break;
        case UAD_DATA: // Load UAD Payload
          LoadNewUADPayload(report);
          break;
        case UAD_SAVE: // Save UAD to NVS
          SaveUAD();  
          break;
        case UAD_LOAD: // Load UAD from NVS
          LoadUAD();
          break;
        case UAD_BEGIN: // Start UAD Runtime
          BeginUAD();
          break;
        default:
          SendError(report[0], 0); // Command not found, return command with error code 0
          break;
      }
    }
    else // Read Command
    {
      switch (command)
      {
        case DEVICE_DESCRIPTOR:
          SendDeviceDescriptor();
          break;
        case UAD_STATUS:
          SendUADStatus();
          break;
        case UAD_DATA:
          SendUADPayload(report);
          break;
        default:
          SendError(report[0], 0); // Command not found, return command with error code 0
          break;
      }
    }
  }
}

void CustomControlMap::PrepNewUAD(const uint8_t* report) {
  uint32_t new_uad_size = (report[2] << 24) | (report[3] << 16) | (report[4] << 8) | report[5];
  MLOGD("CustomControlMap", "Prep New UAD with Size: %d", new_uad_size);

  if (new_uad_size > MAX_UAD_SIZE)
  {
    SendError(report[0], 3); // UAD Size too large
    return;
  }

  uadRT.UnloadUAD();
  MatrixOS::LED::Fill(Color(0), 0);

  uadSize = 0;
  vPortFree(uadData);

  uadData = (uint8_t*)pvPortMalloc(new_uad_size);
  if (uadData == nullptr)
  {
    MatrixOS::SYS::ErrorHandler("Failed to allocate memory for new UAD");
    SendError(report[0], 4); // Failed to allocate memory for new UAD
    return;
  }
  uadSize = new_uad_size;

  SendAck(report[0]);
}

void CustomControlMap::LoadNewUADPayload(const uint8_t* report) {
  if (uadData == nullptr)
  {
    MLOGE("CustomControlMap", "UAD not ready");
    SendError(report[0], 2); // UAD not ready
    return;
  }

  uint16_t section = ((report[1] << 8) | report[2]);
  uint32_t offset = section * MAX_HID_TRANSFER_SIZE;
  uint8_t size = report[3];

  if(size > MAX_HID_TRANSFER_SIZE)
  {
    MLOGE("CustomControlMap", "Size too large - Max is %d but got %d", MAX_HID_TRANSFER_SIZE, size);
    SendError(report[0], 3); // Size too large
    return;
  }

  if(offset + size > uadSize)
  {
    MLOGE("CustomControlMap", "Offset out of range - Section: %d - Offset: %d - Size: %d - UAD Size: %d", section, offset, size, uadSize);
    SendError(report[0], 1); // Offset out of range
    return;
  }

  memcpy(uadData + offset, report + 4, size);

  SendAck(report[0], section & 0xFF);
}

void CustomControlMap::SaveUAD() {
  if (uadData == nullptr)
  {
    SendError(UAD_SAVE, 1); // UAD not ready
    return;
  }

  if(uadSize == 0)
  {
    SendError(UAD_SAVE, 2); // UAD Size is 0
    return;
  }

  SaveUADtoNVS();
  SendAck(UAD_SAVE | HID_RESPONSE);
}

void CustomControlMap::LoadUAD() {
  LoadUADfromNVS();
  SendAck(UAD_LOAD | HID_RESPONSE);
}

void CustomControlMap::BeginUAD() {
  if (uadData == nullptr)
  {
    SendError(UAD_BEGIN, 1); // UAD not ready
    return;
  }

  if(uadSize == 0)
  {
    SendError(UAD_BEGIN, 2); // UAD Size is 0
    return;
  }

  if (!uadRT.LoadUAD(uadData, uadSize))
  {
    SendError(UAD_BEGIN, 3); // Failed to load UAD
    return;
  }

  SendAck(UAD_BEGIN | HID_RESPONSE);
}

void CustomControlMap::SendDeviceDescriptor() {
  MLOGD("CustomControlMap", "Send Device Descriptor");
  vector<uint8_t> payload = {
    DEVICE_DESCRIPTOR | HID_RESPONSE,                    // Response to DEVICE_DESCRIPTOR
    UADRuntime::UAD_MAJOR_VERSION,  // UAD Major Version 0
    UADRuntime::UAD_MINOR_VERSION,  // UAD Minor Version 1
    0x00,                    // Vendor ID 0x00 - Basiclly the same as Sysex header. Change to dynamic linked later
    0x02,                    // Vendor ID 0x02
    0x03,                    // Vendor ID 0x03
    0x4D,                    // Family ID 0x4D
    0x58,                    // Family ID 0x58
    0x11,                    // Model ID 0x11
    Device::x_size,          // Device X 8
    Device::y_size,          // Device Y 8
    MAX_UAD_LAYER,           // Max Layers
    (uint8_t)((MAX_UAD_SIZE >> 24) & 0xFF), // UAD Size MSB1
    (uint8_t)((MAX_UAD_SIZE >> 16) & 0xFF), // UAD Size MSB2
    (uint8_t)((MAX_UAD_SIZE >> 8) & 0xFF),  // UAD Size MSB3
    (uint8_t)((MAX_UAD_SIZE) & 0xFF),       // UAD Size MSB4
    (uint8_t)MAX_HID_TRANSFER_SIZE,   // Max HID Transfer Size
  };

  MLOGD("CustomControlMap", "Device Descriptor Created");

  if(!SendHID(payload))
  {
    MLOGE("CustomControlMap", "Failed to send device descriptor");
  }
}

void CustomControlMap::SendUADStatus() {
  MLOGD("CustomControlMap", "Send UAD Status");
  vector<uint8_t> payload = {
    UAD_STATUS | HID_RESPONSE,             // Response to UAD_STATUS
    (uint8_t)uadRT.loaded,    // UAD Loaded
    (uint8_t)((uadRT.uadSize >> 24) & 0xFF), // UAD Size MSB1
    (uint8_t)((uadRT.uadSize >> 16) & 0xFF), // UAD Size MSB2
    (uint8_t)((uadRT.uadSize >> 8) & 0xFF),  // UAD Size MSB3
    (uint8_t)((uadRT.uadSize) & 0xFF),       // UAD Size MSB4
    uadRT.layerCount,          // Layer Count
    (uint8_t)((uadRT.layerEnabled >> 8) & 0xFF), // Enabled Layers MSB
    (uint8_t)(uadRT.layerEnabled & 0xFF),        // Enabled Layers LSB
    (uint8_t)((uadRT.layerPassthrough >> 8) & 0xFF), // Passthrough Layers MSB
    (uint8_t)(uadRT.layerPassthrough & 0xFF),        // Passthrough Layers LSB
  };

  if(!SendHID(payload))
  {
    MLOGE("CustomControlMap", "Failed to send device descriptor");
  };
}

void CustomControlMap::SendUADPayload(const uint8_t* report) {
  uint16_t section = ((report[1] << 8) | report[2]);
  uint32_t offset = section * MAX_HID_TRANSFER_SIZE;
  uint8_t size = MAX_HID_TRANSFER_SIZE;

  MLOGD("CustomControlMap", "Send UAD Payload #%d - Offset: %d - Size: %d", section + 1, offset, size);

  if(offset >= uadRT.uadSize)
  {
    SendError(report[0], 1); // Offset out of range
    return;
  }

  if(size > MAX_HID_TRANSFER_SIZE)
  {
    SendError(report[0], 2); // Transfer size too large
    return;
  }

  if(offset + size > uadRT.uadSize)
  {
    size = uadRT.uadSize - offset;
  }

  vector<uint8_t> payload = {
    UAD_DATA | HID_RESPONSE, // Response to UAD_DATA
    (uint8_t)((section >> 8) & 0xFF), // Section MSB
    (uint8_t)(section & 0xFF),        // Section LSB
    size,                             // Size
  };

  payload.insert(payload.end(), uadRT.uad + offset, uadRT.uad + offset + size);

  if(!SendHID(payload))
  {
    MLOGE("CustomControlMap", "Failed to send uad payload #%d", offset);
  }
}

void CustomControlMap::SendError(uint8_t command, uint8_t error_code) {
  MLOGD("CustomControlMap", "Send Command Error %d - %d", command, error_code);
  vector<uint8_t> payload = {
    HIDCommand::ERR,          // ERR Command
    command,                 // Command
    error_code,              // Error Code
  };

  if(!SendHID(payload))
  {
    MLOGE("CustomControlMap", "Failed to send device descriptor");
  }
}

void CustomControlMap::SendAck(uint8_t command, uint8_t ack_code) {
  MLOGD("CustomControlMap", "Send Command Ack %d - %d", command, ack_code);
  vector<uint8_t> payload = {
    HIDCommand::ACK,          // ACK Command
    command,                 // Command
    ack_code,              // Ack Code. Not used for now
  };

  if(!SendHID(payload))
  {
    MLOGE("CustomControlMap", "Failed to send device descriptor");
  }
}


bool CustomControlMap::SendHID(const vector<uint8_t> &report, uint8_t retry) {

  if(retry == 0)
  {
    retry = 1;
  }

  for(uint8_t i = 0; i < retry; i++)
  {
    MLOGD("CustomControlMap", "Send HID Report - Retry: %d", i);
    if(MatrixOS::HID::RawHID::Send(report))
    {
      return true;
    }
  }
  return false;
}

void CustomControlMap::KeyEventHandler(KeyEvent& keyEvent) {
  // Reserve Function Key
  if (keyEvent.ID() == FUNCTION_KEY && keyEvent.State() == (menuLock ? HOLD : PRESSED))
  {
    ActionMenu();
  }

  uadRT.KeyEvent(keyEvent.ID(), &keyEvent.info);
}

void CustomControlMap::Reload()
{
  MatrixOS::SYS::ExecuteAPP(info.author, info.name); // Just relaunch the APP for now lol
}

void CustomControlMap::ActionMenu() {
  MatrixOS::LED::CopyLayer(0, 1);
  MLOGD("CustomControlMap", "Enter Action Menu");

  UI actionMenu("Action Menu", Color(0x00FFAA), true);

  UIButton reloadBtn;
  reloadBtn.SetName("Reload");
  reloadBtn.SetColor(Color(0xFF0000));
  reloadBtn.OnPress([&]() -> void { Reload(); });
  actionMenu.AddUIComponent(reloadBtn, Point(0, 5));

  UIButton systemSettingBtn;
  systemSettingBtn.SetName("System Setting");
  systemSettingBtn.SetColor(Color(0xFFFFFF));
  systemSettingBtn.OnPress([&]() -> void { MatrixOS::SYS::OpenSetting(); });
  actionMenu.AddUIComponent(systemSettingBtn, Point(7, 5));

  UIToggle menuLockToggle;
  menuLockToggle.SetName("Menu Lock");
  menuLockToggle.SetColor(Color(0x00FF00));
  menuLockToggle.SetValuePointer(&menuLock);
  menuLockToggle.OnPress([&]() -> void { menuLock.Save(); });
  actionMenu.AddUIComponent(menuLockToggle, Point(0, 2));
  

  UILayerControl layerControl("Activated Layers", Color(0x00FFFF), Dimension(8, 2), &uadRT, UADRuntime::LayerInfoType::ACTIVE);
  actionMenu.AddUIComponent(layerControl, Point(0, 0));

  UILayerControl passthroughControl("Layer Passthrough", Color(0xFF00FF), Dimension(8, 2), &uadRT, UADRuntime::LayerInfoType::PASSTHROUGH);                       
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
  
  if(uadRT.loaded)
  {
    uadRT.InitializeLayer(); // Reinitialize layer after exit action menu so layer led update correctly
  }

  MLOGD("CustomControlMap", "Exit Action Menu");
}
