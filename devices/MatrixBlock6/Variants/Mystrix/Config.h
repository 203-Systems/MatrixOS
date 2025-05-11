// Define Device Specific Macro, Value and private function
#pragma once

#define GRID_TYPE_8x8
#define FAMILY_MYSTRIX


#define DEVICE_BATTERY

#define MULTIPRESS 10  // Key Press will be process at once
// #define LC8812

#include "Family.h"
#include "framework/SavedVariable.h"

#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

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

  // Matrix OS required
  inline string name = "Mystrix";
  inline string model = "MX1S";

  inline string manufacturer_name = "203 Systems";
  inline string product_name = "Mystrix";
  inline uint16_t usb_vid = 0x0203;
  inline uint16_t usb_pid = 0x1040;  //(Device Class)0001 (Device Code)000001 (Reserved for Device ID (0~63))000000

  inline uint16_t led_count = 64 + 32;
  inline uint8_t led_brightness_level[8] = {8, 22, 39, 60, 84, 110, 138, 169};
  #define FINE_LED_BRIGHTNESS
  inline uint8_t led_brightness_fine_level[16] = {8, 16, 26, 38, 50, 64, 80, 96, 112, 130, 149, 169, 189, 209, 232, 255};

  inline vector<LEDPartition> led_partitions = {
      {"Grid", 1.0, 0, 64},
      {"Underglow", 4.0, 64, 32},
  };

  // Device Specific
  const uint8_t x_size = 8;
  const uint8_t y_size = 8;
  namespace KeyPad
  {
    inline uint16_t keypad_scanrate = 480;
    inline uint16_t touchbar_scanrate = 60;
    const uint8_t touchbar_size = 16;

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
  }

  namespace HWMidi
  {
    inline gpio_num_t tx_gpio = GPIO_NUM_18;
    inline gpio_num_t rx_gpio = GPIO_NUM_NC;
  }

  namespace LED
  {
    #define MAX_LED_LAYERS 8
    inline gpio_num_t led_pin;
    const inline uint16_t fps = 120;  // Depends on the FreeRTOS tick speed
  }

  // Load Device config
  void LoadV100();
  void LoadV110();
  void LoadRevC();
  void LoadKeypadSetting();
}
