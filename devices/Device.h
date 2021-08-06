#ifndef __DEVICE__
#define __DEVICE__

#include "framework/Framework.h"
#include "tusb.h"
#include "config.h"

namespace Device
{  
    /*Required Varaiables:
    const char name[];
    const uint16_t numsOfLED;
    */
    
    void DeviceInit();
    void DeviceTask();
    void Delay(uint32_t interval);
    // uint32_t GetTick();
    uint32_t Millis();
    void Reboot();
    void Bootloader();
    void ErrorHandler();

    namespace LED
    {
        void Update(Color* frameBuffer, uint8_t brightness = 255); //Render LED
        uint16_t XY2Index(uint8_t GridID, Point xy); //Grid XY to global buffer index
        uint16_t ID2Index(uint8_t GridID, uint16_t index); //Local led Index to buffer index
    }

    namespace KeyPad
    {
        bool Scan();
    }
}

#endif