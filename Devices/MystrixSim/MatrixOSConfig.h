#pragma once
#include "Framework.h"

#define X_SIZE 8
#define Y_SIZE 8

#define OS_SHELL APPID("203 Systems", "Shell")
#define DEFAULT_BOOTANIMATION APPID("203 Systems", "Mystrix Boot")

namespace Device
{
inline string name = "MystrixSim";
inline string model = "MXSIM";

inline string manufacturerName = "203 Systems";
inline string productName = "Mystrix Sim";
inline uint16_t usb_vid = 0x0203;
inline uint16_t usb_pid = 0x1041;

inline uint8_t xSize = X_SIZE;
inline uint8_t ySize = Y_SIZE;

namespace LED
{
#define MAX_LED_LAYERS 8
const inline uint16_t fps = 60;

inline uint16_t count = 64 + 32;
inline uint8_t brightnessLevel[8] = {8, 22, 39, 60, 84, 110, 138, 169};
#define FINE_LED_BRIGHTNESS
inline uint8_t brightnessFineLevel[16] = {8, 16, 26, 38, 50, 64, 80, 96, 112, 130, 149, 169, 189, 209, 232, 255};

inline vector<LEDPartition> partitions = {
    {"Grid", 1.0, 0, 64, RGB_24B},
    {"Underglow", 4.0, 64, 32, RGB_24B},
};
} // namespace LED
} // namespace Device
