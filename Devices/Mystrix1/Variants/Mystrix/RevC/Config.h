namespace REVC
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
inline void LoadRevC() {
  ESP_LOGI("Device Init", "Mystrix Rev C Config Loaded");
  LED::led_pin = REVC::LED_PIN;

  KeyPad::fn_pin = REVC::FN_PIN;

  gpio_num_t _keypad_write_pins[X_SIZE] = {
      REVC::KEY1_PIN, REVC::KEY2_PIN, REVC::KEY3_PIN, REVC::KEY4_PIN,
      REVC::KEY5_PIN, REVC::KEY6_PIN, REVC::KEY7_PIN, REVC::KEY8_PIN,
  };
  memcpy(KeyPad::keypad_write_pins, _keypad_write_pins, sizeof(_keypad_write_pins));

  gpio_num_t _keypad_read_pins[Y_SIZE] = {
      REVC::KEYREAD1_PIN, REVC::KEYREAD2_PIN, REVC::KEYREAD3_PIN, REVC::KEYREAD4_PIN,
      REVC::KEYREAD5_PIN, REVC::KEYREAD6_PIN, REVC::KEYREAD7_PIN, REVC::KEYREAD8_PIN,
  };
  memcpy(KeyPad::keypad_read_pins, _keypad_read_pins, sizeof(_keypad_read_pins));

  adc_channel_t _keypad_read_adc_channel[Y_SIZE] = {
      REVC::KEYREAD1_ADC_CHANNEL, REVC::KEYREAD2_ADC_CHANNEL, REVC::KEYREAD3_ADC_CHANNEL, REVC::KEYREAD4_ADC_CHANNEL,
      REVC::KEYREAD5_ADC_CHANNEL, REVC::KEYREAD6_ADC_CHANNEL, REVC::KEYREAD7_ADC_CHANNEL, REVC::KEYREAD8_ADC_CHANNEL,
  };
  memcpy(KeyPad::keypad_read_adc_channel, _keypad_read_adc_channel, sizeof(_keypad_read_adc_channel));

  KeyPad::touch_data_pin = REVC::TOUCH_DATA_PIN;
  KeyPad::touch_clock_pin = REVC::TOUCH_CLOCK_PIN;
  uint8_t _touchbar_map[16] = {4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8, 0, 1, 2, 3};
  memcpy(KeyPad::touchbar_map, _touchbar_map, sizeof(_touchbar_map) * sizeof(_touchbar_map[0]));

  HWMidi::tx_gpio = REVC::POWER_CORD_PIN;

  Storage::sd_clk_pin = REVC::SD_CLK_PIN;
  Storage::sd_cmd_pin = REVC::SD_CMD_PIN;
  Storage::sd_d0_pin = REVC::SD_D0_PIN;
  Storage::sd_d1_pin = REVC::SD_D1_PIN;
  Storage::sd_d2_pin = REVC::SD_D2_PIN;
  Storage::sd_d3_pin = REVC::SD_D3_PIN;
  Storage::sd_det_pin = REVC::SD_DET_PIN;
  Storage::sd_4bit_mode = REVC::SD_4BIT_MODE;
  Storage::sd_freq_khz = REVC::SD_FREQ_KHZ;
}
}
