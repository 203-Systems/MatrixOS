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

} // namespace Device::KeyPad

namespace Device::Input
{
void SuppressActiveInputs() {
  KeyPad::fnState.Suppress();

  for (uint8_t x = 0; x < X_SIZE; x++)
  {
    for (uint8_t y = 0; y < Y_SIZE; y++)
    {
      KeyPad::keypadState[x][y].Suppress();
    }
  }

  for (uint8_t i = 0; i < KeyPad::touchbarSize; i++)
  {
    KeyPad::touchbarState[i].Suppress();
  }
}

bool GetState(InputId id, InputSnapshot* snapshot) {
  KeypadInfo* info = nullptr;

  switch (id.clusterId)
  {
    case 0: // FN button
      if (id.memberId != 0) return false;
      info = &KeyPad::fnState;
      break;
    case 1: // Main grid
    {
      uint8_t x = id.memberId % X_SIZE;
      uint8_t y = id.memberId / X_SIZE;
      if (id.memberId >= X_SIZE * Y_SIZE) return false;
      info = &KeyPad::keypadState[x][y];
      break;
    }
    case 2: // TouchBar Left
      if (id.memberId >= KeyPad::touchbarSize / 2) return false;
      info = &KeyPad::touchbarState[id.memberId];
      break;
    case 3: // TouchBar Right
      if (id.memberId >= KeyPad::touchbarSize / 2) return false;
      info = &KeyPad::touchbarState[KeyPad::touchbarSize / 2 + id.memberId];
      break;
    default:
      return false;
  }

  memset(snapshot, 0, sizeof(*snapshot));
  snapshot->id = id;
  snapshot->inputClass = InputClass::Keypad;
  snapshot->keypad = *info;
  return true;
}

bool GetKeypadCapabilities(uint8_t clusterId, KeypadCapabilities* caps) {
  switch (clusterId)
  {
    case 0: // FN button: binary only
      *caps = {false, false, false, false};
      return true;
    case 1: // Main grid
      *caps = {true, true, KeyPad::velocitySensitivity, true};
      return true;
    case 2: // TouchBar Left
    case 3: // TouchBar Right
      *caps = {false, false, false, false};
      return true;
    default:
      return false;
  }
}
} // namespace Device::Input

namespace Device::KeyPad
{
IRAM_ATTR bool NotifyOS(InputId id, KeypadInfo* keyState) {
  InputEvent inputEvent;
  inputEvent.id = id;
  inputEvent.inputClass = InputClass::Keypad;
  inputEvent.keypad = *keyState;  // slices to KeypadInfo base
  MatrixOS::Input::NewEvent(inputEvent);
  return false;
}
} // namespace Device::KeyPad
