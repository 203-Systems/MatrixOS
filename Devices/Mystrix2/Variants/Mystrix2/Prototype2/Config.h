namespace PT2
{
  const gpio_num_t FN_PIN = GPIO_NUM_18;

  const gpio_num_t LED_PIN = GPIO_NUM_34;

  const gpio_num_t PMIC_INT_PIN = GPIO_NUM_9;

  const gpio_num_t I2C_SDA_PIN = GPIO_NUM_5;
  const gpio_num_t I2C_SCL_PIN = GPIO_NUM_6;

  const gpio_num_t KEYPAD_TX_PIN = GPIO_NUM_16; //TX from ESP32 Side
  const gpio_num_t KEYPAD_RX_PIN = GPIO_NUM_17; //RX from ESP32 Side

  const gpio_num_t SD_CLK_PIN = GPIO_NUM_33;
  const gpio_num_t SD_CMD_PIN = GPIO_NUM_47;
  const gpio_num_t SD_D0_PIN = GPIO_NUM_34;
  const gpio_num_t SD_D1_PIN = GPIO_NUM_48;
  const gpio_num_t SD_D2_PIN = GPIO_NUM_21;
  const gpio_num_t SD_D3_PIN = GPIO_NUM_26;
  const gpio_num_t SD_DET_PIN = GPIO_NUM_10;
  const bool SD_4BIT_MODE = true;
  const uint32_t SD_FREQ_KHZ = 40000;
}

namespace Device
{
inline void LoadPT2() {
  ESP_LOGI("Device Init", "Mystrix 2 PT2 Config Loaded");
  LED::led_pin = PT2::LED_PIN;

  KeyPad::fn_pin = PT2::FN_PIN;

  uint8_t _touchbar_map[16] = {4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8, 0, 1, 2, 3};
  memcpy(KeyPad::touchbar_map, _touchbar_map, sizeof(_touchbar_map) * sizeof(_touchbar_map[0]));
  i2c_sda_pin = PT2::I2C_SDA_PIN;
  i2c_scl_pin = PT2::I2C_SCL_PIN;

  Storage::sd_clk_pin = PT2::SD_CLK_PIN;
  Storage::sd_cmd_pin = PT2::SD_CMD_PIN;
  Storage::sd_d0_pin = PT2::SD_D0_PIN;
  Storage::sd_d1_pin = PT2::SD_D1_PIN;
  Storage::sd_d2_pin = PT2::SD_D2_PIN;
  Storage::sd_d3_pin = PT2::SD_D3_PIN;
  Storage::sd_det_pin = PT2::SD_DET_PIN;
  Storage::sd_4bit_mode = PT2::SD_4BIT_MODE;
  Storage::sd_freq_khz = PT2::SD_FREQ_KHZ;
}
}
