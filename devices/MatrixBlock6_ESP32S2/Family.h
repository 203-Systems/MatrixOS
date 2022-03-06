//Declear Family specific function
#pragma once
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "hal/gpio_ll.h"
#include "hal/usb_hal.h"
#include "soc/usb_periph.h"

#include "driver/periph_ctrl.h"
#include "driver/rmt.h"
#include "driver/adc.h"
#include "driver/adc_common.h"

#include "esp_adc_cal.h"
#include "esp_task_wdt.h"
#include "esp_rom_gpio.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "esp_private/system_internal.h"

#include "WS2812/WS2812.h"
#include "framework/Color.h"



#define DONT_START_FREERTOS_SCHEDULER

namespace Device
{   
    namespace USB
    {
        void Init();
    }
    namespace LED
    {
        void Init();
    }
    
    namespace KeyPad
    {
        void Init();
    }
    
    namespace TouchBar
    {
        void Init();
    }
    
    namespace NVS
    {
        void Init();
    }



    namespace WIFI
    {
        void Init();
    }

    namespace ESPNOW
    {
        void Init();
        void Flush(void* param);
        bool SendMidi(uint8_t* packet);
        uint32_t MidiAvailable();
        MidiPacket GetMidi();
        void BroadcastMac();
        void UpdatePeer(const uint8_t* new_mac);
    }
}