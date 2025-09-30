#include "Device.h"
// #include "ulp_keypad.h"
#include "ulp_riscv.h"
#include "rom/ets_sys.h"

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
    for (uint8_t y = 0; y < Y_SIZE; y++)
    { io_conf.pin_bit_mask |= (1ULL << keypad_read_pins[y]); }
    gpio_config(&io_conf);
    
    // Config Output Pins
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = 0;
    for (uint8_t x = 0; x < X_SIZE; x++)
    { io_conf.pin_bit_mask |= (1ULL << keypad_write_pins[x]); }
    gpio_config(&io_conf);

    // Set all output pins to low
    for (uint8_t x = 0; x < X_SIZE; x++)
    { gpio_set_level(keypad_write_pins[x], 0); }
  }

  void Start()
  {
    // ulp_riscv_load_binary(ulp_keypad_bin_start, (ulp_keypad_bin_end - ulp_keypad_bin_start));
    // ulp_riscv_run();
  }

  IRAM_ATTR bool Scan()
  {
    for(uint8_t x = 0; x < X_SIZE; x++)
    {
      gpio_set_level(keypad_write_pins[x], 1);
      for(uint8_t y = 0; y < Y_SIZE; y++)
      {
        Fract16 reading = gpio_get_level(keypad_read_pins[y]) * FRACT16_MAX;
        // MLOGD("Keypad", "%d %d Read: %d", x, y, gpio_get_level(keypad_read_pins[y]));
        bool updated = keypadState[x][y].Update(binary_config, reading);
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
      ets_delay_us(10);
    }
    return false;
  }
}
