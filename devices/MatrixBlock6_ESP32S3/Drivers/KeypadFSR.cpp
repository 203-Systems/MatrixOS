#include "Device.h"
#include "ulp_fsr_keypad.h"
#include "ulp_riscv.h"

#include "esp_private/adc_share_hw_ctrl.h"
#include "esp_private/esp_sleep_internal.h"

#define VELOCITY_SENSITIVE_KEYPAD_ADC_ATTEN ADC_ATTEN_DB_12
#define VELOCITY_SENSITIVE_KEYPAD_ADC_WIDTH ADC_BITWIDTH_12
#define SAMPLES 1

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
  Fract16 (*low_thresholds)[x_size][y_size] = nullptr;
  Fract16 (*high_thresholds)[x_size][y_size] = nullptr;

  void Init() {
    gpio_config_t io_conf;
    adc_oneshot_unit_handle_t adc_handle;

    // Config Input Pins
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_RISCV,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t adc_config = {
        .atten = VELOCITY_SENSITIVE_KEYPAD_ADC_ATTEN,
        .bitwidth = VELOCITY_SENSITIVE_KEYPAD_ADC_WIDTH,
    };

    for (uint8_t y = 0; y < y_size; y++)
    { adc_oneshot_config_channel(adc_handle, keypad_read_adc_channel[y], &adc_config); }

    // Config Output Pins
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = 0;
    for (uint8_t x = 0; x < x_size; x++)
    { io_conf.pin_bit_mask |= (1ULL << keypad_write_pins[x]); }
    gpio_config(&io_conf);

    // Calibrate the ADC
    adc_set_hw_calibration_code(ADC_UNIT_1, VELOCITY_SENSITIVE_KEYPAD_ADC_ATTEN);

    // Enables the use of ADC and temperature sensor in monitor (ULP) mode
    esp_sleep_enable_adc_tsens_monitor(true);

    // Set the threshold
    low_thresholds = (Fract16 (*)[x_size][y_size])pvPortMalloc(sizeof(Fract16) * x_size * y_size); 
    high_thresholds = (Fract16 (*)[x_size][y_size])pvPortMalloc(sizeof(Fract16) * x_size * y_size);

    if(low_thresholds == nullptr || high_thresholds == nullptr)
    {
      // Error
    }

    for (uint8_t x = 0; x < x_size; x++)
    {
      for (uint8_t y = 0; y < y_size; y++)
      {
        (*low_thresholds)[x][y] = keypad_config.low_threshold;
        (*high_thresholds)[x][y] = keypad_config.high_threshold;
      }
    }

    MatrixOS::NVS::GetVariable(FORCE_CALIBRATION_LOW_HASH, low_thresholds, sizeof(Fract16) * x_size * y_size);
    MatrixOS::NVS::GetVariable(FORCE_CALIBRATION_HIGH_HASH, high_thresholds, sizeof(Fract16) * x_size * y_size);
  }

  void SaveCalibration()
  {
    MatrixOS::NVS::SetVariable(FORCE_CALIBRATION_LOW_HASH, low_thresholds, sizeof(Fract16) * x_size * y_size);
    MatrixOS::NVS::SetVariable(FORCE_CALIBRATION_HIGH_HASH, high_thresholds, sizeof(Fract16) * x_size * y_size);
  }

  void Start() {
    ulp_riscv_halt();
    ulp_riscv_load_binary(ulp_fsr_keypad_bin_start, (ulp_fsr_keypad_bin_end - ulp_fsr_keypad_bin_start));
    ulp_riscv_run();
  }
  

  bool Scan() {
    // ESP_LOGI("Keypad ULP", "Scaned: %lu", ulp_count);
    uint16_t(*result)[8][SAMPLES] = (uint16_t(*)[8][SAMPLES]) &ulp_result;
    // uint16_t(*threshold)[8] = (uint16_t(*)[8]) &ulp_threshold;

    KeyConfig config = keypad_config;
    for (uint8_t y = 0; y < Device::y_size; y++)
    {
      for (uint8_t x = 0; x < Device::x_size; x++)
      {
        uint16_t reading = 0;
        reading = result[x][y][0];
        Fract16 read = (reading << 4) + (reading >> 8);  // Raw Voltage mapped. Will add calibration curve later.
        config.low_threshold = (*low_thresholds)[x][y];
        config.high_threshold = (*high_thresholds)[x][y];
        bool updated = keypadState[x][y].update(config, read, true);
        if (updated)
        {
          uint16_t keyID = (1 << 12) + (x << 6) + y;
          if (NotifyOS(keyID, &keypadState[x][y]))
          { return true; }
        }
      }
    }
    return false;
  }


}