// Declear Family specific function
#pragma once
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "hal/gpio_ll.h"
#include "hal/usb_hal.h"
#include "soc/usb_periph.h"

#include "driver/rmt.h"

// #include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_rom_gpio.h"
#include "esp_log.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_adc/adc_oneshot.h"

#include "nvs_flash.h"

#include "esp_private/system_internal.h"

#include "WS2812/WS2812.h"
#include "framework/Color.h"

#define DEVICE_KEYBOARD // Keyboardtemplate

#define FUNCTION_KEY 0  // Keypad Code for main function key
// #define DEVICE_APPLICATIONS
// #define DEVICE_SETTING

#define DEVICE_SAVED_VAR_SCOPE "Device"

namespace Device
{
  // Device Variable
  inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, bluetooth, bool, false);

  void LoadDeviceInfo();
  void LoadVarientInfo();

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
    void InitKeyPad();

    void Start();
    void StartKeyPad();

    // If return true, meaning the scan in intrupted
    bool ScanKeyPad();

    bool NotifyOS(uint16_t keyID, KeyInfo* keyInfo);  // Passthough MatrixOS::KeyPad::NewEvent() result
  }

  namespace NVS
  {
    void Init();
  }
}