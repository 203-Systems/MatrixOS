// Define Device Keypad Function
#include "Device.h"
#include "timers.h"
#include "MatrixOSConfig.h"

namespace Device::KeyPad
{
  StaticTimer_t keypad_timer_def;
  TimerHandle_t keypad_timer;

  void Init() {
    InitFN();
    InitKeyPad();
    InitTouchBar();
  }

  void InitFN() {
    // FN key is already configured in Family.cpp's KeyPad_Init()
    // This function is kept for API compatibility
  }

  void InitKeyPad() {
    // Keypad is already configured in Family.cpp's KeyPad_Init()
    // This function is kept for API compatibility
  }

  void InitTouchBar() {
    // TODO: Initialize touch bar for STM32F103
  }

  // Timer callback wrapper with correct signature
  static void KeypadTimerCallback(TimerHandle_t xTimer) {
    (void)xTimer;
    Scan();
  }

  void StartKeyPad() {
    // Create FreeRTOS timer for keypad scanning
    keypad_timer = xTimerCreateStatic(NULL, configTICK_RATE_HZ / keypad_scanrate, true, NULL, KeypadTimerCallback, &keypad_timer_def);
    xTimerStart(keypad_timer, 0);
  }

  void StartTouchBar() {
    // TODO: Start touch bar scanning for STM32F103
  }

  void Start() {
    StartKeyPad();
    StartTouchBar();
  }

  void Scan() {
    ScanFN();
    ScanKeyPad();
  }

  bool ScanKeyPad() {
    // MatrixBlock5 uses binary keypad scanning (no FSR support)
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      // Activate column
      HAL_GPIO_WritePin(keypad_write_ports[x], keypad_write_pins[x], GPIO_PIN_SET);

      for (uint8_t y = 0; y < Y_SIZE; y++)
      {
        // Read row
        if (keypadState[x][y].Update(keypad_config, read))
        {
          uint16_t keyID = (1 << 12) + (x << 6) + y;
          if (NotifyOS(keyID, &keypadState[x][y]))
          {
            // Deactivate column and return if OS notification interrupted
            HAL_GPIO_WritePin(keypad_write_ports[x], keypad_write_pins[x], GPIO_PIN_RESET);
            return true;
          }
        }
      }

      // Deactivate column
      HAL_GPIO_WritePin(keypad_write_ports[x], keypad_write_pins[x], GPIO_PIN_RESET);

      // Small delay for settling
      volatile int i;
      for (i = 0; i < 5; ++i) {}
    }
    return false;
  }

  bool ScanFN() {
    Fract16 read = HAL_GPIO_ReadPin(fn_port, fn_pin) * UINT16_MAX;
    if (fn_active_low)
    { read = UINT16_MAX - (uint16_t)read; }
    if (fnState.Update(keypad_config, read))
    {
      if (NotifyOS(0, &fnState))
      { return true; }
    }
    return false;
  }

  void Clear() {
    fnState.Clear();

    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      for (uint8_t y = 0; y < Y_SIZE; y++)
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
        if (x < X_SIZE && y < Y_SIZE)
          return &keypadState[x][y];
        break;
      }
      case 2:  // Touch Bar
      {
        uint16_t index = keyID & (0b0000111111111111);
        if (index < touchbar_size)
          return &touchbarState[index];
        break;
      }
    }
    return nullptr;
  }

  bool NotifyOS(uint16_t keyID, KeyInfo* keyInfo) {
    KeyEvent keyEvent;
    keyEvent.id = keyID;
    keyEvent.info = *keyInfo;
    return MatrixOS::KeyPad::NewEvent(&keyEvent);
  }

  uint16_t XY2ID(Point xy) {
    if (xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)  // Main grid
    { return (1 << 12) + (xy.x << 6) + xy.y; }
    else if ((xy.x == -1 || xy.x == 8) && (xy.y >= 0 && xy.y < 8))  // Touch Bar
    { return (2 << 12) + xy.y + (xy.x == 8) * 8; }
    return UINT16_MAX;
  }

  Point ID2XY(uint16_t keyID) {
    uint8_t keyClass = keyID >> 12;
    switch (keyClass)
    {
      case 1:  // Main Grid
      {
        int16_t x = (keyID & 0b0000111111000000) >> 6;
        int16_t y = keyID & (0b0000000000111111);
        if (x < X_SIZE && y < Y_SIZE)
          return Point(x, y);
        break;
      }
      case 2:  // TouchBar
      {
        uint16_t index = keyID & (0b0000111111111111);
        if (index < touchbar_size)
        {
          if (index / 8)  // Right
          { return Point(X_SIZE, index % 8); }
          else  // Left
          { return Point(-1, index % 8); }
        }
        break;
      }
    }
    return Point(INT16_MIN, INT16_MIN);
  }
}