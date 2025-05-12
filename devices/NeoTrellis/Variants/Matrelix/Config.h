// Define Device Specific Macro, Value and private function
#pragma once

#define GRID_TYPE_8x8
#define FAMILY "MATRIXLIX"
#define MODEL "MX1"

#define MULTIPRESS 10  // Key Press will be process at once

#include "Family.h"
#include "framework/SavedVariable.h"

#include "driver/gpio.h"
#include "driver/i2c_master.h"

namespace Device
{
  // Matrix OS required
  inline string name = "Matrelix";
  inline string model = "NT64";

  inline string manufacturer_name = "203 Systems";
  inline string product_name = "Matrelix";
  inline uint16_t usb_vid = 0x0203;
  inline uint16_t usb_pid = 0x1840;  //(Device Class)0001 (Device Code)100001 (Reserved for Device ID (0~63))000000

  inline uint16_t led_count = 64;
  inline uint8_t led_brightness_level[8] = {8, 22, 39, 60, 84, 110, 138, 169};
  #define FINE_LED_BRIGHTNESS
  inline uint8_t led_brightness_fine_level[16] = {8, 16, 26, 38, 50, 64, 80, 96, 112, 130, 149, 169, 189, 209, 232, 255};

  inline vector<LEDPartition> led_partitions = {
      {"Grid", 1.0, 0, 64},
  };

  // Device Specific
  const uint8_t x_size = 8;
  const uint8_t y_size = 8;

  namespace NeoTrellis
  {
    const gpio_num_t neotrellis_i2c_sda = GPIO_NUM_3;
    const gpio_num_t neotrellis_i2c_scl = GPIO_NUM_4;
    const gpio_num_t neotrellis_i2c_pwr = GPIO_NUM_7;

    const uint8_t neotrellis_i2c_addr[4] =
    {
      0x2E, 0x2F, 
      0x30, 0x31 
    };
  }

  namespace KeyPad
  {
    inline bool velocity_sensitivity = false; // TODO remove this later (Currently being used by Mystrix calibration app)
    inline gpio_num_t fn_pin = GPIO_NUM_0;
    inline bool fn_active_low = true;
    inline bool virtual_fn = true; // Press all 4 key in the center to trigger FN
    inline uint16_t keypad_scanrate = 100;

    inline KeyConfig binary_config = {
        .apply_curve = false,
        .low_threshold = 0,
        .high_threshold = 65535,
        .activation_offset = 0,
        .debounce = 0,
    };
  }

  // LED
  namespace LED
  {
    #define MAX_LED_LAYERS 8
    const inline uint16_t fps = 60;  // Depends on the FreeRTOS tick speed
  }
}
