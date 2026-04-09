// Define Device Keypad Function
#include "Device.h"
#include "Input/Input.h"
#include "Framework/Input/InputCompat.h"
#include "timers.h"
#include "driver/gpio.h"
#include "MatrixOSConfig.h"

namespace Device::KeyPad
{
StaticTimer_t keypadTimerDef;
TimerHandle_t keypadTimer;

void Init() {
  InitFN();
  InitKeyPad();
  InitTouchBar();
}

// TODO Change to use interrupt
void InitFN() {
  gpio_config_t io_conf;

  // Config FN
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pin_bit_mask = (1ULL << fnPin);
#ifdef FN_PIN_ACTIVE_HIGH // Active HIGH
  io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
#else // Active Low
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
#endif
  gpio_config(&io_conf);
}

void InitKeyPad() {
  if (!velocitySensitivity)
  {
    Binary::Init();
  }
  else
  {
    FSR::Init();
  }
}

// Timer callback wrapper with correct signature
static void KeypadTimerCallback(TimerHandle_t xTimer) {
  (void)xTimer;
  Scan();
}

void StartKeyPad() {
  if (!velocitySensitivity)
  {
    Binary::Start();
  }
  else
  {
    FSR::Start();
  }

  keypadTimer = xTimerCreateStatic(NULL, configTICK_RATE_HZ / keypadScanrate, true, NULL, KeypadTimerCallback, &keypadTimerDef);

  xTimerStart(keypadTimer, 0);
}

void Start() {
  StartKeyPad();
  StartTouchBar();
}

IRAM_ATTR void Scan() {
  ScanFN();
  ScanKeyPad();
}

IRAM_ATTR bool ScanKeyPad() {
  if (!velocitySensitivity)
  {
    return Binary::Scan();
  }
  else
  {
    return FSR::Scan();
  }
}

IRAM_ATTR bool ScanFN() {
  Fract16 read = gpio_get_level(fnPin) ? 0 : UINT16_MAX;
  // ESP_LOGI("FN", "%d", gpio_get_level(fnPin));
  if (fnState.Update(binaryConfig, read))
  {
    if (NotifyOS(FUNCTION_KEY, &fnState))
    {
      return true;
    }
  }
  return false;
}

void Clear() {
  fnState.Clear();

  for (uint8_t x = 0; x < X_SIZE; x++)
  {
    for (uint8_t y = 0; y < Y_SIZE; y++)
    {
      keypadState[x][y].Clear();
    }
  }

  for (uint8_t i = 0; i < touchbarSize; i++)
  {
    touchbarState[i].Clear();
  }
}

KeyInfo* GetKey(uint16_t keyID) {
  uint8_t keyClass = keyID >> 12;
  switch (keyClass)
  {
  case 0: // System
  {
    uint16_t index = keyID & (0b0000111111111111);
    switch (index)
    {
    case 0:
      return &fnState;
    }
    break;
  }
  case 1: // Main Grid
  {
    int16_t x = (keyID & (0b0000111111000000)) >> 6;
    int16_t y = keyID & (0b0000000000111111);
    if (x < X_SIZE && y < Y_SIZE)
      return &keypadState[x][y];
    break;
  }
  case 2: // Touch Bar
  {
    uint16_t index = keyID & (0b0000111111111111);
    // MLOGD("Keypad", "Read Touch %d", index);
    if (index < touchbarSize)
      return &touchbarState[index];
    break;
  }
  }
  return nullptr; // Return an empty KeyInfo
}

IRAM_ATTR bool NotifyOS(uint16_t keyID, KeyInfo* keyInfo) {
  InputEvent inputEvent;
  inputEvent.id = BridgeKeyId(keyID);
  inputEvent.inputClass = InputClass::Keypad;
  inputEvent.keypad = KeyInfoToKeypadInfo(*keyInfo);
  MatrixOS::Input::NewEvent(inputEvent);
  return false;
}

uint16_t XY2ID(Point xy) {
  if (xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8) // Main grid
  {
    return (1 << 12) + (xy.x << 6) + xy.y;
  }
  else if ((xy.x == -1 || xy.x == 8) && (xy.y >= 0 && xy.y < 8)) // Touch Bar
  {
    return (2 << 12) + xy.y + (xy.x == 8) * 8;
  }
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
  case 1: // Main Grid
  {
    int16_t x = (keyID & 0b0000111111000000) >> 6;
    int16_t y = keyID & (0b0000000000111111);
    if (x < X_SIZE && y < Y_SIZE)
      return Point(x, y);
    break;
  }
  case 2: // TouchBar
  {
    uint16_t index = keyID & (0b0000111111111111);
    if (index < touchbarSize)
    {
      if (index / 8) // Right
      {
        return Point(X_SIZE, index % 8);
      }
      else // Left
      {
        return Point(-1, index % 8);
      }
    }
    break;
  }
  }
  return Point(INT16_MIN, INT16_MIN);
}
} // namespace Device::KeyPad
