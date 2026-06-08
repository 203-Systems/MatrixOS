#include <stdint.h>
#include "ulp_riscv_utils.h"
#include "ulp_riscv_gpio.h"
#include "ulp_riscv_adc_ulp_core.h"

#define X_SIZE 8
#define Y_SIZE 8

#define HISTORY_SIZE 8
#define IIF_LENGTH 4
#define SETTLING_THRESHOLD 640
#define STABLE_DIFF_THRESHOLD 192
#define SETTLING_DELAY_CYCLES 24

volatile gpio_num_t keypadWritePins[X_SIZE] = {GPIO_NUM_21, GPIO_NUM_17, GPIO_NUM_1,  GPIO_NUM_6,
                                                 GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15};

volatile adc_channel_t keypadReadAdcChannel[Y_SIZE] = {ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4,
                                                          ADC_CHANNEL_6, ADC_CHANNEL_7, ADC_CHANNEL_8, ADC_CHANNEL_9};

volatile uint16_t result[X_SIZE][Y_SIZE];
volatile uint16_t raw_history[HISTORY_SIZE][X_SIZE][Y_SIZE];
volatile uint16_t first_sample[X_SIZE][Y_SIZE];
volatile uint16_t stable_sample[X_SIZE][Y_SIZE];
volatile uint16_t sample_retry_count[X_SIZE][Y_SIZE];

volatile uint16_t history_index;
volatile uint32_t count;

static inline uint16_t scale_reading(uint16_t reading) {
  return (reading << 4) + (reading >> 8);
}

static inline uint16_t absolute_difference(uint16_t left, uint16_t right) {
  return left > right ? (left - right) : (right - left);
}

static inline void settling_delay(void) {
  for (volatile uint32_t cycle = 0; cycle < SETTLING_DELAY_CYCLES; cycle++)
  {
    __asm__ __volatile__("nop");
  }
}

static inline uint16_t sample_stable_reading(adc_channel_t channel, uint8_t x, uint8_t y) {
  uint16_t first = scale_reading(ulp_riscv_adc_read_channel(ADC_UNIT_1, channel));
  uint16_t stable = first;
  uint16_t retries = 0;

  if (first > SETTLING_THRESHOLD)
  {
    settling_delay();
    uint16_t second = scale_reading(ulp_riscv_adc_read_channel(ADC_UNIT_1, channel));
    stable = second;
    retries = 1;

    if (absolute_difference(first, second) > STABLE_DIFF_THRESHOLD)
    {
      settling_delay();
      uint16_t third = scale_reading(ulp_riscv_adc_read_channel(ADC_UNIT_1, channel));
      stable = (second + third) / 2;
      retries = 2;
    }
  }

  first_sample[x][y] = first;
  stable_sample[x][y] = stable;
  sample_retry_count[x][y] = retries;
  return stable;
}

int main(void) {
  count = 0;
  history_index = 0;
  for (uint8_t x = 0; x < X_SIZE; x++)
  {
    ulp_riscv_gpio_init(keypadWritePins[x]);
    ulp_riscv_gpio_output_enable(keypadWritePins[x]);
    ulp_riscv_gpio_set_output_mode(keypadWritePins[x], RTCIO_MODE_OUTPUT);
    ulp_riscv_gpio_output_level(keypadWritePins[x], 0);

    for (uint8_t y = 0; y < Y_SIZE; y++)
    {
      result[x][y] = 0;
      first_sample[x][y] = 0;
      stable_sample[x][y] = 0;
      sample_retry_count[x][y] = 0;
      for (uint8_t history = 0; history < HISTORY_SIZE; history++)
      {
        raw_history[history][x][y] = 0;
      }
    }
  }

  while (true)
  {
    uint16_t frame_index = history_index;
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      ulp_riscv_gpio_output_level(keypadWritePins[x], 1);
      for (uint8_t y = 0; y < Y_SIZE; y++)
      {
        uint16_t reading = sample_stable_reading(keypadReadAdcChannel[y], x, y);

        raw_history[frame_index][x][y] = reading;
        result[x][y] = (result[x][y] * (IIF_LENGTH - 1) + reading) / IIF_LENGTH;
      }
      ulp_riscv_gpio_output_level(keypadWritePins[x], 0);
    }
    history_index = (frame_index + 1) % HISTORY_SIZE;
    count++;
  }
}