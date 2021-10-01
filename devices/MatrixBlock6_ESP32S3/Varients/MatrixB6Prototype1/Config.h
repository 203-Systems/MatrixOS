//Define Device Specific Macro, Value and private function
#pragma once

#include "Family.h"

#define GRID_8x8
#define MODEL MXB6PT1
#define MULTIPRESS 10 //Key Press will be process at once

namespace Device
{
    const char name[] = "Matrix Block6 Prototype 1.0";
    const char model[] = "MXB6PT1";
    const uint16_t numsOfLED = 64;
    const uint8_t x_size = 8;
    const uint8_t y_size = 8;

    const uint8_t touchbar_size = 8; //Not required by the API, private use. 16 Physical but 8 virtualized key.

    void USB_Init();

    namespace KeyPad
    {
        inline KeyInfo fnState;
        inline KeyInfo keypadState[x_size][y_size];
        inline KeyInfo touchbarState[x_size];
        inline uint16_t changeList[MULTIPRESS + 1];

        void FNScan();
        void KeyPadScan();
        void TouchBarScan();

        bool addToList(uint16_t keyID); //Return true when list is full. 
        void clearList();
        bool isListFull();
    }
}

#define FN_Pin 11

#define Key1_Pin 21
#define Key2_Pin 26
#define Key3_Pin 33
#define Key4_Pin 34
#define Key5_Pin 35
#define Key6_Pin 36
#define Key7_Pin 37
#define Key8_Pin 38

#define KeyRead1_Pin 1
#define KeyRead2_Pin 2
#define KeyRead3_Pin 3
#define KeyRead4_Pin 4
#define KeyRead5_Pin 5
#define KeyRead6_Pin 6
#define KeyRead7_Pin 7
#define KeyRead8_Pin 8

#define TouchData_Pin 12
#define TouchClock_Pin 13

#define PowerCord_Pin 18

#define Battery_CHRG_Pin 17
#define Battery_STDBY_Pin 16

#define VBAT_Sensing_Pin 9
#define VBUS_Sensing_Pin 10

#define Matrix_Mod_GPIO_Pin 14

inline uint16_t keypad_write_pins[] = {
    Key1_Pin,
    Key2_Pin,
    Key3_Pin,
    Key4_Pin,
    Key5_Pin,
    Key6_Pin,
    Key7_Pin,
    Key8_Pin,
};

inline uint16_t keypad_read_pins[] = {
    KeyRead1_Pin,
    KeyRead2_Pin,
    KeyRead3_Pin,
    KeyRead4_Pin,
    KeyRead5_Pin,
    KeyRead6_Pin,
    KeyRead7_Pin,
    KeyRead8_Pin,
};