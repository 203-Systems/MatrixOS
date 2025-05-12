// Define Device Keypad Function
#include "Device.h"
#include "timers.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "include/NeoTrellis.h"
#include "rom/ets_sys.h"

namespace Device::KeyPad
{
  StaticTimer_t keypad_timer_def;
  TimerHandle_t keypad_timer;

  uint8_t keypad_reading_cache[8] = {0};

  KeyInfo fnState;
  KeyInfo keypadState[x_size][y_size];

  void InitKeyPad() {
    for (uint8_t i = 0; i < 4; i++)
    {
      bool success = true;
      // Activate Keypad
      for (int k = 0; k < NEO_TRELLIS_NUM_KEYS; k++) {
        
        uint8_t key = NEO_TRELLIS_KEY(k);

        neoTrellisKeyState ks;
        ks.bit.STATE = 1;
        ks.bit.ACTIVE = (1 << SEESAW_KEYPAD_EDGE_FALLING);

        uint8_t cmd[] = { SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_EVENT, key, ks.reg };
        if (i2c_master_transmit(NeoTrellis::neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
          ESP_LOGE("NT-KeyPad", "Failed to register key %d for falling edge for %d", key, i);
          success = false;
        }
        // else
        // {
        //   ESP_LOGI("NT-KeyPad", "Key %d registered for falling edge for %d", key, i);
        // }

        ks.bit.ACTIVE = (1 << SEESAW_KEYPAD_EDGE_RISING);
        cmd[3] = ks.reg;
        if (i2c_master_transmit(NeoTrellis::neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
          ESP_LOGE("NT-KeyPad", "Failed to set key %d for rising edge for %d", key, i);
          success = false;
        }
        // else
        // {
        //   ESP_LOGI("NT-KeyPad", "Key %d registered for rising edge for %d", key, i);
        // }
      }

      if (success)
      {
        ESP_LOGI("NT-LED", "NeoTrellis %d LED initialized", i);
      }
      else
      {
        ESP_LOGE("NT-LED", "NeoTrellis %d LED initialization failed", i);
      }
    }
  }

  void InitFN() {
    gpio_config_t io_conf;

    // Config FN
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pin_bit_mask = (1ULL << fn_pin);
#ifdef FN_PIN_ACTIVE_HIGH  // Active HIGH
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
#else  // Active Low
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
#endif
    gpio_config(&io_conf);
  }

  void Init() {
    InitKeyPad();
    InitFN();
  }

  void Start() {
    keypad_timer = xTimerCreateStatic(NULL, configTICK_RATE_HZ / keypad_scanrate, true, NULL, reinterpret_cast<TimerCallbackFunction_t>(Scan), &keypad_timer_def);
    
    xTimerStart(keypad_timer, 0);
  }

  void Scan() {

    // Scan Keypad
    for (uint8_t i = 0; i < 4; i++)
    {
      if (xSemaphoreTake(NeoTrellis::neotrellis_i2c_semaphore, portMAX_DELAY) == pdTRUE) 
      {
        uint8_t keypad_events = 0;
        {
          uint8_t cmd[] = { SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_COUNT };
          if (i2c_master_transmit(NeoTrellis::neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
            ESP_LOGE("NT-KeyPad", "Failed to request to keypad event counts for %d", i);
          }
        }

        ets_delay_us(200);

        if (i2c_master_receive(NeoTrellis::neotrellis_i2c_dev[i], &keypad_events, sizeof(keypad_events), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
          ESP_LOGE("NT-KeyPad", "Failed to read keypad event counts for %d", i);
        }

        if(keypad_events == 255)
        {
          ESP_LOGE("NT-KeyPad", "Failed to read keypad event counts for %d (not ready)", i);
        }
      
        // Read Keypad events
        if(keypad_events > 0) {
          ESP_LOGI("Keypad", "Keypad %d events: %d\n", i, keypad_events);
          // keypad_events += 2;  // IDK why +2 is needed for polling
          if (keypad_events > 16)
          { keypad_events = 16; }
          neoTrellisKeyEventRaw event[16];

          uint8_t cmd[] = { SEESAW_KEYPAD_BASE, SEESAW_KEYPAD_FIFO };
          if (i2c_master_transmit(NeoTrellis::neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
            ESP_LOGE("NT-KeyPad", "Failed to read keypad events for %d", i);
          } 

          ets_delay_us(200);

          if (i2c_master_receive(NeoTrellis::neotrellis_i2c_dev[i], (uint8_t*)event, sizeof(event), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
            ESP_LOGE("NT-KeyPad", "Failed to read keypad events for %d", i);
          }
          
          for (uint8_t e = 0; e < keypad_events; e++) {

            uint8_t x = event[e].bit.NUM % 8;
            uint8_t y = event[e].bit.NUM / 8;

            if (i == 1 || i == 3)
            {
              x += 4;
            }

            if (i == 2 || i == 3)
            {
              y += 4;
            }

            if(event[e].bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING)
            {
              keypad_reading_cache[x] &= ~(1 << y);
            }
            else if(event[e].bit.EDGE == SEESAW_KEYPAD_EDGE_RISING)
            {
              keypad_reading_cache[x] |= (1 << y);
            }

            ESP_LOGI("Keypad", "Event %d: %d %d (%d %d)\n", i, event[e].bit.EDGE, event[e].bit.NUM, x, y);
          }
        }
      
        xSemaphoreGive(NeoTrellis::neotrellis_i2c_semaphore);
      }
      else
      {
        ESP_LOGE("NT-KeyPad", "Failed to take I2C semaphore");
      }
    }

    // Scan FN
    {
      bool fn_pressed = gpio_get_level(fn_pin) == (fn_active_low ? 0 : 1);
      if(!fn_pressed && virtual_fn)
      {
        fn_pressed |= ((keypad_reading_cache[3] & (1 << 3)) != 0) && 
                      ((keypad_reading_cache[3] & (1 << 4)) != 0) && 
                      ((keypad_reading_cache[4] & (1 << 3)) != 0) && 
                      ((keypad_reading_cache[4] & (1 << 4)) != 0); // center 4 keys pressed

        // Clear the center 4 keys so OS don't handle them
        if (fn_pressed)
        {
          fn_pressed = true;
          keypad_reading_cache[3] &= ~(1 << 3);
          keypad_reading_cache[3] &= ~(1 << 4);
          keypad_reading_cache[4] &= ~(1 << 3);
          keypad_reading_cache[4] &= ~(1 << 4);
        }
      }              

      bool updated = fnState.update(binary_config, fn_pressed * UINT16_MAX);
        

      if (updated)
      {
        (void)NotifyOS(FUNCTION_KEY, &fnState);
      }
    }

    for (uint8_t x = 0; x < x_size; x++)
    {
      for (uint8_t y = 0; y < y_size; y++)
      {
        Fract16 reading = ((keypad_reading_cache[x] & (1 << y)) != 0) * UINT16_MAX;
        bool updated = keypadState[x][y].update(binary_config, reading);
        if (updated)
        {
          uint16_t keyID = (1 << 12) + (x << 6) + y;
          (void)NotifyOS(keyID, &keypadState[x][y]);
        }
      }
    }

    return;
  }

  void Clear() {
    fnState.Clear();

    for (uint8_t x = 0; x < x_size; x++)
    {
      for (uint8_t y = 0; y < y_size; y++)
      { keypadState[x][y].Clear(); }
    }
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
    }
    return nullptr;  // Return an empty KeyInfo
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
    return UINT16_MAX;
  }

  // Matrix use the following ID Struct
  // CCCC IIIIIIIIIIII
  // C as class (4 bits), I as index (12 bits). I could be split by the class definition, for example, class 0 (grid),
  // it's split to XXXXXXX YYYYYYY. Class List: Class 0 - System - IIIIIIIIIIII Class 1 - Grid - XXXXXX YYYYYY Class 2
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
    }
    return Point(INT16_MIN, INT16_MIN);
  }
}