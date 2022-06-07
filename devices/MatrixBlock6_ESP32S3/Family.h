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

#include "esp_timer.h"
#include "esp_adc_cal.h"
#include "esp_task_wdt.h"
#include "esp_rom_gpio.h"
#include "esp_log.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"

#include "nvs_flash.h"

#include "esp_private/system_internal.h"

#include "WS2812/WS2812.h"
#include "framework/Color.h"

#define FUNCTION_KEY 0 //Keypad Code for main function key
#define DEVICE_APPLICATIONS
#define DEVICE_SETTING

#define DEVICE_SAVED_VAR_SCOPE "Device"

namespace Device
{   
    //Device Variable
    inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, bluetooth, bool, false);

    void LoadDeviceInfo();
    void LoadVarientInfo();

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

        inline KeyInfo fnState;

        void InitKeyPad();
        void InitTouchBar();

        void FNScan();
        void KeyPadScan();
        void TouchBarScan();

        bool addToList(uint16_t keyID); //Return true when list is full. 
        void clearList();
        bool isListFull();
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

    namespace BLEMIDI
    {
        extern bool started;
        void Init(string name);
        void Toggle();
        void Start();
        void Stop();
        bool SendMidi(uint8_t* packet);
        uint32_t MidiAvailable();
        MidiPacket GetMidi();
    }

    namespace ESPNOW
    {
        extern bool started;
        void Init();
        void Flush(void* param);
        bool SendMidi(uint8_t* packet);
        uint32_t MidiAvailable();
        MidiPacket GetMidi();
        void BroadcastMac();
        void UpdatePeer(const uint8_t* new_mac);
    }
}