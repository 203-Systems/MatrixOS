#pragma once

#include "framework/Framework.h"
#include "tusb.h"
#include "Config.h"
// #include "framework/UI.h"

namespace Device
{  
    /*
    Required Varaiables:
    const string name;
    const uint16_t numsOfLED;
    const uint8_t x_size;
    const uint8_t y_size;
    string serial_number;
    */
    
    void DeviceInit();
    void PostBootTask();
    void DeviceTask();
    void Reboot();
    void Bootloader();
    void ErrorHandler();

    void Log(string format, va_list valst);

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
        void Clear(); //Since only Device layer aware the keyInfo buffer, the function's job is to run Clear() on all keyInfo
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
        vector<char> Read(uint32_t hash);
        // void* Read(uint32_t hash);
        bool Write(uint32_t hash, void* pointer, uint16_t length);
        bool Delete(uint32_t hash);
        void Clear();
    }


    #ifdef DEVICE_BATTERY
    namespace Battery
    {   
        bool Chagring();
        float Voltage();
    }
    #endif

    #ifdef DEVICE_MIDI
    namespace MIDI
    {
        uint32_t Available();
        MidiPacket Get();
        bool Send(MidiPacket packet);
    } 
    #endif

}

namespace MatrixOS::Logging
{
    void LogError (string tag, string format, ...);
    void LogWarning (string tag, string format, ...);
    void LogInfo (string tag, string format, ...);
    void LogDebug (string tag, string format, ...);
    void LogVerbose (string tag, string format, ...);
}

namespace MatrixOS::SYS
{
    void ErrorHandler(string error);
}