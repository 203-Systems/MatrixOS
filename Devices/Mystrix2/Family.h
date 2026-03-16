// Declear Family specific function
#pragma once

#include "Device.h"

#include "Framework.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

#define ESP_PLATFORM 1

// Family-specific defines
#define GRID_TYPE_8x8
#define FAMILY_MYSTRIX
#define MULTIPRESS 10  // Key Press will be process at once

#define DEVICE_SAVED_VAR_SCOPE "Device"

// Factory configuration
// #define FACTORY_CONFIG //Global switch for using factory config
// #define FACTORY_DEVICE_VERSION 'P' // Pro
// #define FACTORY_DEVICE_VERSION 'U' // Ultra
#ifdef FACTORY_CONFIG
#if FACTORY_DEVICE_VERSION == 'P'
#define FACTORY_DEVICE_MODEL {'M', 'X', '2', 'P'}
#elif FACTORY_DEVICE_VERSION == 'U'
#define FACTORY_DEVICE_MODEL {'M', 'X', '2', 'U'}
#else 
#error "FACTORY_DEVICE_VERSION is not correct"
#endif
#endif

#define FACTORY_DEVICE_REVISION {'P', 'T', '0', '2'}

#define FACTORY_MFG_YEAR 26
#define FACTORY_MFG_MONTH 03

struct DeviceInfo {
  char model[4];
  char revision[4];
  uint8_t production_year;
  uint8_t production_month;
};

namespace Device
{
  inline DeviceInfo device_info;

  // Device Variable
  inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, touchbar_enable, bool, true);
  inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, bluetooth, bool, false);

  inline gpio_num_t i2c_sda_pin;
  inline gpio_num_t i2c_scl_pin;

  namespace LED
  {
    inline gpio_num_t led_pin;
  }

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
    enum KeypadType {BinaryKeypad, FSRKeypad, MPEKeypad};

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

    inline gpio_num_t fn_pin;
    inline bool velocity_sensitivity = true;
    inline KeypadType keypad_type = KeypadType::MPEKeypad;

    inline KeyConfig binary_config = {
        .apply_curve = false,
        .low_threshold = 0,
        .high_threshold = 65535,
        .activation_offset = 0,
        .debounce = 3,
    };

    inline KeyConfig keypad_config = {
        .apply_curve = true,
        .low_threshold = 1536,
        .high_threshold = 32767,
        .activation_offset = 256,
        .debounce = 10,
    };

    inline const uint16_t keypad_scanrate = 240;
    inline const uint16_t touchbar_scanrate = 120;
                                                 
    inline KeyInfo fn_state;
    inline KeyInfo keypad_state[X_SIZE][Y_SIZE];
    inline KeyInfo touchbar_state[16]; // Virtual 16 keys to be backward compatible with Mystrix 1 apps

    namespace FSR
    {
      void Init();
      void Start();
      bool Scan();
    }

    namespace MPE
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
    void Start();
    void Stop();
  }

  namespace Storage
  {
    inline gpio_num_t sd_clk_pin;
    inline gpio_num_t sd_cmd_pin;
    inline gpio_num_t sd_d0_pin;
    inline gpio_num_t sd_d1_pin;
    inline gpio_num_t sd_d2_pin;
    inline gpio_num_t sd_d3_pin;
    inline gpio_num_t sd_det_pin;

    inline bool sd_4bit_mode;
    inline uint32_t sd_freq_khz;

    void Init();
  }
}
