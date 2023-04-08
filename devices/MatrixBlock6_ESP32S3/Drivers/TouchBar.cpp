#include "Device.h"
#include "timers.h"

namespace Device::KeyPad
{
  // TODO Change to use interrupt

  StaticTimer_t touchbar_timer_def;
  TimerHandle_t touchbar_timer;

  void TouchBarTimerHandler()  // This exists because return type of TouchBarScan is bool
  {
    if(touchbar_enable) ScanTouchBar();
  }

  void InitTouchBar() {
    // Set Touch Data Pin
    gpio_config_t data_io_conf;
    data_io_conf.intr_type = GPIO_INTR_DISABLE;
    data_io_conf.mode = GPIO_MODE_INPUT;
    data_io_conf.pin_bit_mask = (1ULL << touchData_Pin);
    data_io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    data_io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&data_io_conf);

    // Set Touch Clock Pin
    gpio_config_t clock_io_conf;
    clock_io_conf.intr_type = GPIO_INTR_DISABLE;
    clock_io_conf.mode = GPIO_MODE_OUTPUT;
    clock_io_conf.pin_bit_mask = (1ULL << touchClock_Pin);
    clock_io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    clock_io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&clock_io_conf);

    for (uint8_t i = 0; i < touchbar_size; i++)
    { touchbarState[i].setConfig(&touch_config); }
  }

  void StartTouchBar() {
    touchbar_timer =
        xTimerCreateStatic(NULL, configTICK_RATE_HZ / Device::touchbar_scanrate, true, NULL,
                           reinterpret_cast<TimerCallbackFunction_t>(TouchBarTimerHandler), &touchbar_timer_def);
    xTimerStart(touchbar_timer, 0);
  }

  bool ScanTouchBar() {
    for (uint8_t i = 0; i < touchbar_size; i++)
    {
      gpio_set_level(touchClock_Pin, 1);

      Fract16 reading = gpio_get_level(touchData_Pin) * UINT16_MAX;

      gpio_set_level(touchClock_Pin, 0);

      uint8_t key_id = touchbar_map[i];
      bool updated = touchbarState[key_id].update(reading, false);
      if (updated)
      {
        uint16_t keyID = (2 << 12) + key_id;
        if (NotifyOS(keyID, &touchbarState[key_id]))
        { return true; }
      }
    }
    return false;
  }
}