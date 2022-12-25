// Define Device Keypad Function
#include "Device.h"
#include "timers.h"

#define VELOCITY_SENSITIVE_KEYPAD_ADC_ATTEN ADC_ATTEN_DB_0
#define VELOCITY_SENSITIVE_KEYPAD_ADC_WIDTH ADC_BITWIDTH_12

namespace Device::KeyPad
{
  adc_oneshot_unit_handle_t adc_handle;
  StaticTimer_t keypad_timer_def;
  TimerHandle_t keypad_timer;

  void Init() {
    LoadCustomSettings();
    InitKeyPad();
    InitTouchBar();
  }

  void Scan() {
    if (ScanFN())
      return;
    if (ScanKeyPad())
      return;
  }

  void InitKeyPad() {
    gpio_config_t io_conf;

    // Config FN
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pin_bit_mask = (1ULL << fn_pin);
#ifdef fn_pin_ACTIVE_HIGH  // Active HIGH
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
#else  // Active Low
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
#endif
    gpio_config(&io_conf);

    // Config Input Pins
    if (!keypad_config.velocity_sensitive)  // Non velocity sensitive keypad
    {
      io_conf.intr_type = GPIO_INTR_DISABLE;
      io_conf.mode = GPIO_MODE_INPUT;
      io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
      io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
      io_conf.pin_bit_mask = 0;
      for (uint8_t y = 0; y < y_size; y++)
      {
        io_conf.pin_bit_mask |= (1ULL << keypad_read_pins[y]);
      }
      gpio_config(&io_conf);
    }
    else  // Velocity sensitive keypad
    {
      adc_oneshot_unit_init_cfg_t init_config = {
          .unit_id = ADC_UNIT_1,
      };
      adc_oneshot_new_unit(&init_config, &adc_handle);

      adc_oneshot_chan_cfg_t adc_config = {
        .atten = VELOCITY_SENSITIVE_KEYPAD_ADC_ATTEN,
        .bitwidth = VELOCITY_SENSITIVE_KEYPAD_ADC_WIDTH,
      };
    
      for (uint8_t y = 0; y < y_size; y++)
      { adc_oneshot_config_channel(adc_handle, keypad_read_adc_channel[y], &adc_config); }
    }

    // Config Output Pins
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = 0;
    for (uint8_t x = 0; x < x_size; x++)
    {
      io_conf.pin_bit_mask |= (1ULL << keypad_write_pins[x]);
    }
    gpio_config(&io_conf);

    // Set up Matrix OS key config
    fnState.setConfig(&fn_config);

    for (uint8_t x = 0; x < x_size; x++)
    {
      for (uint8_t y = 0; y < y_size; y++)
      { keypadState[x][y].setConfig(&keypad_config); }
    }
  }

  void StartKeyPad() {
    keypad_timer = xTimerCreateStatic(NULL, configTICK_RATE_HZ / Device::keypad_scanrate, true, NULL,
                                      reinterpret_cast<TimerCallbackFunction_t>(Scan), &keypad_timer_def);
    xTimerStart(keypad_timer, 0);
  }

  void Start() {
    StartKeyPad();
    StartTouchBar();
  }

  void Clear() {
    fnState.Clear();

    for (uint8_t x = 0; x < x_size; x++)
    {
      for (uint8_t y = 0; y < y_size; y++)
      { keypadState[x][y].Clear(); }
    }

    for (uint8_t i = 0; i < touchbar_size; i++)
    { touchbarState[i].Clear(); }
  }

  KeyInfo* GetKey(uint16_t keyID) {
    uint8_t keyClass = keyID >> 12;
    switch (keyClass)
    {
      case 0:  // System
      {
        uint16_t index = keyID & (0b0000111111111111);
        switch (index)
        {
          case 0:
            return &fnState;
        }
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
      case 2:  // Touch Bar
      {
        uint16_t index = keyID & (0b0000111111111111);
        // MatrixOS::Logging::LogDebug("Keypad", "Read Touch %d", index);
        if (index < touchbar_size)
          return &touchbarState[index];
        break;
      }
    }
    return nullptr;  // Return an empty KeyInfo
  }

  bool ScanFN() {
    Fract16 read = gpio_get_level(fn_pin) * UINT16_MAX;
    // ESP_LOGI("FN", "%d", gpio_get_level(fn_pin));
    if (fn_active_low)
    { read = UINT16_MAX - (uint16_t)read; }
    if (fnState.update(read, false))
    {
      if (NotifyOS(0, &fnState))
      { return true; }
    }
    return false;
  }

  bool ScanKeyPad() {
    // int64_t time =  esp_timer_get_time();
    Fract16 read = 0;
    for (uint8_t y = 0; y < Device::y_size; y++)
    {
      for (uint8_t x = 0; x < Device::x_size; x++)
      {
        gpio_set_level(keypad_write_pins[x], 1);  // Just more stable to turn off the pin and turn back on for each read
        if (!keypad_config.velocity_sensitive)  // Non velocity sensitive keypad
        { read = gpio_get_level(keypad_read_pins[y]) * UINT16_MAX; }
        else  // Velocity sensitive keypad
        {
          int raw_voltage;
          adc_oneshot_read(adc_handle, keypad_read_adc_channel[y], &raw_voltage);
          read = (raw_voltage << 4) + (raw_voltage >> 8);  // Raw Voltage mapped. Will add calibration curve later.
        }
        gpio_set_level(keypad_write_pins[x], 0);  // Set pin back to low
        bool updated = keypadState[x][y].update(read, true);
        if (updated)
        {
          uint16_t keyID = (1 << 12) + (x << 6) + y;
          if (NotifyOS(keyID, &keypadState[x][y]))
          { return true; }
        }
      }
      // volatile int i; for(i=0; i<5; ++i) {} //Add small delay
    }
    // int64_t time_taken =  esp_timer_get_time() - time;
    // ESP_LOGI("Keypad", "%d μs passed, %.2f", (int32_t)time_taken, 1000000.0 / time_taken);
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
    // MatrixOS::Logging::LogError("Keypad", "Failed XY2ID %d %d", xy.x, xy.y);
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
        if (x < Device::x_size && y < Device::y_size)
          return Point(x, y);
        break;
      }
      case 2:  // TouchBar
      {
        uint16_t index = keyID & (0b0000111111111111);
        if (index < Device::touchbar_size)
        {
          if (index / 8)  // Right
          { return Point(Device::x_size, index % 8); }
          else  // Left
          { return Point(-1, index % 8); }
        }
        break;
      }
    }
    return Point(INT16_MIN, INT16_MIN);
  }
}