#pragma once

#include <stdint.h>
#include "driver/rmt.h"
#include "framework/Color.h"
#include "esp_log.h"

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
  void Init(rmt_channel_t rmt_channel, gpio_num_t gpio_tx, uint16_t numsOfLED);
  uint8_t Show(Color* array, uint8_t brightness = 255);

  void setup_rmt_data_buffer(Color* array, uint8_t brightness);
  // void rmt_callback(rmt_channel_t rmt_channel, void* arg);

  // extern rmt_item32_t* rmtBuffer;
  // extern bool transmit_in_progress;
  // extern uint16_t numsOfLED;
  // extern rmt_channel_t rmt_channel;
  // extern gpio_num_t gpio_tx;
}