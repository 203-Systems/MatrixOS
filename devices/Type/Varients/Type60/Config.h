// Define Device Specific Macro, Value and private function
#pragma once

#define GRID_8x8
#define MODEL MX1P

#define DEVICE_BATTERY

#define MULTIPRESS 10  // Key Press will be process at once
// #define LC8812

#include "Family.h"
#include "framework/SavedVariable.h"

#define FACTORY_CONFIG EVT1
#define FACTORY_MFG_YEAR 22
#define FACTORY_MFG_MONTH 04

struct DeviceInfo {
  char DeviceCode[4];
  char Revision[4];
  uint8_t ProductionYear;
  uint8_t ProductionMonth;
};

namespace Device
{
  inline DeviceInfo deviceInfo;
  const string name = "Type 60";
  const string model = "TYPE60";

  const string manufaturer_name = "203 Electronics";
  const string product_name = "Type 60";
  const uint16_t usb_vid = 0x0203;
  const uint16_t usb_pid = 0x2040;  //(Device Class)0001 (Device Code)000001 (Reserved for Device ID (0~63))000000

  const uint16_t numsOfLED = 79 + 40;
  inline uint16_t keypad_scanrate = 480;
  const uint8_t x_size = 15;
  const uint8_t y_size = 5;

  namespace KeyPad
  {
    inline KeyConfig keypad_config = {
        .velocity_sensitive = false,
        .low_threshold = 0,
        .high_threshold = 65535,
        .debounce = 0,
    };

    const inline uint8_t write_size = 7;
    const inline uint8_t read_size = 10;

    inline KeyInfo keypadState[write_size][read_size];

    inline gpio_num_t keypad_write_pins[write_size];
    inline gpio_num_t keypad_read_pins[read_size];
  }

// LED
#define MAX_LED_LAYERS 5
  inline gpio_num_t led_pin;
  inline gpio_num_t led_en_pin;
  inline uint16_t fps = 120;  // Depends on the FreeRTOS tick speed
  inline uint8_t brightness_level[8] = {8, 12, 24, 40, 64, 90, 120, 142};
#define FINE_LED_BRIGHTNESS
  inline uint8_t fine_brightness_level[16] = {4, 8, 14, 20, 28, 38, 50, 64, 80, 98, 120, 142, 168, 198, 232, 255};
  inline uint8_t led_chunk_count = 2;
  inline ws2812_chunk led_chunk[2] = {{79, Color(0xFFFFFF), 1.0}, {40, Color(0xFFFFFF), 4}};

  // Load Device config
  void LoadEVT1();
}
