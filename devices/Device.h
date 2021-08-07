#pragma once

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
        uint16_t XY2Index(Point xy); //Grid XY to global buffer index, return UINT16_MAX if not index for given XY
        uint16_t ID2Index(uint16_t ledID); //Local led Index to buffer index, return UINT16_MAX if not index for given Index
    }

    namespace KeyPad
    {
        uint16_t* Scan(); //Returns an array, first element will be # of key changed, following elements are keyID
        KeyInfo GetKey(Point keyXY);
        KeyInfo GetKey(uint16_t keyID);
        uint16_t XY2ID(Point xy); //Not sure if this is required by Matrix OS, added in for now. return UINT16_MAX if no ID is assigned to given XY
        Point ID2XY(uint16_t keyID); //Locate XY for given key ID, return Point(INT16_MIN, INT16_MIN) if no XY found for given ID;
    }   
}