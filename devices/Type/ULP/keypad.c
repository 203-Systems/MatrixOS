#include <stdint.h>
#include "ulp_riscv_utils.h"
#include "ulp_riscv_gpio.h"

#define WRITE_SIZE 7
#define READ_SIZE 10

volatile gpio_num_t keypad_write_pins[WRITE_SIZE] = 
{
  GPIO_NUM_2,
  GPIO_NUM_3,
  GPIO_NUM_4,
  GPIO_NUM_16,
  GPIO_NUM_15,
  GPIO_NUM_14,
  GPIO_NUM_13,
};

volatile gpio_num_t keypad_read_pins[READ_SIZE] = 
{
  GPIO_NUM_8,
  GPIO_NUM_7,
  GPIO_NUM_10,
  GPIO_NUM_9,
  GPIO_NUM_12,
  GPIO_NUM_11,
  GPIO_NUM_18,
  GPIO_NUM_21,
  GPIO_NUM_1,
  GPIO_NUM_0
};

volatile uint8_t result[WRITE_SIZE][READ_SIZE];

volatile uint32_t count;

int main(void)
{
  count = 0;
  for (uint8_t x = 0; x < WRITE_SIZE; x++)
  {
    ulp_riscv_gpio_init(keypad_write_pins[x]);
    ulp_riscv_gpio_output_enable(keypad_write_pins[x]);
    ulp_riscv_gpio_set_output_mode(keypad_write_pins[x], RTCIO_MODE_OUTPUT);
    ulp_riscv_gpio_output_level(keypad_write_pins[x], 0);
  }
  while(1)
  {
    for (uint8_t x = 0; x < WRITE_SIZE; x++)
    {
      ulp_riscv_gpio_output_level(keypad_write_pins[x], 1);
      for (uint8_t y = 0; y < READ_SIZE; y++)
      {
        result[x][y] = ulp_riscv_gpio_get_level(keypad_read_pins[y]);
      }
      ulp_riscv_gpio_output_level(keypad_write_pins[x], 0);
    }
    count++;
  }
}