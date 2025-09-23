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

namespace WS2812
{
  inline bool dithering = false;
  inline uint8_t dithering_threshold = 4; // Channel value lower than this will not dither
  void Init(gpio_num_t gpio_pin, std::vector<LEDPartition>& partitions);
  void Show(Color* buffer, std::vector<uint8_t>& brightness);
}