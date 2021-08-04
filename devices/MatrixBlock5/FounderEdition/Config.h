//Define Device Specific Macro and Value
#ifndef __DEVICE__CONFIG_H
#define __DEVICE__CONFIG_H

#include "WS2812.h"
#include "Family.h"

extern TIM_HandleTypeDef htim8;
extern DMA_HandleTypeDef hdma_tim8_ch2;

namespace Device
{
    const char name[] = "Matrix Founder Edition";
    const uint16_t numsOfLED = 64;

    void MX_DMA_Init(void);
    void MX_TIM8_Init(void);
}

extern "C"
{
    void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

    void NMI_Handler(void);
    void HardFault_Handler(void);
    void MemManage_Handler(void);
    void BusFault_Handler(void);
    void UsageFault_Handler(void);
    void SVC_Handler(void);
    void DebugMon_Handler(void);
    void PendSV_Handler(void);
    void SysTick_Handler(void);
    void DMA2_Channel4_5_IRQHandler(void);
}

#endif