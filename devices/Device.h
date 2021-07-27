#ifndef __DEVICES__
#define __DEVICES__

#include "tusb.h"
#include "framework/Color.h"

namespace Device
{
    void Device_Init();
    void Delay(uint32_t interval);
    // uint32_t GetTick();
    uint32_t Millis();
    void Reboot();
    void Bootloader();
    void Error_Handler();

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

    // namespace USB
    // {
    //     void Init();
    // }

    namespace 
    {
        void SystemClock_Config();
        void GPIO_Init();
        // static void MX_GPIO_Init(void);
        // static void MX_DMA_Init(void);
        // static void MX_USART2_UART_Init(void);
        // static void MX_TIM3_Init(void);
    }
}

#endif