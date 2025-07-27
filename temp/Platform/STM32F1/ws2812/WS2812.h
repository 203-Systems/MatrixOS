#ifndef __WS2812_H
#define __WS2812_H

#include "framework/Color.h"
#include "stm32f1xx_hal.h"
#include <stdlib.h>

#define LED_DMA_END_LENGTH 80

namespace WS2812
{
  void Init(TIM_HandleTypeDef* htim, unsigned int TIM_Channel, uint16_t NumsOfLED);
  uint32_t GetTimerPeriod();
  uint8_t Show(Color* array, uint8_t brightness = 255);
  void DMAHandler(TIM_HandleTypeDef* htim);
  void SendData();
  void SendEndBuffer();
  void PrepLEDBuffer(uint8_t brightness);

  extern uint16_t numsOfLED;
  extern uint8_t TH_DutyCycle;
  extern uint8_t TL_DutyCycle;
  extern TIM_HandleTypeDef* htim;
  extern unsigned int tim_Channel;
  extern Color* frameBuffer;
  extern uint8_t* pwmBuffer;
  extern uint16_t bufferSize;
  extern int32_t progress;  //# of led sent, -1 means end signal has been send and ready for new job, -2 means end
                            //signal transmit in progress
}

#endif
