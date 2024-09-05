namespace EVT1
{
  const gpio_num_t LED_Pin = GPIO_NUM_46;
  const gpio_num_t LED_EN_Pin = GPIO_NUM_35;

  const gpio_num_t Key1_Pin = GPIO_NUM_2;
  const gpio_num_t Key2_Pin = GPIO_NUM_3;
  const gpio_num_t Key3_Pin = GPIO_NUM_4;
  const gpio_num_t Key4_Pin = GPIO_NUM_16;
  const gpio_num_t Key5_Pin = GPIO_NUM_15;
  const gpio_num_t Key6_Pin = GPIO_NUM_14;
  const gpio_num_t Key7_Pin = GPIO_NUM_13;

  const gpio_num_t KeyRead1_Pin = GPIO_NUM_8;
  const gpio_num_t KeyRead2_Pin = GPIO_NUM_7;
  const gpio_num_t KeyRead3_Pin = GPIO_NUM_10;
  const gpio_num_t KeyRead4_Pin = GPIO_NUM_9;
  const gpio_num_t KeyRead5_Pin = GPIO_NUM_12;
  const gpio_num_t KeyRead6_Pin = GPIO_NUM_11;
  const gpio_num_t KeyRead7_Pin = GPIO_NUM_18;
  const gpio_num_t KeyRead8_Pin = GPIO_NUM_21;
  const gpio_num_t KeyRead9_Pin = GPIO_NUM_1;
  const gpio_num_t KeyRead10_Pin = GPIO_NUM_0;

  const gpio_num_t PowerCord_Pin = GPIO_NUM_26;

  const gpio_num_t PMIC_INT_Pin = GPIO_NUM_17;

  const gpio_num_t I2C_SDA_Pin = GPIO_NUM_38;
  const gpio_num_t I2C_SCL_Pin = GPIO_NUM_45;

#if FACTORY_CONFIG == EVT1
  const DeviceInfo deviceInfo{{'T', 'P', '6', '0'}, {'E', 'V', 'T', '1'}, 0, 0};
#endif
}

void Device::LoadEVT1() {
  ESP_LOGI("Device Init", "Type60 EVT1 Config Loaded");
  led_pin = EVT1::LED_Pin;
  led_en_pin = EVT1::LED_EN_Pin;

  gpio_num_t _keypad_write_pins[7] = {
      EVT1::Key1_Pin, EVT1::Key2_Pin, EVT1::Key3_Pin, EVT1::Key4_Pin,
      EVT1::Key5_Pin, EVT1::Key6_Pin, EVT1::Key7_Pin
  };
  memcpy(KeyPad::keypad_write_pins, _keypad_write_pins, sizeof(_keypad_write_pins));

  gpio_num_t _keypad_read_pins[10] = {
      EVT1::KeyRead1_Pin, EVT1::KeyRead2_Pin, EVT1::KeyRead3_Pin, EVT1::KeyRead4_Pin,
      EVT1::KeyRead5_Pin, EVT1::KeyRead6_Pin, EVT1::KeyRead7_Pin, EVT1::KeyRead8_Pin,
      EVT1::KeyRead9_Pin, EVT1::KeyRead10_Pin
  };
  memcpy(KeyPad::keypad_read_pins, _keypad_read_pins, sizeof(_keypad_read_pins));

  };