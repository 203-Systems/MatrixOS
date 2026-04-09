// Define Device Keypad Function
#include "Device.h"
#include "Input/Input.h"
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
    if (NotifyOS(InputId{0, 0}, &fnState))
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

IRAM_ATTR bool NotifyOS(InputId id, KeypadInfo* keyState) {
  InputEvent inputEvent;
  inputEvent.id = id;
  inputEvent.inputClass = InputClass::Keypad;
  inputEvent.keypad = *keyState;  // slices to KeypadInfo base
  MatrixOS::Input::NewEvent(inputEvent);
  return false;
}
} // namespace Device::KeyPad
