#pragma once

#include <stdint.h>
#include <vector>
#include <cstring>
#include "Framework.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_check.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"

#define BITS_PER_LED_CMD 24
#define LED_BUFFER_ITEMS ((NUM_LEDS * BITS_PER_LED_CMD))

#ifdef LC8812
#define WS2812_T0H_NS 300
#define WS2812_T0L_NS 900
#define WS2812_T1H_NS 600
#define WS2812_T1L_NS 600
#define WS2812_RESET_US 80
#else
#define WS2812_T0H_NS 350
#define WS2812_T0L_NS 1000
#define WS2812_T1H_NS 1000
#define WS2812_T1L_NS 350
#define WS2812_RESET_US 280
#endif

// // These values are determined by measuring pulse timing with logic analyzer and adjusting to match datasheet.
// #define T0H 14  // 0 bit high time
// #define T1H 52  // 1 bit high time
// #define TL  52  // low time for either bit

namespace WS2812
{
  inline bool dithering = true;
  inline uint8_t dithering_threshold = 4; // Channel value lower than this will not dither
  void Init(gpio_num_t gpio_pin, std::vector<LEDPartition>& led_partitions);
  IRAM_ATTR void Show(Color* buffer, std::vector<uint8_t>& brightness);
}