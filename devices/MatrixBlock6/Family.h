// Declear Family specific function
#pragma once

#include "esp_log.h"

#define FUNCTION_KEY 0  // Keypad Code for main function key
#define DEVICE_SETTING

#define DEVICE_SAVED_VAR_SCOPE "Device"

#ifdef FACTORY_CONFIG
#if FACTORY_DEVICE_VERSION == 'S'
#define FACTORY_DEVICE_MODEL {'M', 'X', '1', 'S'}
#elif FACTORY_DEVICE_VERSION == 'P'
#define FACTORY_DEVICE_MODEL {'M', 'X', '1', 'P'}
#else 
#error "FACTORY_DEVICE_VERSION is not correct"
#endif
#endif

#define FACTORY_DEVICE_REVISION {'R', 'E', 'V', 'C'}

#define FACTORY_MFG_YEAR 23
#define FACTORY_MFG_MONTH 03

struct DeviceInfo {
  char Model[4];
  char Revision[4];
  uint8_t ProductionYear;
  uint8_t ProductionMonth;
};

namespace Device
{
  // Device Variable
  inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, touchbar_enable, bool, true);
  inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, bluetooth, bool, false);

  void LoadDeviceInfo();
  void LoadVariantInfo();

  namespace USB
  {
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
    void InitFN();
    void InitKeyPad();
    void InitTouchBar();

    void Start();
    void StartKeyPad();
    void StartTouchBar();

    // If return true, meaning the scan in interrupted
    void Scan();
    bool ScanKeyPad();
    bool ScanFN();
    bool ScanTouchBar();

    namespace Binary
    {
      void Init();
      void Start();
      bool Scan();
    }

    namespace FSR
    {
      void Init();
      void Start();
      bool Scan();
    }

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

  namespace HWMidi
  {
    void Init();
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