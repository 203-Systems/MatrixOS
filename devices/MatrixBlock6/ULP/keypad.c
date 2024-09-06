#include <stdint.h>
#include "ulp_riscv_utils.h"
#include "ulp_riscv_gpio.h"
#include "ulp_riscv_adc_ulp_core.h"

#define X_SIZE 8
#define Y_SIZE 8

volatile gpio_num_t keypad_write_pins[X_SIZE] = 
{
  GPIO_NUM_21,
  GPIO_NUM_17,
  GPIO_NUM_1,
  GPIO_NUM_6,
  GPIO_NUM_12,
  GPIO_NUM_13,
  GPIO_NUM_14,
  GPIO_NUM_15
};

volatile gpio_num_t keypad_read_pins[Y_SIZE] = 
{
  GPIO_NUM_2,
  GPIO_NUM_3,
  GPIO_NUM_4,
  GPIO_NUM_5,
  GPIO_NUM_7,
  GPIO_NUM_8,
  GPIO_NUM_9,
  GPIO_NUM_10
};

// TODO Add bit map mode
volatile uint8_t result[X_SIZE][Y_SIZE];

volatile uint32_t count;

int main(void)
{
  count = 0;
  for (uint8_t x = 0; x < X_SIZE; x++)
  {
    ulp_riscv_gpio_init(keypad_write_pins[x]);
    ulp_riscv_gpio_output_enable(keypad_write_pins[x]);
    ulp_riscv_gpio_set_output_mode(keypad_write_pins[x], RTCIO_MODE_OUTPUT);
    ulp_riscv_gpio_output_level(keypad_write_pins[x], 0);
  }
  for (uint8_t y = 0; y < Y_SIZE; y++)
  {
    ulp_riscv_gpio_init(keypad_read_pins[y]);
    ulp_riscv_gpio_input_enable(keypad_write_pins[y]);
  }
  while(true)
  {
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      ulp_riscv_gpio_output_level(keypad_write_pins[x], 1);
      for (uint8_t y = 0; y < Y_SIZE; y++)
      {
        result[x][y] = ulp_riscv_gpio_get_level(keypad_read_pins[y]);
      }
      ulp_riscv_gpio_output_level(keypad_write_pins[x], 0);
    }
    count++;
  }
}