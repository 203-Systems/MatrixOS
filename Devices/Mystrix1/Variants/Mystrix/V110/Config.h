namespace V110
{
  const gpio_num_t FN_PIN = GPIO_NUM_16;

  const gpio_num_t LED_PIN = GPIO_NUM_38;

  const gpio_num_t KEY1_PIN = GPIO_NUM_21;
  const gpio_num_t KEY2_PIN = GPIO_NUM_17;
  const gpio_num_t KEY3_PIN = GPIO_NUM_1;
  const gpio_num_t KEY4_PIN = GPIO_NUM_6;
  const gpio_num_t KEY5_PIN = GPIO_NUM_12;
  const gpio_num_t KEY6_PIN = GPIO_NUM_13;
  const gpio_num_t KEY7_PIN = GPIO_NUM_14;
  const gpio_num_t KEY8_PIN = GPIO_NUM_15;

  const gpio_num_t KEYREAD1_PIN = GPIO_NUM_2;
  const gpio_num_t KEYREAD2_PIN = GPIO_NUM_3;
  const gpio_num_t KEYREAD3_PIN = GPIO_NUM_4;
  const gpio_num_t KEYREAD4_PIN = GPIO_NUM_5;
  const gpio_num_t KEYREAD5_PIN = GPIO_NUM_7;
  const gpio_num_t KEYREAD6_PIN = GPIO_NUM_8;
  const gpio_num_t KEYREAD7_PIN = GPIO_NUM_9;
  const gpio_num_t KEYREAD8_PIN = GPIO_NUM_10;

  const adc_channel_t KEYREAD1_ADC_CHANNEL = ADC_CHANNEL_1;
  const adc_channel_t KEYREAD2_ADC_CHANNEL = ADC_CHANNEL_2;
  const adc_channel_t KEYREAD3_ADC_CHANNEL = ADC_CHANNEL_3;
  const adc_channel_t KEYREAD4_ADC_CHANNEL = ADC_CHANNEL_4;
  const adc_channel_t KEYREAD5_ADC_CHANNEL = ADC_CHANNEL_6;
  const adc_channel_t KEYREAD6_ADC_CHANNEL = ADC_CHANNEL_7;
  const adc_channel_t KEYREAD7_ADC_CHANNEL = ADC_CHANNEL_8;
  const adc_channel_t KEYREAD8_ADC_CHANNEL = ADC_CHANNEL_9;

  const gpio_num_t TOUCH_DATA_PIN = GPIO_NUM_47;
  const gpio_num_t TOUCH_CLOCK_PIN = GPIO_NUM_33;

  const gpio_num_t POWER_CORD_PIN = GPIO_NUM_18;

  const gpio_num_t PMIC_INT_PIN = GPIO_NUM_11;

  const gpio_num_t MYSTRIX_MOD_GPIO_PIN = GPIO_NUM_37;

  const gpio_num_t I2C_SDA_PIN = GPIO_NUM_34;
  const gpio_num_t I2C_SCL_PIN = GPIO_NUM_48;

  const gpio_num_t SD_CLK_PIN = GPIO_NUM_41;
  const gpio_num_t SD_CMD_PIN = GPIO_NUM_40;
  const gpio_num_t SD_D0_PIN = GPIO_NUM_42;
  const gpio_num_t SD_D1_PIN = GPIO_NUM_34;
  const gpio_num_t SD_D2_PIN = GPIO_NUM_48;
  const gpio_num_t SD_D3_PIN = GPIO_NUM_39;
  const gpio_num_t SD_DET_PIN = GPIO_NUM_26;
  const bool SD_4BIT_MODE = true;
  const uint32_t SD_FREQ_KHZ = 20000;
}

namespace Device
{
inline void LoadV110() {
  ESP_LOGI("Device Init", "Mystrix Pro V110 Config Loaded");
  LED::led_pin = V110::LED_PIN;

  KeyPad::fn_pin = V110::FN_PIN;

  gpio_num_t _keypad_write_pins[X_SIZE] = {
      V110::KEY1_PIN, V110::KEY2_PIN, V110::KEY3_PIN, V110::KEY4_PIN,
      V110::KEY5_PIN, V110::KEY6_PIN, V110::KEY7_PIN, V110::KEY8_PIN,
  };
  memcpy(KeyPad::keypad_write_pins, _keypad_write_pins, sizeof(_keypad_write_pins));

  gpio_num_t _keypad_read_pins[Y_SIZE] = {
      V110::KEYREAD1_PIN, V110::KEYREAD2_PIN, V110::KEYREAD3_PIN, V110::KEYREAD4_PIN,
      V110::KEYREAD5_PIN, V110::KEYREAD6_PIN, V110::KEYREAD7_PIN, V110::KEYREAD8_PIN,
  };
  memcpy(KeyPad::keypad_read_pins, _keypad_read_pins, sizeof(_keypad_read_pins));

  adc_channel_t _keypad_read_adc_channel[Y_SIZE] = {
      V110::KEYREAD1_ADC_CHANNEL, V110::KEYREAD2_ADC_CHANNEL, V110::KEYREAD3_ADC_CHANNEL, V110::KEYREAD4_ADC_CHANNEL,
      V110::KEYREAD5_ADC_CHANNEL, V110::KEYREAD6_ADC_CHANNEL, V110::KEYREAD7_ADC_CHANNEL, V110::KEYREAD8_ADC_CHANNEL,
  };
  memcpy(KeyPad::keypad_read_adc_channel, _keypad_read_adc_channel, sizeof(_keypad_read_adc_channel));

  KeyPad::touch_data_pin = V110::TOUCH_DATA_PIN;
  KeyPad::touch_clock_pin = V110::TOUCH_CLOCK_PIN;
  uint8_t _touchbar_map[16] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3};
  memcpy(KeyPad::touchbar_map, _touchbar_map, sizeof(_touchbar_map) * sizeof(_touchbar_map[0]));

  HWMidi::tx_gpio = V110::POWER_CORD_PIN;

  Storage::sd_clk_pin = V110::SD_CLK_PIN;
  Storage::sd_cmd_pin = V110::SD_CMD_PIN;
  Storage::sd_d0_pin = V110::SD_D0_PIN;
  Storage::sd_d1_pin = V110::SD_D1_PIN;
  Storage::sd_d2_pin = V110::SD_D2_PIN;
  Storage::sd_d3_pin = V110::SD_D3_PIN;
  Storage::sd_det_pin = V110::SD_DET_PIN;
  Storage::sd_4bit_mode = V110::SD_4BIT_MODE;
  Storage::sd_freq_khz = V110::SD_FREQ_KHZ;
}
}
