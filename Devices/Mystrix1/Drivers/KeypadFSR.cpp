#include "Device.h"

#include "esp_adc/adc_oneshot.h"

#include "ulp_fsr_keypad.h"
#include "ulp_riscv.h"

#include "esp_private/adc_share_hw_ctrl.h"
#include "esp_private/esp_sleep_internal.h"

#define VELOCITY_SENSITIVE_KEYPAD_ADC_ATTEN ADC_ATTEN_DB_12
#define VELOCITY_SENSITIVE_KEYPAD_ADC_WIDTH ADC_BITWIDTH_12

extern const uint8_t ulp_fsr_keypad_bin_start[] asm("_binary_ulp_fsr_keypad_bin_start");
extern const uint8_t ulp_fsr_keypad_bin_end[] asm("_binary_ulp_fsr_keypad_bin_end");

#define FORCE_CALIBRATION_LOW_HASH StaticHash("MATRIX—FORCE-CALIBRATION-LOW")
#define FORCE_CALIBRATION_HIGH_HASH StaticHash("MATRIX—FORCE-CALIBRATION-HIGH")

namespace MatrixOS::USB
{
bool Connected();
}

namespace Device::KeyPad::FSR
{
Fract16 (*lowThresholds)[X_SIZE][Y_SIZE] = nullptr;
Fract16 (*highThresholds)[X_SIZE][Y_SIZE] = nullptr;

CreateSavedVar("ForceCalibration", lowOffset, int16_t, 0);
CreateSavedVar("ForceCalibration", highOffset, int16_t, 0);

void Init() {
  gpio_config_t io_conf;
  adc_oneshot_unit_handle_t adc_handle;

  // Config Input Pins
  adc_oneshot_unit_init_cfg_t initConfig = {
      .unitId = ADC_UNIT_1,
      .ulp_mode = ADC_ULP_MODE_RISCV,
  };
  adc_oneshot_new_unit(&initConfig, &adc_handle);

  adc_oneshot_chan_cfg_t adc_config = {
      .atten = VELOCITY_SENSITIVE_KEYPAD_ADC_ATTEN,
      .bitwidth = VELOCITY_SENSITIVE_KEYPAD_ADC_WIDTH,
  };

  for (uint8_t y = 0; y < Y_SIZE; y++)
  {
    adc_oneshot_config_channel(adc_handle, keypadReadAdcChannel[y], &adc_config);
  }

  // Config Output Pins
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pin_bit_mask = 0;
  for (uint8_t x = 0; x < X_SIZE; x++)
  {
    io_conf.pin_bit_mask |= (1ULL << keypadWritePins[x]);
  }
  gpio_config(&io_conf);

  // Calibrate the ADC
  adc_set_hw_calibration_code(ADC_UNIT_1, VELOCITY_SENSITIVE_KEYPAD_ADC_ATTEN);

  // Enables the use of ADC and temperature sensor in monitor (ULP) mode
  esp_sleep_enable_adc_tsens_monitor(true);

  // Set the threshold
  lowThresholds = (Fract16(*)[X_SIZE][Y_SIZE])pvPortMalloc(sizeof(Fract16) * X_SIZE * Y_SIZE);
  highThresholds = (Fract16(*)[X_SIZE][Y_SIZE])pvPortMalloc(sizeof(Fract16) * X_SIZE * Y_SIZE);

  if (lowThresholds == nullptr || highThresholds == nullptr)
  {
    // Error
  }

  for (uint8_t x = 0; x < X_SIZE; x++)
  {
    for (uint8_t y = 0; y < Y_SIZE; y++)
    {
      (*lowThresholds)[x][y] = keypadConfig.lowThreshold;
      (*highThresholds)[x][y] = keypadConfig.highThreshold;
    }
  }

  MatrixOS::NVS::GetVariable(FORCE_CALIBRATION_LOW_HASH, lowThresholds, sizeof(Fract16) * X_SIZE * Y_SIZE);
  MatrixOS::NVS::GetVariable(FORCE_CALIBRATION_HIGH_HASH, highThresholds, sizeof(Fract16) * X_SIZE * Y_SIZE);
}

void SaveLowCalibration() {
  MatrixOS::NVS::SetVariable(FORCE_CALIBRATION_LOW_HASH, lowThresholds, sizeof(Fract16) * X_SIZE * Y_SIZE);
}

void SaveHighCalibration() {
  MatrixOS::NVS::SetVariable(FORCE_CALIBRATION_HIGH_HASH, highThresholds, sizeof(Fract16) * X_SIZE * Y_SIZE);
}

void ClearLowCalibration() {
  MatrixOS::NVS::DeleteVariable(FORCE_CALIBRATION_LOW_HASH);
  for (uint8_t x = 0; x < X_SIZE; x++)
  {
    for (uint8_t y = 0; y < Y_SIZE; y++)
    {
      (*lowThresholds)[x][y] = keypadConfig.lowThreshold;
    }
  }
}

void ClearHighCalibration() {
  MatrixOS::NVS::DeleteVariable(FORCE_CALIBRATION_HIGH_HASH);
  for (uint8_t x = 0; x < X_SIZE; x++)
  {
    for (uint8_t y = 0; y < Y_SIZE; y++)
    {
      (*highThresholds)[x][y] = keypadConfig.highThreshold;
    }
  }
}

int16_t GetLowOffset() {
  return lowOffset;
}

int16_t GetHighOffset() {
  return highOffset;
}

void SetLowOffset(int16_t offset) {
  lowOffset.Set(offset);
}

void SetHighOffset(int16_t offset) {
  highOffset.Set(offset);
}

uint32_t GetScanCount() {
  return ulp_count;
}

uint16_t GetRawReading(uint8_t x, uint8_t y) {
  uint16_t (*result)[Y_SIZE] = (uint16_t (*)[Y_SIZE]) & ulp_result;
  return result[x][y];
}

void Start() {
  ulp_riscv_halt();
  ulp_riscv_load_binary(ulp_fsr_keypad_bin_start, (ulp_fsr_keypad_bin_end - ulp_fsr_keypad_bin_start));
  ulp_riscv_run();
}

#define CLAMP(x, low, high) (x < low ? low : (x > high ? high : x))
IRAM_ATTR bool Scan() {
  // ESP_LOGI("Keypad ULP", "Scaned: %lu", ulp_count);
  uint16_t (*result)[Y_SIZE] = (uint16_t (*)[Y_SIZE]) & ulp_result;
  // uint16_t(*threshold)[Y_SIZE] = (uint16_t(*)[Y_SIZE]) &ulp_threshold;

  KeyConfig config = keypadConfig;
  for (uint8_t y = 0; y < Y_SIZE; y++)
  {
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      Fract16 reading = (Fract16)result[x][y];
      int32_t newLowThreshold = (uint16_t)(*lowThresholds)[x][y] + lowOffset.Get();
      int32_t newHighThreshold = (uint16_t)(*highThresholds)[x][y] + highOffset.Get();

      config.lowThreshold = CLAMP(newLowThreshold, 512, UINT16_MAX);
      config.highThreshold = CLAMP(newHighThreshold, 25600, UINT16_MAX);
      bool updated = keypadState[x][y].Update(config, reading);
      if (updated)
      {
        uint16_t keyID = (1 << 12) + (x << 6) + y;
        if (NotifyOS(keyID, &keypadState[x][y]))
        {
          return true;
        }
        // ESP_LOGI("Keypad ULP", "Key %d,%d (%d) updated: %d (R:%d, L:%d, H:%d)", x, y, keyID, (uint16_t)keypadState[x][y].velocity,
        // (uint16_t)reading, (uint16_t)config.lowThreshold, (uint16_t)config.highThreshold);
      }
    }
  }
  return false;
}
} // namespace Device::KeyPad::FSR
