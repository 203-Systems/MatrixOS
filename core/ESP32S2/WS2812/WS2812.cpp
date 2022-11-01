#include "WS2812.h"

namespace WS2812
{
  static const char* TAG = "WS2812";

  rmt_item32_t* rmtBuffer;
  // bool transmit_in_progress = false;
  uint16_t numsOfLED;
  rmt_channel_t rmt_channel;
  gpio_num_t gpio_tx;

  static uint32_t ws2812_t0h_ticks = 0;
  static uint32_t ws2812_t1h_ticks = 0;
  static uint32_t ws2812_t0l_ticks = 0;
  static uint32_t ws2812_t1l_ticks = 0;
  static uint32_t ws2812_reset_ticks = 0;

  static rmt_item32_t bit0;
  static rmt_item32_t bit1;
  static rmt_item32_t reset;

  void Init(rmt_channel_t rmt_channel, gpio_num_t gpio_tx, uint16_t numsOfLED) {
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(gpio_tx, rmt_channel);
    // set counter clock to 40MHz
    config.clk_div = 2;
    config.mem_block_num = 3;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    WS2812::rmt_channel = rmt_channel;
    // this.gpio_tx = gpio_tx;
    WS2812::numsOfLED = numsOfLED;

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

    bit0 = {{{ws2812_t0h_ticks, 1, ws2812_t0l_ticks, 0}}};  // Logical 0
    bit1 = {{{ws2812_t1h_ticks, 1, ws2812_t1l_ticks, 0}}};  // Logical 1
    reset = {{{ws2812_reset_ticks, 0, 0, 0}}};              // Reset

    // ESP_LOGV(TAG, "ws2812_t0h_ticks %d", ws2812_t0h_ticks);
    // ESP_LOGV(TAG, "ws2812_t0l_ticks %d", ws2812_t0l_ticks);
    // ESP_LOGV(TAG, "ws2812_t1h_ticks %d", ws2812_t1h_ticks);
    // ESP_LOGV(TAG, "ws2812_t1l_ticks %d", ws2812_t1l_ticks);
    // ESP_LOGV(TAG, "ws2812_reset_ticks %d", ws2812_reset_ticks);
  }

  uint8_t Show(Color* array, uint8_t brightness) {
    // ESP_LOGV(TAG, "Show");
    uint32_t status;
    rmt_get_status(rmt_channel, &status);
    // if(status & 0x003ff000) //RMT_MEM_RADDR_EX
    // {
    //     // ESP_LOGI(TAG, "transmit still in progress, abort");
    //     return -1;
    // }
    setup_rmt_data_buffer(array, brightness);
    ESP_ERROR_CHECK(rmt_write_items(rmt_channel, rmtBuffer, numsOfLED * BITS_PER_LED_CMD + 1, false));
    // ESP_ERROR_CHECK(rmt_wait_tx_done(LED_RMT_TX_CHANNEL, portMAX_DELAY));
    return 0;
  }

  void setup_rmt_data_buffer(Color* array, uint8_t brightness) {
    // rmtBuffer[0] = reset;
    for (uint16_t led = 0; led < numsOfLED; led++)
    {
      // ESP_LOGV("RMT", "LED %d", led);
      uint32_t bits_to_send = array[led].GRB(brightness);
      uint32_t mask = 1 << (BITS_PER_LED_CMD - 1);
      for (uint32_t bit = 0; bit < BITS_PER_LED_CMD; bit++)
      {
        uint32_t bit_is_set = bits_to_send & mask;
        rmtBuffer[led * BITS_PER_LED_CMD + bit] = bit_is_set ? bit1 : bit0;
        mask >>= 1;
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