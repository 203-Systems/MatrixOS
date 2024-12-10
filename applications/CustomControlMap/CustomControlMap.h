#pragma once

#include "MatrixOS.h"
#include "UAD.h"
#include "applications/Application.h"
#include "UILayerControl.h"

class CustomControlMap : public Application {
 public:
  static Application_Info info;

  void Setup() override;
  void Loop() override;

  const uint8_t MAX_UAD_LAYER = 16;
  const size_t MAX_UAD_SIZE = 8196; // 8KB
  const uint32_t UAD_NVS_HASH = StaticHash("CustomControlMap-UAD");
  const uint16_t MAX_HID_TRANSFER_SIZE = 8; // 8 bytes

 private:
  UADRuntime uadRT;
  uint8_t* uadData = nullptr;
  size_t uadSize = 0;
  bool menuLock = false;

  void KeyEventHandler(uint16_t keyID, KeyInfo* keyInfo);
  void Reload();
  void ActionMenu();

  void LoadUADfromNVS();
  void SaveUADtoNVS();

  void HIDReportHandler();

  void PrepNewUAD(const uint8_t* report);
  void LoadNewUADPayload(const uint8_t* report);
  void SaveUAD();
  void LoadUAD();
  void BeginUAD();

  void SendDeviceDescriptor();
  void SendUADStatus();
  void SendUADPayload(const uint8_t* report);

  void SendError(uint8_t command, uint8_t error_code);
  void SendAck(uint8_t command, uint8_t ack_code = 0);

  bool SendHID(const vector<uint8_t> &report, uint8_t retry = 5);
};

inline Application_Info CustomControlMap::info = {
    .name = "Custom Control Map",
    .author = "203 Systems",
    .color = Color(0xFFFF00),
    .version = 1,
    .visibility = true,
};


enum HIDCommand { // MSB is 1 for Write/Response, 0 for Read/Request
  DEVICE_DESCRIPTOR = 0x01, // Gets the device descriptor 
  UAD_STATUS = 0x02, // Gets the active UAD info like loaded and size. Sets the UAD info for the new UAD payload (will clear the current UAD)
  UAD_DATA = 0x03, // Gets or sets the UAD data for the given offset
  UAD_SAVE = 0x04, // Save the UAD to NVS
  UAD_LOAD = 0x05, // Load the UAD from NVS
  UAD_BEGIN = 0x06, // Start the UAD Runtime
  ERR = 0xFE, // Error the command, followed by the command (with read or write bit set) and the error code - Always response
  ACK = 0xFF, // Acknowledge the command, followed by the command (with read or write bit set) - Always response
};

const uint8_t HID_RESPONSE = 0x80;

REGISTER_APPLICATION(CustomControlMap);
