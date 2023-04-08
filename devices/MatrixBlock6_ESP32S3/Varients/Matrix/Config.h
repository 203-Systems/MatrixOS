// Define Device Specific Macro, Value and private function
#pragma once

#define GRID_8x8
#define FAMILY MATRIX
#define MODEL MX1

#define DEVICE_BATTERY

#define MULTIPRESS 10  // Key Press will be process at once
// #define LC8812

#include "Family.h"

#include "framework/SavedVariable.h"

// #define FACTORY_CONFIG //Global switch for using factory config

#define FACTORY_DEVICE_VERSION 'S' // Standard
// #define FACTORY_DEVICE_VERSION 'P' // Pro

#if FACTORY_DEVICE_VERSION == 'S'
#define FACTORY_DEVICE_MODEL {'M', 'X', '1', 'S'}
#elif FACTORY_DEVICE_VERSION == 'P'
#define FACTORY_DEVICE_MODEL {'M', 'X', '1', 'P'}
#else 
#error "FACTORY_DEVICE_VERSION is not correct"
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
  inline string name = "Matrix";
  inline string model = "MX1S";

  const string manufaturer_name = "203 Electronics";
  const string product_name = "Matrix";
  const uint16_t usb_vid = 0x0203;
  const uint16_t usb_pid = 0x1040;  //(Device Class)0001 (Device Code)000001 (Reserved for Device ID (0~63))000000

  const uint16_t numsOfLED = 64 + 32;
  inline uint16_t keypad_scanrate = 480;
  inline uint16_t touchbar_scanrate = 60;
  const uint8_t x_size = 8;
  const uint8_t y_size = 8;
  const uint8_t touchbar_size = 16;  // Not required by the API, private use.

  namespace KeyPad
  {
    inline gpio_num_t fn_pin;
    inline bool fn_active_low = true;
    inline bool velocity_sensitivity = false;

    inline KeyConfig fn_config = {
        .velocity_sensitive = false,
        .low_threshold = 0,
        .high_threshold = 65535,
        .debounce = 0,
    };

    inline KeyConfig keypad_config = {
        .velocity_sensitive = true,
        .low_threshold = 512,
        .high_threshold = 65535,
        .debounce = 5,
    };

    inline KeyConfig touch_config = {
        .velocity_sensitive = false,
        .low_threshold = 0,
        .high_threshold = 65535,
        .debounce = 0,
    };

    inline gpio_num_t keypad_write_pins[8];
    inline gpio_num_t keypad_read_pins[8];
    inline adc_channel_t keypad_read_adc_channel[8];

    inline gpio_num_t touchData_Pin;
    inline gpio_num_t touchClock_Pin;
    inline uint8_t touchbar_map[touchbar_size];  // Touch number as index and touch location as value (Left touch down
                                                 // and then right touch down)

    inline KeyInfo fnState;
    inline KeyInfo keypadState[x_size][y_size];
    inline KeyInfo touchbarState[touchbar_size];

    inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, keypad_custom_setting, bool, false);
    inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, keypad_velocity_sensitive, bool, KeyPad::keypad_config.velocity_sensitive);
    inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, keypad_low_threshold, Fract16, KeyPad::keypad_config.low_threshold);
    inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, keypad_high_threshold, Fract16, KeyPad::keypad_config.high_threshold);
    inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, keypad_debounce, uint16_t, KeyPad::keypad_config.debounce);

    inline void LoadCustomSettings() {
      return;
      if (keypad_custom_setting)
      {
        if (keypad_low_threshold == 0)  // Can't be lower than 1
        { keypad_low_threshold = 1; }
        keypad_config.velocity_sensitive = keypad_velocity_sensitive;
        keypad_config.low_threshold = keypad_low_threshold;
        keypad_config.high_threshold = keypad_high_threshold;
        keypad_config.debounce = keypad_debounce;
      }
      else
      {
        keypad_velocity_sensitive = keypad_config.velocity_sensitive;
        keypad_low_threshold = keypad_config.low_threshold;
        keypad_high_threshold = keypad_config.high_threshold;
        keypad_debounce = keypad_config.debounce;
      }
    }
  }

  namespace HWMidi
  {
    inline gpio_num_t tx_gpio = GPIO_NUM_18;
    inline gpio_num_t rx_gpio = GPIO_NUM_NC;
  }

// LED
#define MAX_LED_LAYERS 5
  inline gpio_num_t led_pin;
  inline uint16_t fps = 120;  // Depends on the FreeRTOS tick speed
  inline uint8_t brightness_level[8] = {8, 12, 24, 40, 64, 90, 120, 142};
#define FINE_LED_BRIGHTNESS
  inline uint8_t fine_brightness_level[16] = {4, 8, 14, 20, 28, 38, 50, 64, 80, 98, 120, 142, 168, 198, 232, 255};
  inline uint8_t led_chunk_count = 2;
  inline ws2812_chunk led_chunk[2] = {{64, Color(0xFFFFFF), 1.0}, {32, Color(0xFFFFFF), 4}};
  // const Dimension grid_size(8,8);
  // const Point grid_offset = Point(1,1);

  // Load Device config
  void LoadV100();
  void LoadV110();
  void LoadRevC();
  void LoadKeypadSetting();
}
