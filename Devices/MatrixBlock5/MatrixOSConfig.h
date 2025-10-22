#pragma once
#include "Framework.h"

// Platform-specific attributes
// IRAM_ATTR is ESP32-specific for placing functions in IRAM
// For STM32, we don't need this attribute, so define it as empty
#define IRAM_ATTR __RAM_FUNC

#define FUNCTION_KEY 0  // Keypad Code for main function key

#define X_SIZE 8
#define Y_SIZE 8

#define OS_SHELL APPID("203 Systems", "Shell")
#define DEFAULT_BOOTANIMATION APPID("203 Systems", "Matrix Boot")

namespace Device
{
  // Matrix OS required
  inline string name = "MatrixBlock5";
  inline string model = "MB5F";

  inline string manufacturer_name = "203 Systems";
  inline string product_name = "MatrixBlock5";
  inline uint16_t usb_vid = 0x0203;
  inline uint16_t usb_pid = 0x1041;  // Different PID from Mystrix

  // MatrixOS required dimensions - defined directly for MatrixBlock5
  inline uint8_t x_size = X_SIZE;
  inline uint8_t y_size = Y_SIZE;

  namespace LED
  {
    #define MAX_LED_LAYERS 8
    const inline uint16_t fps = 120;  // Depends on the FreeRTOS tick speed

    inline uint16_t count = 64;  // Only main grid, no underglow
    inline uint8_t brightness_level[8] = {8, 22, 39, 60, 84, 110, 138, 169};
    #define FINE_LED_BRIGHTNESS
    inline uint8_t brightness_fine_level[16] = {8, 16, 26, 38, 50, 64, 80, 96, 112, 130, 149, 169, 189, 209, 232, 255};

    inline vector<LEDPartition> partitions = {
        {"Grid", 1.0, 0, 64},
    };
  }
}