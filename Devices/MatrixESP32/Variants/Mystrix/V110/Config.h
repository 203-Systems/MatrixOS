namespace V110
{
  const gpio_num_t FN_Pin = GPIO_NUM_16;

  const gpio_num_t LED_Pin = GPIO_NUM_38;

  const gpio_num_t Key1_Pin = GPIO_NUM_21;
  const gpio_num_t Key2_Pin = GPIO_NUM_17;
  const gpio_num_t Key3_Pin = GPIO_NUM_1;
  const gpio_num_t Key4_Pin = GPIO_NUM_6;
  const gpio_num_t Key5_Pin = GPIO_NUM_12;
  const gpio_num_t Key6_Pin = GPIO_NUM_13;
  const gpio_num_t Key7_Pin = GPIO_NUM_14;
  const gpio_num_t Key8_Pin = GPIO_NUM_15;

  const gpio_num_t KeyRead1_Pin = GPIO_NUM_2;
  const gpio_num_t KeyRead2_Pin = GPIO_NUM_3;
  const gpio_num_t KeyRead3_Pin = GPIO_NUM_4;
  const gpio_num_t KeyRead4_Pin = GPIO_NUM_5;
  const gpio_num_t KeyRead5_Pin = GPIO_NUM_7;
  const gpio_num_t KeyRead6_Pin = GPIO_NUM_8;
  const gpio_num_t KeyRead7_Pin = GPIO_NUM_9;
  const gpio_num_t KeyRead8_Pin = GPIO_NUM_10;

  const adc_channel_t KeyRead1_ADC_CHANNEL = ADC_CHANNEL_1;
  const adc_channel_t KeyRead2_ADC_CHANNEL = ADC_CHANNEL_2;
  const adc_channel_t KeyRead3_ADC_CHANNEL = ADC_CHANNEL_3;
  const adc_channel_t KeyRead4_ADC_CHANNEL = ADC_CHANNEL_4;
  const adc_channel_t KeyRead5_ADC_CHANNEL = ADC_CHANNEL_6;
  const adc_channel_t KeyRead6_ADC_CHANNEL = ADC_CHANNEL_7;
  const adc_channel_t KeyRead7_ADC_CHANNEL = ADC_CHANNEL_8;
  const adc_channel_t KeyRead8_ADC_CHANNEL = ADC_CHANNEL_9;

  const gpio_num_t TouchData_Pin = GPIO_NUM_47;
  const gpio_num_t TouchClock_Pin = GPIO_NUM_33;

  const gpio_num_t PowerCord_Pin = GPIO_NUM_18;

  const gpio_num_t PMIC_INT_Pin = GPIO_NUM_11;

  const gpio_num_t Mystrix_Mod_GPIO_Pin = GPIO_NUM_37;

  const gpio_num_t I2C_SDA_Pin = GPIO_NUM_34;
  const gpio_num_t I2C_SCL_Pin = GPIO_NUM_48;

  const gpio_num_t SD_CLK_Pin = GPIO_NUM_41;
  const gpio_num_t SD_CMD_Pin = GPIO_NUM_40;
  const gpio_num_t SD_D0_Pin = GPIO_NUM_42;
  const gpio_num_t SD_D1_Pin = GPIO_NUM_34;
  const gpio_num_t SD_D2_Pin = GPIO_NUM_48;
  const gpio_num_t SD_D3_Pin = GPIO_NUM_39;
  const gpio_num_t SD_DET_Pin = GPIO_NUM_26;
  const bool SD_4bit_mode = true;
  const uint32_t SD_FREQ_KHZ = 20000;
}

namespace Device
{
inline void LoadV110() {
  ESP_LOGI("Device Init", "Mystrix Pro V110 Config Loaded");
  LED::led_pin = V110::LED_Pin;

  KeyPad::fn_pin = V110::FN_Pin;

  gpio_num_t _keypad_write_pins[X_SIZE] = {
      V110::Key1_Pin, V110::Key2_Pin, V110::Key3_Pin, V110::Key4_Pin,
      V110::Key5_Pin, V110::Key6_Pin, V110::Key7_Pin, V110::Key8_Pin,
  };
  memcpy(KeyPad::keypad_write_pins, _keypad_write_pins, sizeof(_keypad_write_pins));

  gpio_num_t _keypad_read_pins[Y_SIZE] = {
      V110::KeyRead1_Pin, V110::KeyRead2_Pin, V110::KeyRead3_Pin, V110::KeyRead4_Pin,
      V110::KeyRead5_Pin, V110::KeyRead6_Pin, V110::KeyRead7_Pin, V110::KeyRead8_Pin,
  };
  memcpy(KeyPad::keypad_read_pins, _keypad_read_pins, sizeof(_keypad_read_pins));

  adc_channel_t _keypad_read_adc_channel[Y_SIZE] = {
      V110::KeyRead1_ADC_CHANNEL, V110::KeyRead2_ADC_CHANNEL, V110::KeyRead3_ADC_CHANNEL, V110::KeyRead4_ADC_CHANNEL,
      V110::KeyRead5_ADC_CHANNEL, V110::KeyRead6_ADC_CHANNEL, V110::KeyRead7_ADC_CHANNEL, V110::KeyRead8_ADC_CHANNEL,
  };
  memcpy(KeyPad::keypad_read_adc_channel, _keypad_read_adc_channel, sizeof(_keypad_read_adc_channel));

  KeyPad::touchData_Pin = V110::TouchData_Pin;
  KeyPad::touchClock_Pin = V110::TouchClock_Pin;
  uint8_t _touchbar_map[16] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3};
  memcpy(KeyPad::touchbar_map, _touchbar_map, sizeof(_touchbar_map) * sizeof(_touchbar_map[0]));

  Storage::sd_clk_pin = V110::SD_CLK_Pin;
  Storage::sd_cmd_pin = V110::SD_CMD_Pin;
  Storage::sd_d0_pin = V110::SD_D0_Pin;
  Storage::sd_d1_pin = V110::SD_D1_Pin;
  Storage::sd_d2_pin = V110::SD_D2_Pin;
  Storage::sd_d3_pin = V110::SD_D3_Pin;
  Storage::sd_det_pin = V110::SD_DET_Pin;
  Storage::sd_4bit_mode = V110::SD_4bit_mode;
  Storage::sd_freq_khz = V110::SD_FREQ_KHZ;
}
}