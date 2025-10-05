#include "WS2812.h"

namespace WS2812
{
  static uint16_t numsOfLED;
  static uint8_t* led_data;
  static uint8_t* dither_error;
  static bool transfer_in_progress = false;

  // STM32 specific variables
  static TIM_HandleTypeDef* ws2812_timer = nullptr;
  static uint32_t ws2812_channel;
  static DMA_HandleTypeDef hdma_tim;

  // WS2812 timing constants (for 72MHz system clock)
  const uint32_t T0H = 29;  // 0.4us at 72MHz (29 cycles)
  const uint32_t T1H = 58;  // 0.8us at 72MHz (58 cycles)
  const uint32_t T0L = 58;  // 0.85us at 72MHz (58 cycles)
  const uint32_t T1L = 29;  // 0.45us at 72MHz (29 cycles)
  const uint32_t RESET = 2400; // 50us at 72MHz (2400 cycles)

  void Init(TIM_HandleTypeDef* timer, uint32_t channel, uint16_t led_count) {
    ws2812_timer = timer;
    ws2812_channel = channel;
    numsOfLED = led_count;

    // Allocate memory for LED data (3 bytes per LED)
    led_data = (uint8_t*)malloc(numsOfLED * 3);
    if (led_data == NULL) {
      // Handle memory allocation failure
      return;
    }

    // Allocate memory for dithering error
    dither_error = (uint8_t*)malloc(numsOfLED * 3 * sizeof(uint8_t));
    if (dither_error == NULL) {
      free(led_data);
      led_data = NULL;
      return;
    }

    // Initialize dither error with random values
    for (uint16_t i = 0; i < numsOfLED * 3; i++) {
      dither_error[i] = HAL_GetTick() & 0x7F; // Simple pseudo-random
    }

    // Clear LED data
    memset(led_data, 0, numsOfLED * 3);

    // Start PWM output
    HAL_TIM_PWM_Start(ws2812_timer, ws2812_channel);
  }

  void Show(Color* buffer, std::vector<uint8_t>& brightness) {
    // Safety checks
    if (buffer == NULL || led_data == NULL || transfer_in_progress) {
      return;
    }

    // Process LED data with brightness and dithering
    for (uint16_t i = 0; i < numsOfLED; i++) {
      uint16_t data_index = i * 3;

      // Apply brightness (assuming single partition for MatrixBlock5)
      uint8_t local_brightness = brightness.size() > 0 ? brightness[0] : 255;

      led_data[data_index] = Color::scale8_video(buffer[i].G, local_brightness);
      led_data[data_index + 1] = Color::scale8_video(buffer[i].R, local_brightness);
      led_data[data_index + 2] = Color::scale8_video(buffer[i].B, local_brightness);

      // Apply dithering if enabled
      if (dithering && dither_error != NULL) {
        const uint8_t dither_error_threshold = 16;

        // Process all channels
        for (int ch = 0; ch < 3; ch++) {
          uint16_t channel_index = data_index + ch;
          if (led_data[channel_index] >= dithering_threshold) {
            uint16_t expected;
            // Get the appropriate color component based on channel
            if (ch == 0) expected = (uint16_t)buffer[i].G * local_brightness;      // Green
            else if (ch == 1) expected = (uint16_t)buffer[i].R * local_brightness;  // Red
            else expected = (uint16_t)buffer[i].B * local_brightness;               // Blue
            uint16_t actual = (uint16_t)led_data[channel_index] << 8;

            if (expected > actual) {
              uint8_t new_dither_error = (expected - actual) >> 8;
              if (new_dither_error >= dither_error_threshold) {
                dither_error[channel_index] += new_dither_error >> 1;
                if (dither_error[channel_index] >= 128) {
                  if (led_data[channel_index] < 255) {
                    led_data[channel_index] += 1;
                  }
                  dither_error[channel_index] -= 128;
                }
              }
            }
          }
        }
      }
    }

    // For STM32F1, we'll use a simple blocking approach for now
    // In a real implementation, this would use DMA

    // Convert RGB data to PWM duty cycles and send
    // This is a simplified implementation - real implementation would use DMA

    // Set transfer flag
    transfer_in_progress = true;

    // Simple blocking transmission (replace with DMA in production)
    for (uint16_t i = 0; i < numsOfLED * 3; i++) {
      uint8_t byte = led_data[i];

      // Send each bit
      for (int bit = 7; bit >= 0; bit--) {
        if (byte & (1 << bit)) {
          // Send '1' bit
          __HAL_TIM_SET_COMPARE(ws2812_timer, ws2812_channel, T1H);
          // Wait for T1H duration
          for (volatile int j = 0; j < 10; j++); // Simple delay
          __HAL_TIM_SET_COMPARE(ws2812_timer, ws2812_channel, T1L);
        } else {
          // Send '0' bit
          __HAL_TIM_SET_COMPARE(ws2812_timer, ws2812_channel, T0H);
          // Wait for T0H duration
          for (volatile int j = 0; j < 10; j++); // Simple delay
          __HAL_TIM_SET_COMPARE(ws2812_timer, ws2812_channel, T0L);
        }
        // Wait for bit period
        for (volatile int j = 0; j < 10; j++); // Simple delay
      }
    }

    // Send reset pulse
    __HAL_TIM_SET_COMPARE(ws2812_timer, ws2812_channel, 0);
    // Wait for reset duration
    for (volatile int j = 0; j < 1000; j++); // Simple delay

    transfer_in_progress = false;
  }

  // DMA callback functions
  void DMA_TransferComplete_Callback(void) {
    transfer_in_progress = false;
  }

  void DMA_TransferHalfComplete_Callback(void) {
    // Handle half transfer if needed
  }
}