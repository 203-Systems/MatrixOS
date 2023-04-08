// Define Device Keypad Function
#include "Device.h"
#include "timers.h"

#include "ulp_riscv.h"
#include "ulp_keypad.h"

#include "esp_private/esp_sleep_internal.h"
#include "esp_private/adc_share_hw_ctrl.h"


extern const uint8_t ulp_keypad_bin_start[] asm("_binary_ulp_keypad_bin_start");
extern const uint8_t ulp_keypad_bin_end[] asm("_binary_ulp_keypad_bin_end");

namespace Device::KeyPad
{
  adc_oneshot_unit_handle_t adc_handle;
  StaticTimer_t keypad_timer_def;
  TimerHandle_t keypad_timer;

  void Init() {
    InitKeyPad();
  }

  void Scan() {
    ScanKeyPad();
  }

  void InitKeyPad() {
    gpio_config_t io_conf;

    // Config Input Pins

      io_conf.intr_type = GPIO_INTR_DISABLE;
      io_conf.mode = GPIO_MODE_INPUT;
      io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
      io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
      io_conf.pin_bit_mask = 0;
      for (uint8_t read_id = 0; read_id < read_size; read_id++)
      { io_conf.pin_bit_mask |= (1ULL << keypad_read_pins[read_id]); }
      gpio_config(&io_conf);

    // Config Output Pins
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = 0;
    for (uint8_t write_id = 0; write_id < write_size; write_id++)
    { io_conf.pin_bit_mask |= (1ULL << keypad_write_pins[write_id]); }
    gpio_config(&io_conf);

    for (uint8_t write_id = 0; write_id < write_size; write_id++)
    {
      for (uint8_t read_id = 0; read_id < read_size; read_id++)
      { keypadState[write_id][read_id].setConfig(&keypad_config); }
    }
  }

  void StartKeyPad() {
    ulp_riscv_load_binary(ulp_keypad_bin_start, (ulp_keypad_bin_end - ulp_keypad_bin_start));
    ulp_riscv_run();

    keypad_timer = xTimerCreateStatic(NULL, configTICK_RATE_HZ / Device::keypad_scanrate, true, NULL, reinterpret_cast<TimerCallbackFunction_t>(Scan), &keypad_timer_def);
    xTimerStart(keypad_timer, 0);
  }

  void Start() {
    StartKeyPad();
  }

  void Clear() {
    for (uint8_t write_id = 0; write_id < write_size; write_id++)
    {
      for (uint8_t read_id = 0; read_id < read_size; read_id++)
      { keypadState[write_id][read_id].Clear(); }
    }
  }

  KeyInfo* GetKey(uint16_t keyID) {
    uint8_t keyClass = keyID >> 12;
    switch (keyClass)
    {
      case 0:  // System
      {
        // uint16_t index = keyID & (0b0000111111111111);
        // switch (index)
        // {
        //   case 0:
        //     return &fnState;
        // }
        break;
      }
      case 1:  // Main Grid
      {
        int16_t x = (keyID & (0b0000111111000000)) >> 6;
        int16_t y = keyID & (0b0000000000111111);
        if (x < x_size && y < y_size)
          return &keypadState[x][y];
        break;
      }
    }
    return nullptr;  // Return an empty KeyInfo
  }

  Point KeyGridRemap(Point hwPoint) //From hardware keymatrix to os level grid
  {
    Point result = Point(hwPoint.x * 2 + hwPoint.y % 2, hwPoint.y / 2);
    
    // Fix exceptions 
    if(result.x == 13 && result.y == 0) //Remap Backspace L
      result = Point(12, 2);
    else if(result.x == 13 && result.y == 1) //Remap Backspace R
      result = Point(13, 0);
    else if(result.x == 12 && result.y == 2) //Remap Backware slash (Above enter)
      result = Point(13, 1);

    return result;
  }

  bool ScanKeyPad()
  {
    // ESP_LOGI("Keypad ULP", "Scaned: %lu", ulp_count);
    uint8_t (*result)[read_size] = (uint8_t(*)[read_size])&ulp_result;
    for(uint8_t hw_y = 0; hw_y < read_size; hw_y ++)
    {
      for(uint8_t hw_x = 0; hw_x < write_size; hw_x++)
      {
        Fract16 read = result[hw_x][hw_y] * UINT16_MAX;
        Point os_xy = KeyGridRemap(Point(hw_x, hw_y));

        bool updated = keypadState[os_xy.x][os_xy.y].update(read);
        if (updated)
        {
          uint16_t keyID = (1 << 12) + (os_xy.x << 6) + os_xy.y;
          if (NotifyOS(keyID, &keypadState[os_xy.x][os_xy.y]))
          {           
            // printf("\n");
            return true; 
          }
        }
      }
    }
    // printf("\n");
    return false;
  }

  bool NotifyOS(uint16_t keyID, KeyInfo* keyInfo) {
    KeyEvent keyEvent;
    keyEvent.id = keyID;
    keyEvent.info = *keyInfo;
    return MatrixOS::KEYPAD::NewEvent(&keyEvent);
  }

  uint16_t XY2ID(Point xy) {
    if (xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)  // Main grid
    { return (1 << 12) + (xy.x << 6) + xy.y; }
    else if ((xy.x == -1 || xy.x == 8) && (xy.y >= 0 && xy.y < 8))  // Touch Bar
    { return (2 << 12) + xy.y + (xy.x == 8) * 8; }
    // MLOGE("Keypad", "Failed GetKeyID %d %d", xy.x, xy.y);
    return UINT16_MAX;
  }

  // Matrix use the following ID Struct
  // CCCC IIIIIIIIIIII
  // C as class (4 bits), I as index (12 bits). I could be spilted by the class defination, for example, class 0 (grid),
  // it's spilted to XXXXXXX YYYYYYY. Class List: Class 0 - System - IIIIIIIIIIII Class 1 - Grid - XXXXXX YYYYYY Class 2
  // - TouchBar - IIIIIIIIIIII Class 3 - Underglow - IIIIIIIIIIII

  Point ID2XY(uint16_t keyID) {
    uint8_t keyClass = keyID >> 12;
    switch (keyClass)
    {
      case 1:  // Main Grid
      {
        int16_t x = (keyID & 0b0000111111000000) >> 6;
        int16_t y = keyID & (0b0000000000111111);
        if (x < write_size && y < read_size)
          return Point(x, y);
        break;
      }
    }
    return Point(INT16_MIN, INT16_MIN);
  }
}