//Define Device Specific Macro, Value and private function
#pragma once

#define GRID_8x8
#define MODEL MX1P

#define DEVICE_BATTERY
#define DEVICE_MIDI

#define MULTIPRESS 10 //Key Press will be process at once
// #define LC8812

#include "Family.h"
#include "framework/SavedVariable.h"

#define FACTORY_CONFIG V110
#define FACTORY_MFG_YEAR 22
#define FACTORY_MFG_MONTH 06

struct DeviceInfo
{
    char DeviceCode[4];
    char Revision[4];
    uint8_t ProductionYear;
    uint8_t ProductionMonth;
};

namespace Device
{
    inline DeviceInfo deviceInfo;
    const string name = "Matrix Pro";
    const string model = "MX1P";

    const string manufaturer_name = "203 Electronics";
    const string product_name = "Matrix";
    const uint16_t usb_vid = 0x0203; 
    const uint16_t usb_pid = 0x1040; //(Device Class)0001 (Device Code)000001 (Reserved for Device ID (0~63))000000

    const uint16_t numsOfLED = 64 + 32;
    inline uint16_t keypad_scanrate = 120;
    const uint8_t x_size = 8;
    const uint8_t y_size = 8;
    const uint8_t touchbar_size = 16; //Not required by the API, private use.

    namespace KeyPad
    {
        inline bool FSR;

        inline gpio_num_t fn_pin;
        inline bool fn_active_low = true;

        inline Fract16 low_threshold = 2560;
        inline Fract16 high_threshold = 57344;

        inline gpio_num_t keypad_write_pins[8];
        inline gpio_num_t keypad_read_pins[8];
        inline adc1_channel_t keypad_read_adc_channel[8];

        inline gpio_num_t touchData_Pin;
        inline gpio_num_t touchClock_Pin;
        inline uint8_t touchbar_map[touchbar_size]; //Touch number as index and touch location as value (Left touch down and then right touch down)

        inline KeyInfo keypadState[x_size][y_size];
        inline KeyInfo touchbarState[touchbar_size];
        inline uint16_t changeList[MULTIPRESS + 1];
    }

    //LED
    #define MAX_LED_LAYERS 5
    inline gpio_num_t led_pin;
    inline uint16_t fps = 120; //Depends on the FreeRTOS tick speed
    inline uint8_t brightness_level[9] = {8, 12, 24, 40, 64, 90, 128, 168, 255};
    // const Dimension grid_size(8,8);
    // const Point grid_offset = Point(1,1);

    //Load Device config
    void LoadV100();
    void LoadV110();
}
