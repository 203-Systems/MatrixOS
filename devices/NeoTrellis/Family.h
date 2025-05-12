// Declear Family specific function
#pragma once

#include "esp_log.h"
#include "driver/i2c_master.h"

#define FUNCTION_KEY 0  // Keypad Code for main function key
#define DEVICE_SETTING

#define DEVICE_SAVED_VAR_SCOPE "Device"

namespace Device
{
  // Device Variable
  inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, bluetooth, bool, false);

  void LoadDeviceInfo();
  void LoadVariantInfo();

  namespace USB
  {
    void Init();
  }

  namespace NeoTrellis
  {
    inline i2c_master_bus_handle_t neotrellis_i2c_bus = NULL;
    inline i2c_master_dev_handle_t neotrellis_i2c_dev[4] = {NULL, NULL, NULL, NULL};
    inline SemaphoreHandle_t neotrellis_i2c_semaphore = xSemaphoreCreateMutex();

    void Init();
  }

  namespace LED
  {
    void Init();
    void Start();
  }

  namespace KeyPad
  {
    void Init();
    void Start();
    void Scan();
    bool NotifyOS(uint16_t keyID, KeyInfo* keyInfo);  // Passthrough MatrixOS::KeyPad::NewEvent() result
  }

  namespace NVS
  {
    void Init();
  }

  namespace WIFI
  {
    void Init();
  }

  namespace BLEMIDI
  {
    extern bool started;
    void Init(string name);
    void Toggle();
    void Start();
    void Stop();
    bool SendMidi(uint8_t* packet);
    uint32_t MidiAvailable();
    MidiPacket GetMidi();
  }

  namespace ESPNOW
  {
    extern bool started;
    void Init();
    void Flush(void* param);
    bool SendMidi(uint8_t* packet);
    uint32_t MidiAvailable();
    MidiPacket GetMidi();
    void BroadcastMac();
    void UpdatePeer(const uint8_t* new_mac);
  }
}