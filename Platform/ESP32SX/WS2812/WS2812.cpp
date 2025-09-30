#include "WS2812.h"

namespace WS2812
{
  uint16_t numsOfLED;

  rmt_channel_handle_t rmt_channel = NULL;
  rmt_encoder_handle_t rmt_encoder = NULL;

  #define RMT_LED_STRIP_RESOLUTION_HZ 10000000

  std::vector<LEDPartition>* partitions;

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

  IRAM_ATTR static esp_err_t rmt_del_led_encoder(rmt_encoder_t *rmt_encoder) {
    rmt_led_encoder_t* led_encoder = __containerof(rmt_encoder, rmt_led_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
  }

  IRAM_ATTR static esp_err_t rmt_led_encoder_reset(rmt_encoder_t *rmt_encoder) {
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

    // different led might have its own timing requirements, following parameter is for LC8812
    rmt_bytes_encoder_config_t bytes_encoder_config = {
      .bit0 = { 3, 1, 9, 0 },
      .bit1 = { 6, 1, 6, 0 },
      .flags = { .msb_first = 1 }
    };

    rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder.bytes_encoder);
    rmt_copy_encoder_config_t copy_encoder_config = {};
    rmt_new_copy_encoder(&copy_encoder_config, &led_encoder.copy_encoder);
    
    led_encoder.reset_code = (rmt_symbol_word_t) { 1000, 0, 0, 0 };

    *ret_encoder = &led_encoder.base;
    return ESP_OK;
  }

  void Init(gpio_num_t gpio_pin, std::vector<LEDPartition>& partitions) {

    WS2812::numsOfLED = 0;
    WS2812::partitions = &partitions;

    for (uint8_t partition_index = 0; partition_index < partitions.size(); partition_index++)
    {
      numsOfLED += partitions[partition_index].size;
    }

    led_data = (uint8_t*)malloc(numsOfLED * 3);
    if (led_data == NULL) {
      ESP_LOGE("WS2812", "Failed to allocate led_data memory");
      return;
    }

    dither_error = (uint8_t*)malloc(numsOfLED * 3 * sizeof(uint8_t));
    if (dither_error == NULL) {
      ESP_LOGE("WS2812", "Failed to allocate dither_error memory");
      free(led_data);
      led_data = NULL;
      return;
    }

    for (uint16_t i = 0; i < numsOfLED * 3; i++)
    {
      dither_error[i] = esp_random() & 0x7F; // Limit the random error to 8 bits
    }

    rmt_tx_channel_config_t rmt_channel_config = {
        .gpio_num = gpio_pin,            // GPIO number
        .clk_src = RMT_CLK_SRC_DEFAULT,  // select source clock
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,       // 10 MHz tick resolution, i.e., 1 tick = 0.1 µs
        .mem_block_symbols = 256,        // memory block size, 256 * 4 = 1024 Bytes
        .trans_queue_depth = 4,          // set the number of transactions that can be pending in the background
        .intr_priority = 99,
        .flags = {.invert_out = 0, .with_dma = 1, .io_loop_back = 0, .io_od_mode = 0},
    };

    ESP_ERROR_CHECK(rmt_new_tx_channel(&rmt_channel_config, &rmt_channel));

    ESP_ERROR_CHECK(rmt_new_led_encoder(&rmt_encoder));

    ESP_ERROR_CHECK(rmt_enable(rmt_channel));
  }

  IRAM_ATTR void Show(Color* buffer, std::vector<uint8_t>& brightness) {
    // Safety checks
    if (buffer == NULL || led_data == NULL || WS2812::partitions == NULL) {
      return;
    }

    // Check if RMT transmission is complete, skip frame if busy to prevent blocking
    if (rmt_tx_wait_all_done(rmt_channel, 0) != ESP_OK) {
      // RMT is busy with previous transmission, skip this frame
      return;
    }

    for (uint8_t partition_index = 0; partition_index < WS2812::partitions->size(); partition_index++)
    {
      if (partition_index >= brightness.size()) {
        break; // Avoid accessing brightness array out of bounds
      }

      LEDPartition local_partition = WS2812::partitions->at(partition_index);

      // Bounds check for led_data access
      if (local_partition.start + local_partition.size > numsOfLED) {
        continue;
      }

      if (brightness[partition_index] == 0) {
        memset(led_data + local_partition.start * 3, 0, local_partition.size * 3);
        continue;
      }

      uint8_t local_brightness = brightness[partition_index];

      for (uint16_t i = 0; i < WS2812::partitions->at(partition_index).size; i++)
      {
        uint16_t buffer_index = local_partition.start + i;
        uint16_t data_index_g = buffer_index * 3;
        uint16_t data_index_r = data_index_g + 1;
        uint16_t data_index_b = data_index_g + 2;

        led_data[data_index_g] = Color::scale8_video(buffer[buffer_index].G, local_brightness);
        led_data[data_index_r] = Color::scale8_video(buffer[buffer_index].R, local_brightness);
        led_data[data_index_b] = Color::scale8_video(buffer[buffer_index].B, local_brightness);

        if(dithering && dither_error != NULL) {
          const uint8_t dither_error_threshold = 16;

          // Process all channels in a loop to reduce code duplication
          struct {
            uint8_t original;
            uint16_t data_index;
          } channels[3] = {
            {buffer[buffer_index].G, data_index_g},
            {buffer[buffer_index].R, data_index_r},
            {buffer[buffer_index].B, data_index_b}
          };

          for (int ch = 0; ch < 3; ch++) {
            if(led_data[channels[ch].data_index] >= dithering_threshold)
            {
              // Calculate error more efficiently using 16-bit intermediate
              uint16_t expected = (uint16_t)channels[ch].original * local_brightness;
              uint16_t actual = (uint16_t)led_data[channels[ch].data_index] << 8;

              if (expected > actual) {
                uint8_t new_dither_error = (expected - actual) >> 8; // Convert back to 8-bit
                if (new_dither_error >= dither_error_threshold)
                {
                  dither_error[channels[ch].data_index] += new_dither_error >> 1;
                  if (dither_error[channels[ch].data_index] >= 128)
                  {
                    if (led_data[channels[ch].data_index] < 255) { // Prevent overflow
                      led_data[channels[ch].data_index] += 1;
                    }
                    dither_error[channels[ch].data_index] -= 128;
                  }
                }
              }
            }
          }
        }

      }
    }

    // Transmit LED data with error handling
    esp_err_t ret = rmt_transmit(rmt_channel, rmt_encoder, led_data, numsOfLED * 3, &rmt_config);
    if (ret != ESP_OK) {
      ESP_LOGW("WS2812", "RMT transmission failed: %s", esp_err_to_name(ret));
    }
  }
}