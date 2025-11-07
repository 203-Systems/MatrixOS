#include "WS2812.h"

namespace Device
{
  void ErrorHandler(void);
}

namespace WS2812
{
  uint16_t numsOfLED;
  uint8_t TH_DutyCycle;
  uint8_t TL_DutyCycle;
  TIM_HandleTypeDef* ws2812_timer = nullptr;
  uint32_t ws2812_channel;
  Color* frameBuffer = nullptr;
  uint8_t* pwmBuffer = nullptr;
  uint16_t bufferSize;
  int32_t progress = -1;  // -1 means signal end has been sent and ready for new job

  void Init(TIM_HandleTypeDef* timer, uint32_t channel, uint16_t led_count) {
    ws2812_timer = timer;
    ws2812_channel = channel;
    numsOfLED = led_count;

    // Calculate buffer size and allocate PWM buffer
    bufferSize = numsOfLED * 24 + LED_DMA_END_LENGTH * 2;
    pwmBuffer = (uint8_t*)calloc(bufferSize, 1);
    if (pwmBuffer == NULL) {
      Device::ErrorHandler();
      return;
    }

    // Register TIM callback for DMA completion
    if (HAL_TIM_RegisterCallback(ws2812_timer, HAL_TIM_PWM_PULSE_FINISHED_CB_ID, DMAHandler) != HAL_OK) {
      Device::ErrorHandler();
    }
  }

  uint32_t GetTimerPeriod() {
    uint32_t timerPeriod;
    if (HAL_RCC_GetHCLKFreq() % 800000 == 0) {
      timerPeriod = (HAL_RCC_GetHCLKFreq() / 800000) - 1;
    }
    else {
      return -1;  // Error: clock frequency not compatible
    }

    // Calculate duty cycles for WS2812 timing
    // TH (high time for '1'): ~0.8us = 173/256 of period
    // TL (high time for '0'): ~0.4us = 82/256 of period
    TH_DutyCycle = (timerPeriod * 173) >> 8;
    TL_DutyCycle = (timerPeriod * 82) >> 8;

    return timerPeriod;
  }

  void Show(Color* buffer, std::vector<uint8_t>& brightness) {
    // Safety checks
    if (buffer == NULL || pwmBuffer == NULL || progress != -1) {
      return;
    }

    // Get brightness value (assume single partition for MatrixBlock5)
    uint8_t local_brightness = brightness.size() > 0 ? brightness[0] : 255;

    // Set up for DMA transfer
    progress = 0;
    frameBuffer = buffer;
    PrepLEDBuffer(local_brightness);
    SendData();
  }

  void DMAHandler(TIM_HandleTypeDef* htim) {
    HAL_TIM_PWM_Stop_DMA(htim, ws2812_channel);
    progress = -1;
  }

  void SendData() {
    HAL_TIM_PWM_Start_DMA(ws2812_timer, ws2812_channel, (uint32_t*)pwmBuffer, bufferSize);
  }

  void PrepLEDBuffer(uint8_t brightness) {
    uint16_t index = 0;

    // Add start padding
    for (uint8_t i = 0; i < LED_DMA_END_LENGTH; i++) {
      pwmBuffer[index] = 0;
      index++;
    }

    // Fill PWM buffer with LED data
    while (index <= (bufferSize - 24) && progress < numsOfLED) {
      // Get GRB value with brightness applied
      uint32_t GRB = frameBuffer[progress].GRB(brightness);

      // Convert each bit to PWM duty cycle
      for (int8_t i = 23; i >= 0; i--) {
        pwmBuffer[index] = (GRB & (1 << i)) ? TH_DutyCycle : TL_DutyCycle;
        index++;
      }
      progress++;
    }

    // Add end padding
    for (uint8_t i = 0; i < LED_DMA_END_LENGTH; i++) {
      pwmBuffer[index] = 0;
      index++;
    }
  }
}
