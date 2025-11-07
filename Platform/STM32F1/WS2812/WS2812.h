#pragma once

#include <stdint.h>
#include <vector>
#include <cstring>
#include <stdlib.h>
#include "Framework.h"
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "timers.h"

#define LED_DMA_END_LENGTH 80

namespace WS2812
{
  inline bool dithering = false;
  inline uint8_t dithering_threshold = 4; // Channel value lower than this will not dither

  void Init(TIM_HandleTypeDef* timer, uint32_t channel, uint16_t led_count);
  void Show(Color* buffer, std::vector<uint8_t>& brightness);
  uint32_t GetTimerPeriod();

  // DMA-based WS2812 functions
  void DMAHandler(TIM_HandleTypeDef* htim);
  void SendData();
  void PrepLEDBuffer(uint8_t brightness);

  extern uint16_t numsOfLED;
  extern uint8_t TH_DutyCycle;
  extern uint8_t TL_DutyCycle;
  extern TIM_HandleTypeDef* ws2812_timer;
  extern uint32_t ws2812_channel;
  extern Color* frameBuffer;
  extern uint8_t* pwmBuffer;
  extern uint16_t bufferSize;
  extern int32_t progress;
}