#pragma once

#include "framework/Framework.h"
#include "tusb.h"
#include "Config.h"

namespace Device
{  
    /*
    Required Varaiables:
    const string name;
    const uint16_t numsOfLED;
    const uint8_t x_size;
    const uint8_t y_size;
    */
    
    void DeviceInit();
    void DeviceTask();
    void Delay(uint32_t interval);
    // uint32_t GetTick();
    uint32_t Millis();
    void Reboot();
    void Bootloader();
    void ErrorHandler();

    void Log(ELogLevel level, string tag, string format, ...);

    string GetSerial();

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

    #undef BKP
    namespace BKP //Back up register, presistant ram after software reset.
    {
        extern uint16_t size;
        uint32_t Read(uint32_t address);
        int8_t Write(uint32_t address, uint32_t data);
    }

    namespace NVS
    {
        std::vector<char> Read(std::string name);
        // void* Read(std::string name);
        bool Write(std::string name, void* pointer, uint16_t length);
        void Clear();
    }


}