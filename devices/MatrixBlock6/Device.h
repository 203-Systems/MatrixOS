#ifndef __DEVICE_H
#define __DEVICE_H

#include "framework/Framework.h"
#include "stm32f4xx_hal.h"

#define NUMSOFLED 64
#define LED_FPS 60
#define KEYPAD_POLLRATE 120

namespace Device
{
    inline const char name[] = "Matrix Block 6 Prototype";
    inline const uint16_t numsOfLED = 64;

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

    namespace USB
    {
        void Init();
    }

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

#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB

#endif