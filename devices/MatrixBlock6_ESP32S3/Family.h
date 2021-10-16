//Declear Family specific function
#pragma once

#include "esp_rom_gpio.h"
#include "hal/gpio_ll.h"
#include "hal/usb_hal.h"
#include "soc/usb_periph.h"

#include "driver/periph_ctrl.h"
#include "driver/rmt.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"
// #include "freertos/queue.h"
// #include "freertos/task.h"
// #include "freertos/timers.h"

namespace Device
{
    void USB_Init();
    void LED_Init();
    void KeyPad_Init();
    void TouchBar_Init();
    void FLASH_Init();
}