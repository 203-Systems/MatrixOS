#include "WS2812.h"
#include <algorithm>
// #include "MatrixOS.h"

namespace WS2812
{
  // static const char *TAG = "WS2812";

  rmt_item32_t* rmtBuffer;
  // bool transmit_in_progress = false;
  uint16_t numsOfLED;
  uint8_t chunkCount;
  ws2812_chunk* chunkInfo;

  rmt_channel_t rmt_channel;

  static uint32_t ws2812_t0h_ticks = 0;
  static uint32_t ws2812_t1h_ticks = 0;
  static uint32_t ws2812_t0l_ticks = 0;
  static uint32_t ws2812_t1l_ticks = 0;
  static uint32_t ws2812_reset_ticks = 0;

  void Init(rmt_channel_t rmt_channel, gpio_num_t gpio_tx, uint8_t chunk_count, ws2812_chunk* chunk_info) {
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(gpio_tx, rmt_channel);
    // set counter clock to 40MHz
    config.clk_div = 2;
    config.mem_block_num = 3;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, ESP_INTR_FLAG_IRAM));

    WS2812::rmt_channel = rmt_channel;
    WS2812::numsOfLED = 0;
    WS2812::chunkInfo = chunk_info;
    WS2812::chunkCount = chunk_count;

    for (uint8_t chunk_index = 0; chunk_index < chunk_count; chunk_index++)
    {
      numsOfLED += chunk_info[chunk_index].length;
      // ESP_LOGI("LED Init", "Chunk #%d - Size: %d", chunk_index, chunk_info[chunk_index].length);
    }

    // TODO Free rmtBuffer if reinit()

    rmtBuffer = (rmt_item32_t*)calloc(numsOfLED * BITS_PER_LED_CMD + 1, sizeof(rmt_item32_t));
    // rmt_register_tx_end_callback(&rmt_callback, NULL);

    uint32_t counter_clk_hz = 0;
    if (rmt_get_counter_clock(rmt_channel, &counter_clk_hz) != ESP_OK)
    {
      // ESP_LOGE(TAG, "get rmt counter clock failed");
    }
    // ns -> ticks
    // ESP_LOGV(TAG, "counter_clk_hz %i", counter_clk_hz);
    float ratio = (float)counter_clk_hz / 1e9;
    // ESP_LOGV(TAG, "ratio %f", ratio);

    ws2812_t0h_ticks = (uint32_t)(ratio * WS2812_T0H_NS);
    ws2812_t0l_ticks = (uint32_t)(ratio * WS2812_T0L_NS);
    ws2812_t1h_ticks = (uint32_t)(ratio * WS2812_T1H_NS);
    ws2812_t1l_ticks = (uint32_t)(ratio * WS2812_T1L_NS);
    ws2812_reset_ticks = (uint32_t)(ratio * WS2812_RESET_US * 1000);

    // ESP_LOGV(TAG, "ws2812_t0h_ticks %d", ws2812_t0h_ticks);
    // ESP_LOGV(TAG, "ws2812_t0l_ticks %d", ws2812_t0l_ticks);
    // ESP_LOGV(TAG, "ws2812_t1h_ticks %d", ws2812_t1h_ticks);
    // ESP_LOGV(TAG, "ws2812_t1l_ticks %d", ws2812_t1l_ticks);
    // ESP_LOGV(TAG, "ws2812_reset_ticks %d", ws2812_reset_ticks);
  }

  uint8_t Show(Color* buffer, uint8_t brightness) {
    // ESP_LOGV(TAG, "Show");
    uint32_t status;
    rmt_get_status(rmt_channel, &status);
    if (status & 0x003ff000)  // RMT_MEM_RADDR_EX
    {
      // ESP_LOGI(TAG, "transmit still in progress, abort");
      return -1;
    }
    setup_rmt_data_buffer(buffer, brightness);
    ESP_ERROR_CHECK(rmt_write_items(rmt_channel, rmtBuffer, numsOfLED * BITS_PER_LED_CMD + 1, true));  // Block the
                                                                                                       // thread because
                                                                                                       // FreeRTOS is
                                                                                                       // gonna handle
                                                                                                       // it
    // ESP_ERROR_CHECK(rmt_wait_tx_done(LED_RMT_TX_CHANNEL, portMAX_DELAY));
    return 0;
  }

  void setup_rmt_data_buffer(Color* buffer, uint8_t brightness) {
    const rmt_item32_t bit0 = {{{ws2812_t0h_ticks, 1, ws2812_t0l_ticks, 0}}};  // Logical 0
    const rmt_item32_t bit1 = {{{ws2812_t1h_ticks, 1, ws2812_t1l_ticks, 0}}};  // Logical 1
    const rmt_item32_t reset = {{{ws2812_reset_ticks, 0, 0, 0}}};              // Reset
    uint16_t buffer_index = 0;
    for (uint8_t chunk_index = 0; chunk_index < chunkCount; chunk_index++)
    {
      uint8_t local_brightness =
          std::clamp<uint16_t>(brightness * chunkInfo[chunk_index].brightness_multiplier, 0, 255);
      for (uint16_t led = 0; led < chunkInfo[chunk_index].length; led++)
      {
        uint32_t bits_to_send = buffer[buffer_index].GRB(local_brightness);
        uint32_t mask = 1 << (BITS_PER_LED_CMD - 1);
        for (uint32_t bit = 0; bit < BITS_PER_LED_CMD; bit++)
        {
          uint32_t bit_is_set = bits_to_send & mask;
          rmtBuffer[buffer_index * BITS_PER_LED_CMD + bit] = bit_is_set ? bit1 : bit0;
          mask >>= 1;
        }
        buffer_index++;
      }
    }
    rmtBuffer[numsOfLED * BITS_PER_LED_CMD] = reset;
  }

  // void rmt_callback(rmt_channel_t rmt_channel, void *arg)
  // {
  //     // ESP_LOGI(TAG, "Transmit finished");
  //     transmit_in_progress = false;
  // }
}