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

namespace MatrixOS::USB
{
  bool Connected();
}

namespace Device::KeyPad::FSR
{
  adc_oneshot_unit_handle_t adc_handle;
  void Init() {
    gpio_config_t io_conf;

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
  }

  uint16_t middleOfThree(uint16_t a, uint16_t b, uint16_t c) {
    // Checking for a
    if ((b <= a && a <= c) || (c <= a && a <= b))
      return a;

    // Checking for b
    if ((a <= b && b <= c) || (c <= b && b <= a))
      return b;

    return c;
  }

  uint16_t minOfThree(uint16_t a, uint16_t b, uint16_t c) {
    // Checking for a
    if (a < b && a < c)
      return a;

    // Checking for b
    if (b < a && b < c)
      return b;

    return c;
  }

  // uint16_t filter(uint16_t* raw_samples)
  // {
  //   // Remove highest 2 and then average mid

  //   // Copy the array
  //   uint16_t samples[SAMPLES];
  //   for (uint8_t i = 0; i < SAMPLES; i++)
  //   { samples[i] = raw_samples[i]; }

  //   uint8_t maxIndex = 0;
  //   uint8_t minIndex = 0;
  //   for (uint8_t i = 1; i < SAMPLES; i++)
  //   {
  //     if (samples[i] > samples[maxIndex])
  //     { maxIndex = i; }
  //     if (samples[i] < samples[minIndex])
  //     { minIndex = i; }
  //   }

  //   uint32_t sum = 0;
  //   for (uint8_t i = 0; i < SAMPLES; i++)
  //   {
  //     if (i != maxIndex && i != minIndex)
  //     { sum += samples[i]; }
  //   }

  //   return uint16_t(sum / (SAMPLES - 2));
  // }

    uint16_t filter(uint16_t* raw_samples)
  {
    // Max

    // Find max in the array
    // uint16_t max = raw_samples[0];



    // for(uint8_t i = 1; i < SAMPLES; i++)
    // {
    //   if(raw_samples[i] > max)
    //   {
    //     max = raw_samples[i];
    //   }
    // }

    uint16_t sum = 0;
    

    for(uint8_t i = 0; i < SAMPLES; i++)
    {
      sum += raw_samples[i] / SAMPLES;
    }

    // return max;
    return sum;
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
        // if (result[x][y][0] != 0 && result[x][y][1] != 0 && result[x][y][2] != 0)
        // { reading = filter(result[x][y]); }
        reading = result[x][y][0];
        Fract16 read = (reading << 4) + (reading >> 8);  // Raw Voltage mapped. Will add calibration curve later.
        // keypadState[x][y].raw_velocity = read;
        // config.low_threshold = 512;
        // keypadState[x][y].threshold = threshold[x][y];
        bool updated = keypadState[x][y].update(config, read, true);
        // if(MatrixOS::USB::Connected() && x == 5 && y == 5)
        // {
        //   MatrixOS::Logging::LogDebug("Keypad", "%d %d Raw Read: %d  16bit: %d Threshold: %d", x, y, reading, read, threshold[x][y]);
        // }
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