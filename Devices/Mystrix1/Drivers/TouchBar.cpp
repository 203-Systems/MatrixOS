#include "Device.h"
#include "timers.h"
#include "driver/gpio.h"

namespace Device::KeyPad
{
// TODO Change to use interrupt

StaticTimer_t touchbar_timer_def;
TimerHandle_t touchbarTimer;

IRAM_ATTR void TouchBarTimerHandler() // This exists because return type of TouchBarScan is bool
{
  if (touchbarEnable)
    ScanTouchBar();
}

void InitTouchBar() {
  // Set Touch Data Pin
  gpio_config_t dataIoConf;
  dataIoConf.intr_type = GPIO_INTR_DISABLE;
  dataIoConf.mode = GPIO_MODE_INPUT;
  dataIoConf.pin_bit_mask = (1ULL << touch_data_pin);
  dataIoConf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  dataIoConf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&dataIoConf);

  // Set Touch Clock Pin
  gpio_config_t clockIoConf;
  clockIoConf.intr_type = GPIO_INTR_DISABLE;
  clockIoConf.mode = GPIO_MODE_OUTPUT;
  clockIoConf.pin_bit_mask = (1ULL << touch_clock_pin);
  clockIoConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  clockIoConf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&clockIoConf);
}

void StartTouchBar() {
  touchbarTimer = xTimerCreateStatic(NULL, configTICK_RATE_HZ / touchbarScanrate, true, NULL,
                                      reinterpret_cast<TimerCallbackFunction_t>(TouchBarTimerHandler), &touchbar_timer_def);
  xTimerStart(touchbarTimer, 0);
}

IRAM_ATTR bool ScanTouchBar() {
  for (uint8_t i = 0; i < touchbarSize; i++)
  {
    gpio_set_level(touch_clock_pin, 1);

    Fract16 reading = gpio_get_level(touch_data_pin) * UINT16_MAX;

    gpio_set_level(touch_clock_pin, 0);

    uint8_t keyId = touchbarMap[i];
    bool updated = touchbarState[keyId].Update(binaryConfig, reading);
    if (updated)
    {
      uint16_t keyID = (2 << 12) + keyId;
      if (NotifyOS(keyID, &touchbarState[keyId]))
      {
        return true;
      }
    }
  }
  return false;
}
} // namespace Device::KeyPad
