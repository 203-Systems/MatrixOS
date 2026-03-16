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
// #define FACTORY_DEVICE_VERSION 'S' // Standard
// #define FACTORY_DEVICE_VERSION 'P' // Pro
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
  inline DeviceInfo deviceInfo;

  // Device Variable
  inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, touchbar_enable, bool, true);
  inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, bluetooth, bool, false);

  namespace HWMidi
  {
    inline gpio_num_t tx_gpio = GPIO_NUM_18;
    inline gpio_num_t rx_gpio = GPIO_NUM_NC;
  }

  namespace LED
  {
    inline gpio_num_t led_pin;
    inline bool underglow;
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
    inline bool fn_active_low = true;
    inline bool velocity_sensitivity = false;

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

    inline gpio_num_t keypad_write_pins[X_SIZE];
    inline gpio_num_t keypad_read_pins[Y_SIZE];
    inline adc_channel_t keypad_read_adc_channel[Y_SIZE];

    inline uint16_t keypad_scanrate = 240;

    inline gpio_num_t touchData_Pin;
    inline gpio_num_t touchClock_Pin;

    inline const uint8_t touchbar_size = 16;
    inline const uint16_t touchbar_scanrate = 120;
    inline uint8_t touchbar_map[touchbar_size];  // Touch number as index and touch location as value (Left touch down
                                                 // and then right touch down)
                                                 
    inline KeyInfo fnState;
    inline KeyInfo keypadState[X_SIZE][Y_SIZE];
    inline KeyInfo touchbarState[touchbar_size];

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
    void Start();
    void Stop();
  }

  namespace HWMidi
  {
    void Init();
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