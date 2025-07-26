#include "WS2812.h"

namespace WS2812
{
  uint16_t numsOfLED;

  rmt_channel_handle_t rmt_channel = NULL;
  rmt_encoder_handle_t rmt_encoder = NULL;

  #define RMT_LED_STRIP_RESOLUTION_HZ 10000000

  std::vector<LEDPartition>* led_partitions;

  uint8_t* dither_error;
  uint8_t* led_data;

  typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t* bytes_encoder;
    rmt_encoder_t* copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
  } rmt_led_encoder_t;

  rmt_led_encoder_t led_encoder;

  rmt_transmit_config_t rmt_config = {
      .loop_count = 0,  // no transfer loop
      .flags =
          {
              .eot_level = 0,
              .queue_nonblocking = 0,
          },
  };

  IRAM_ATTR static size_t rmt_encode_led(rmt_encoder_t *rmt_encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state) {
    rmt_led_encoder_t* led_encoder = __containerof(rmt_encoder, rmt_led_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;

    size_t encoded_symbols = 0;

    switch (led_encoder->state)
    {
      case 0:  // send RGB data
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);

        if (session_state & RMT_ENCODING_COMPLETE)
        {
          led_encoder->state = 1;  // switch to next state when current encoding session finished
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
          state = (rmt_encode_state_t)(state | (uint32_t)RMT_ENCODING_MEM_FULL);
          goto out;  // yield if there's no free space for encoding artifacts
        }
      // fall-through
      case 1:  // send reset code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code, sizeof(led_encoder->reset_code), &session_state);

        if (session_state & RMT_ENCODING_COMPLETE)
        {
          led_encoder->state = RMT_ENCODING_RESET;  // back to the initial encoding session
          state = (rmt_encode_state_t)(state | (uint32_t)RMT_ENCODING_COMPLETE);
        }
        if (session_state & RMT_ENCODING_MEM_FULL)
        {
          state = (rmt_encode_state_t)(state | (uint32_t)RMT_ENCODING_MEM_FULL);
          goto out;  // yield if there's no free space for encoding artifacts
        }
    }
  out:
    *ret_state = state;
    return encoded_symbols;
  }

  static esp_err_t rmt_del_led_encoder(rmt_encoder_t *rmt_encoder) {
    rmt_led_encoder_t* led_encoder = __containerof(rmt_encoder, rmt_led_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
  }

  static esp_err_t rmt_led_encoder_reset(rmt_encoder_t *rmt_encoder) {
    rmt_led_encoder_t* led_encoder = __containerof(rmt_encoder, rmt_led_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
  }

  esp_err_t rmt_new_led_encoder(rmt_encoder_handle_t *ret_encoder) {
    led_encoder.base.encode = rmt_encode_led;
    led_encoder.base.del = rmt_del_led_encoder;
    led_encoder.base.reset = rmt_led_encoder_reset;

    // different led might have its own timing requirements, following parameter is for WS2812
    rmt_bytes_encoder_config_t bytes_encoder_config = {
      .bit0 = { 4, 1, 10, 0 },
      .bit1 = { 10, 1, 4, 0 },
      .flags = { .msb_first = 1 }
    };

    rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder.bytes_encoder);
    rmt_copy_encoder_config_t copy_encoder_config = {};
    rmt_new_copy_encoder(&copy_encoder_config, &led_encoder.copy_encoder);
    
    led_encoder.reset_code = (rmt_symbol_word_t) { 2800, 0, 0, 0 };

    *ret_encoder = &led_encoder.base;
    return ESP_OK;
  }

  void Init(gpio_num_t gpio_pin, std::vector<LEDPartition>& led_partitions) {

    WS2812::numsOfLED = 0;
    WS2812::led_partitions = &led_partitions;

    for (uint8_t partition_index = 0; partition_index < led_partitions.size(); partition_index++)
    {
      numsOfLED += led_partitions[partition_index].size;
    }

    led_data = (uint8_t*)malloc(numsOfLED * 3);

    dither_error = (uint8_t*)malloc(numsOfLED * 3 * sizeof(uint8_t));
    for (uint16_t i = 0; i < numsOfLED * 3; i++)
    {
      dither_error[i] = esp_random() & 0x7F; // Limit the random error to 8 bits
    }

    rmt_tx_channel_config_t rmt_channel_config = {
        .gpio_num = gpio_pin,            // GPIO number
        .clk_src = RMT_CLK_SRC_DEFAULT,  // select source clock
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,       // 10 MHz tick resolution, i.e., 1 tick = 0.1 µs
        .mem_block_symbols = 64,         // memory block size, 64 * 4 = 256 Bytes
        .trans_queue_depth = 4,          // set the number of transactions that can be pending in the background
        .intr_priority = 99,
        .flags = {.invert_out = 0, .with_dma = 0, .io_loop_back = 0, .io_od_mode = 0},
    };

    ESP_ERROR_CHECK(rmt_new_tx_channel(&rmt_channel_config, &rmt_channel));

    ESP_ERROR_CHECK(rmt_new_led_encoder(&rmt_encoder));

    ESP_ERROR_CHECK(rmt_enable(rmt_channel));
  }

  IRAM_ATTR void Show(Color* buffer, std::vector<uint8_t>& brightness) {
    // rmt_tx_wait_all_done(rmt_channel, portMAX_DELAY);
    // TODO: IF rmt is busy, just skip
    
    for (uint8_t partition_index = 0; partition_index < WS2812::led_partitions->size(); partition_index++)
    {
      LEDPartition local_partition = WS2812::led_partitions->at(partition_index);

      if (brightness[partition_index] == 0 || partition_index >= brightness.size()) {
        memset(led_data + local_partition.start * 3, 0, local_partition.size * 3);
        continue;
      }

      uint8_t local_brightness = brightness[partition_index];

      for (uint16_t i = 0; i < WS2812::led_partitions->at(partition_index).size; i++)
      {
        uint16_t buffer_index = local_partition.start + i;
        uint16_t data_index_g = buffer_index * 3;
        uint16_t data_index_r = data_index_g + 1;
        uint16_t data_index_b = data_index_g + 2;

        led_data[data_index_g] = Color::scale8_video(buffer[buffer_index].G, local_brightness);
        led_data[data_index_r] = Color::scale8_video(buffer[buffer_index].R, local_brightness);
        led_data[data_index_b] = Color::scale8_video(buffer[buffer_index].B, local_brightness);

        if(dithering == false) {
          continue;
        }

        const uint8_t dither_error_threshold = 16;

        if(led_data[data_index_g] >= dithering_threshold)
        {
          uint8_t new_dither_error_g = (buffer[buffer_index].G * local_brightness) - (led_data[data_index_g] << 8);
          if (new_dither_error_g >= dither_error_threshold)
          {
            dither_error[data_index_g] += new_dither_error_g >> 1;
            if (dither_error[data_index_g] >= 128)
            {
              led_data[data_index_g] += 1;
              dither_error[data_index_g] -= 128;
            }
          }
        }

        if(led_data[data_index_r] >= dithering_threshold)
        {
          uint8_t new_dither_error_r = (buffer[buffer_index].R * local_brightness) - (led_data[data_index_r] << 8);
          if (new_dither_error_r >= dither_error_threshold)
          {
            dither_error[data_index_r] += new_dither_error_r >> 1;
            if (dither_error[data_index_r] >= 128)
            {
              led_data[data_index_r] += 1;
              dither_error[data_index_r] -= 128;
            }
          }
        }

        if(led_data[data_index_b] >= dithering_threshold)
        {
          uint8_t new_dither_error_b = (buffer[buffer_index].B * local_brightness) - (led_data[data_index_b] << 8);
          if (new_dither_error_b >= dither_error_threshold)
          {
            dither_error[data_index_b] += new_dither_error_b >> 1;
            if (dither_error[data_index_r] >= 128)
            {
              led_data[data_index_r] += 1;
              dither_error[data_index_r] -= 128;
            }
          }
        }

      }
    }

    rmt_transmit(rmt_channel, rmt_encoder, led_data, numsOfLED * 3, &rmt_config);
  }
}