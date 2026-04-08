namespace UPT2
{
const gpio_num_t FN_PIN = GPIO_NUM_18;

const gpio_num_t LED_PIN = GPIO_NUM_38;

const gpio_num_t PMIC_INT_PIN = GPIO_NUM_9;

const gpio_num_t I2C_SDA_PIN = GPIO_NUM_5;
const gpio_num_t I2C_SCL_PIN = GPIO_NUM_6;

const gpio_num_t KEYPAD_TX_PIN = GPIO_NUM_16; // TX from ESP32 Side
const gpio_num_t KEYPAD_RX_PIN = GPIO_NUM_17; // RX from ESP32 Side

const gpio_num_t SD_CLK_PIN = GPIO_NUM_33;
const gpio_num_t SD_CMD_PIN = GPIO_NUM_47;
const gpio_num_t SD_D0_PIN = GPIO_NUM_34;
const gpio_num_t SD_D1_PIN = GPIO_NUM_48;
const gpio_num_t SD_D2_PIN = GPIO_NUM_21;
const gpio_num_t SD_D3_PIN = GPIO_NUM_26;
const gpio_num_t SD_DET_PIN = GPIO_NUM_10;
const bool SD_4BIT_MODE = true;
const uint32_t SD_FREQ_KHZ = 40000;
} // namespace UPT2

namespace Device
{
inline void LoadUPT2() {
  ESP_LOGI("Device Init", "Mystrix 2 Ultra UPT2 Config Loaded");
  LED::ledPin = UPT2::LED_PIN;

  KeyPad::keypadType = KeyPad::KeypadType::MPEKeypad;
  KeyPad::fnPin = UPT2::FN_PIN;
  i2cSdaPin = UPT2::I2C_SDA_PIN;
  i2cSclPin = UPT2::I2C_SCL_PIN;

  Storage::sdClkPin = UPT2::SD_CLK_PIN;
  Storage::sdCmdPin = UPT2::SD_CMD_PIN;
  Storage::sdD0Pin = UPT2::SD_D0_PIN;
  Storage::sdD1Pin = UPT2::SD_D1_PIN;
  Storage::sdD2Pin = UPT2::SD_D2_PIN;
  Storage::sdD3Pin = UPT2::SD_D3_PIN;
  Storage::sdDetPin = UPT2::SD_DET_PIN;
  Storage::sd4BitMode = UPT2::SD_4BIT_MODE;
  Storage::sdFreqKhz = UPT2::SD_FREQ_KHZ;
}
} // namespace Device
