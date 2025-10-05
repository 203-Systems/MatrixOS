#pragma once

#include <stdint.h>
#include <vector>
#include <cstring>
#include "Framework.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "timers.h"

namespace WS2812
{
  inline bool dithering = false;
  inline uint8_t dithering_threshold = 4; // Channel value lower than this will not dither

  void Init(TIM_HandleTypeDef* timer, uint32_t channel, uint16_t led_count);
  void Show(Color* buffer, std::vector<uint8_t>& brightness);

  // STM32 specific functions
  void DMA_TransferComplete_Callback(void);
  void DMA_TransferHalfComplete_Callback(void);
}