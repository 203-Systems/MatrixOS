#include "Device.h"
// #include "ulp_keypad.h"
#include "ulp_riscv.h"

// extern const uint8_t ulp_keypad_bin_start[] asm("_binary_ulp_keypad_bin_start");
// extern const uint8_t ulp_keypad_bin_end[] asm("_binary_ulp_keypad_bin_end");

namespace Device::KeyPad::Binary
{
  void Init() {
    gpio_config_t io_conf;

    // Config Input Pins
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = 0;
    for (uint8_t y = 0; y < y_size; y++)
    { io_conf.pin_bit_mask |= (1ULL << keypad_read_pins[y]); }
    gpio_config(&io_conf);
    
    // Config Output Pins
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = 0;
    for (uint8_t x = 0; x < x_size; x++)
    { io_conf.pin_bit_mask |= (1ULL << keypad_write_pins[x]); }
    gpio_config(&io_conf);

    // Set all output pins to low
    for (uint8_t x = 0; x < x_size; x++)
    { gpio_set_level(keypad_write_pins[x], 0); }

    for (uint8_t x = 0; x < x_size; x++)
    {
      for (uint8_t y = 0; y < y_size; y++)
      { keypadState[x][y].setConfig(&keypad_config); }
    }
  }

  void Start()
  {
    // ulp_riscv_load_binary(ulp_keypad_bin_start, (ulp_keypad_bin_end - ulp_keypad_bin_start));
    // ulp_riscv_run();
  }

  // bool Scan()
  // {
  //   // ESP_LOGI("Keypad ULP", "Scaned: %lu", ulp_count);
  //   uint16_t (*result)[8] = (uint16_t(*)[8])&ulp_result;
  //   for(uint8_t y = 0; y < Device::y_size; y ++)
  //   {
  //     for(uint8_t x = 0; x < Device::x_size; x++)
  //     {
  //       Fract16 reading = (result[x][y] > 0) * FRACT16_MAX;
  //       bool updated = keypadState[x][y].update(reading, true);
  //       if (updated)
  //       {
  //         uint16_t keyID = (1 << 12) + (x << 6) + y;
  //         if (NotifyOS(keyID, &keypadState[x][y]))
  //         {           
  //           return true; 
  //         }
  //       }
  //     }
  //   }
  //   return false;
  // }

    bool Scan()
  {
    for(uint8_t x = 0; x < Device::x_size; x++)
    {
      gpio_set_level(keypad_write_pins[x], 1);
      for(uint8_t y = 0; y < Device::y_size; y++)
      {
        Fract16 reading = gpio_get_level(keypad_read_pins[y]) * FRACT16_MAX;
        // MLOGD("Keypad", "%d %d Read: %d", x, y, gpio_get_level(keypad_read_pins[y]));
        bool updated = keypadState[x][y].update(reading, true);
        if (updated)
        {
          uint16_t keyID = (1 << 12) + (x << 6) + y;
          if (NotifyOS(keyID, &keypadState[x][y]))
          {           
            return true; 
          }
        }
      }
      gpio_set_level(keypad_write_pins[x], 0);
    }
    return false;
  }
}