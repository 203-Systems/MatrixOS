namespace REVC
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

  const gpio_num_t Matrix_Mod_GPIO_Pin = GPIO_NUM_37;

  const gpio_num_t I2C_SDA_Pin = GPIO_NUM_34;
  const gpio_num_t I2C_SCL_Pin = GPIO_NUM_48;
}

void Device::LoadRevC() {
  ESP_LOGI("Device Init", "Matrix Rev C Config Loaded");
  led_pin = REVC::LED_Pin;

  KeyPad::fn_pin = REVC::FN_Pin;

  gpio_num_t _keypad_write_pins[8] = {
      REVC::Key1_Pin, REVC::Key2_Pin, REVC::Key3_Pin, REVC::Key4_Pin,
      REVC::Key5_Pin, REVC::Key6_Pin, REVC::Key7_Pin, REVC::Key8_Pin,
  };
  memcpy(KeyPad::keypad_write_pins, _keypad_write_pins, sizeof(_keypad_write_pins));

  gpio_num_t _keypad_read_pins[8] = {
      REVC::KeyRead1_Pin, REVC::KeyRead2_Pin, REVC::KeyRead3_Pin, REVC::KeyRead4_Pin,
      REVC::KeyRead5_Pin, REVC::KeyRead6_Pin, REVC::KeyRead7_Pin, REVC::KeyRead8_Pin,
  };
  memcpy(KeyPad::keypad_read_pins, _keypad_read_pins, sizeof(_keypad_read_pins));

  adc_channel_t _keypad_read_adc_channel[8] = {
      REVC::KeyRead1_ADC_CHANNEL, REVC::KeyRead2_ADC_CHANNEL, REVC::KeyRead3_ADC_CHANNEL, REVC::KeyRead4_ADC_CHANNEL,
      REVC::KeyRead5_ADC_CHANNEL, REVC::KeyRead6_ADC_CHANNEL, REVC::KeyRead7_ADC_CHANNEL, REVC::KeyRead8_ADC_CHANNEL,
  };
  memcpy(KeyPad::keypad_read_adc_channel, _keypad_read_adc_channel, sizeof(_keypad_read_adc_channel));

  KeyPad::touchData_Pin = REVC::TouchData_Pin;
  KeyPad::touchClock_Pin = REVC::TouchClock_Pin;
  uint8_t _touchbar_map[16] = {4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8, 0, 1, 2, 3};
  memcpy(KeyPad::touchbar_map, _touchbar_map, sizeof(_touchbar_map) * sizeof(_touchbar_map[0]));
};