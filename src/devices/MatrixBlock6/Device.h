#ifndef __DEVICE_H
#define __DEVICE_H

#include "../../framework/Framework.h"

#define NUMSOFLED 64
#define LED_FPS 60
#define KEYPAD_POLLRATE 120

namespace Device
{
    // char name[] = "Matrix Block 6 Prototype";

    void Hardware_Init();
    void DelayMs(uint32_t intervalMs);
    // uint32_t GetTick();
    uint32_t Millis();
    void Reboot();
    void Bootloader();

    // const uint8_t SYSEXID[3] = {0x00, 0x02, 0x03};

    namespace LED
    {
        void Init();
        uint8_t Update(Color *frameBuffer, uint8_t brightness = 255);
    }

    namespace KeyPad
    {
        void Init();
        bool Scan();
    }

    namespace USB
    {
        void Init();
    }
}
#endif