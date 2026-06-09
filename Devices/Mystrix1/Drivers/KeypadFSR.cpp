#include <algorithm>

#include "Device.h"
#include "MatrixOS.h"

#include "esp_adc/adc_oneshot.h"

#include "ulp_fsr_keypad.h"
#include "ulp_riscv.h"

#include "esp_private/adc_share_hw_ctrl.h"
#include "esp_private/esp_sleep_internal.h"

#define VELOCITY_SENSITIVE_KEYPAD_ADC_ATTEN ADC_ATTEN_DB_12
#define VELOCITY_SENSITIVE_KEYPAD_ADC_WIDTH ADC_BITWIDTH_12
#define ULP_HISTORY_SIZE 8

inline constexpr uint8_t velocityRegressionLaneSamples = 4;
inline constexpr uint8_t velocityRegressionTotalSamples = velocityRegressionLaneSamples * 2;

static_assert(ULP_HISTORY_SIZE >= velocityRegressionTotalSamples, "ULP history must fit the velocity regression window");

extern const uint8_t ulp_fsr_keypad_bin_start[] asm("_binary_ulp_fsr_keypad_bin_start");
extern const uint8_t ulp_fsr_keypad_bin_end[] asm("_binary_ulp_fsr_keypad_bin_end");

#define FORCE_CALIBRATION_LOW_HASH StaticHash("MATRIX—FORCE-CALIBRATION-LOW")
#define FORCE_CALIBRATION_HIGH_HASH StaticHash("MATRIX—FORCE-CALIBRATION-HIGH")

namespace Device::KeyPad::FSR
{
enum class VelocityResponse : int8_t {
  VerySoft = -10,
  Soft = -5,
  Balanced = 0,
  Hard = 5,
  VeryHard = 10,
};

enum class PressureCurve : int8_t {
  VerySlowRise = -10,
  SlowRise = -5,
  Linear = 0,
  FastRise = 5,
  VeryFastRise = 10,
};

struct UserKeypadConfig {
  VelocityResponse velocityResponse;
  PressureCurve pressureCurve;
  uint8_t maxPressurePercentage;
};

enum class FSRRuntimeState : uint8_t {
  Idle,
  DebouncingPress,
  Active,
  DebouncingRelease,
};

struct FSRKeyRuntime {
  FSRRuntimeState state = FSRRuntimeState::Idle;
  uint32_t stateStartMs = 0;
  uint16_t lastCapturedHistoryIndex = 0;
  uint32_t lastCapturedUlpCount = 0;
  uint8_t velocitySampleCount = 0;
  uint16_t velocitySamples[velocityRegressionTotalSamples] = {};
  Fract16 strikeVelocity = 0;
};

inline constexpr UserKeypadConfig defaultUserKeypadConfig = {
    .velocityResponse = VelocityResponse::Balanced,
    .pressureCurve = PressureCurve::Linear,
    .maxPressurePercentage = 100,
};

Fract16 (*lowThresholds)[X_SIZE][Y_SIZE] = nullptr;
Fract16 (*highThresholds)[X_SIZE][Y_SIZE] = nullptr;
FSRKeyRuntime keypadRuntime[X_SIZE][Y_SIZE] = {};

CreateSavedVar("ForceCalibration", lowOffset, int16_t, 0);
CreateSavedVar("ForceCalibration", highOffset, int16_t, 0);

uint16_t GetHistoryIndex();
uint16_t GetHistoryReading(uint8_t historyIndex, uint8_t x, uint8_t y);
uint32_t GetScanCount();

inline uint32_t Fract16Multiply(uint16_t left, uint16_t right) {
  return ((uint32_t)left * (uint32_t)right) / UINT16_MAX;
}

inline Fract16 ApplyPressureCurvePreset(Fract16 normalized, PressureCurve curve) {
  uint16_t x = (uint16_t)normalized;
  int8_t curveAmount = static_cast<int8_t>(curve);

  if (curveAmount == 0)
  {
    return normalized;
  }

  uint32_t squared = Fract16Multiply(x, x);
  uint32_t fastRise = std::min<uint32_t>((uint32_t)x + (uint32_t)x - squared, UINT16_MAX);
  uint32_t target = curveAmount > 0 ? fastRise : squared;
  uint32_t weight = std::min<uint32_t>(curveAmount < 0 ? -curveAmount : curveAmount, 10);
  uint32_t mixed = (((uint32_t)x * (10 - weight)) + (target * weight)) / 10;
  return Fract16((uint16_t)std::min<uint32_t>(mixed, UINT16_MAX));
}

inline uint32_t CalculatePressureFullScaleRange(uint32_t calibratedRange, uint8_t maxPressurePercentage) {
  uint32_t percentage = maxPressurePercentage == 0 ? 100 : maxPressurePercentage;
  return (calibratedRange * percentage) / 100;
}

inline Fract16 EnsureNoteVelocity(Fract16 velocity) {
  if ((uint16_t)velocity < (1u << 9))
  {
    return Fract16(1, 7);
  }
  return velocity;
}

inline uint16_t GetLatestHistoryIndex() {
  return (GetHistoryIndex() + ULP_HISTORY_SIZE - 1) % ULP_HISTORY_SIZE;
}

inline uint16_t GetPreviousHistoryIndex(uint16_t historyIndex) {
  return (historyIndex + ULP_HISTORY_SIZE - 1) % ULP_HISTORY_SIZE;
}

inline uint32_t SumOneToN(uint8_t count) {
  return ((uint32_t)count * (uint32_t)(count + 1)) / 2;
}

inline uint32_t SumSquaresOneToN(uint8_t count) {
  return ((uint32_t)count * (uint32_t)(count + 1) * (uint32_t)((count * 2) + 1)) / 6;
}

inline Fract16 NormalizeVelocitySlope(uint32_t regressionSlope, VelocityResponse response) {
  uint32_t fullScaleSlope;
  switch (response)
  {
    case VelocityResponse::VerySoft:
      fullScaleSlope = 2304;
      break;
    case VelocityResponse::Soft:
      fullScaleSlope = 2816;
      break;
    case VelocityResponse::Hard:
      fullScaleSlope = 5376;
      break;
    case VelocityResponse::VeryHard:
      fullScaleSlope = 6656;
      break;
    case VelocityResponse::Balanced:
    default:
      fullScaleSlope = 4096;
      break;
  }

  uint32_t normalized = std::min<uint32_t>((regressionSlope * UINT16_MAX) / fullScaleSlope, UINT16_MAX);
  return Fract16((uint16_t)normalized);
}

inline uint32_t CalculateRegressionSlope(uint32_t sumY, uint32_t sumXY, uint8_t sampleCount) {
  if (sampleCount == 0)
  {
    return 0;
  }

  uint32_t n = (uint32_t)sampleCount + 1;
  uint32_t sumX = SumOneToN(sampleCount);
  uint32_t sumXSquared = SumSquaresOneToN(sampleCount);
  uint32_t denominator = (n * sumXSquared) - (sumX * sumX);

  if (denominator == 0)
  {
    return sumY;
  }

  int64_t numerator = ((int64_t)n * (int64_t)sumXY) - ((int64_t)sumX * (int64_t)sumY);
  if (numerator <= 0)
  {
    return 0;
  }

  return (uint32_t)(numerator / denominator);
}

inline Fract16 CalculatePressure(KeypadConfig& config, Fract16 filteredReading) {
  if (!config.applyCurve)
  {
    return filteredReading;
  }

  uint16_t reading = (uint16_t)filteredReading;
  uint16_t lowThreshold = (uint16_t)config.lowThreshold;
  uint16_t highThreshold = (uint16_t)config.highThreshold;

  if (reading <= lowThreshold)
  {
    return 0;
  }

  uint32_t calibratedRange = highThreshold > lowThreshold ? (uint32_t)(highThreshold - lowThreshold) : 1;
  uint32_t fullScaleRange = std::max<uint32_t>(CalculatePressureFullScaleRange(calibratedRange, defaultUserKeypadConfig.maxPressurePercentage), 1);
  uint32_t usablePressure = (uint32_t)reading - lowThreshold;
  uint32_t normalized = std::min<uint32_t>((usablePressure * UINT16_MAX) / fullScaleRange, UINT16_MAX);

  return ApplyPressureCurvePreset(Fract16((uint16_t)normalized), defaultUserKeypadConfig.pressureCurve);
}

inline void ResetVelocityCapture(FSRKeyRuntime& runtime) {
  runtime.lastCapturedHistoryIndex = 0;
  runtime.lastCapturedUlpCount = 0;
  runtime.velocitySampleCount = 0;
}

inline void AppendVelocitySample(FSRKeyRuntime& runtime, uint16_t reading) {
  if (runtime.velocitySampleCount >= velocityRegressionTotalSamples)
  {
    return;
  }

  runtime.velocitySamples[runtime.velocitySampleCount++] = reading;
}

void CaptureNewVelocitySamples(FSRKeyRuntime& runtime, uint8_t x, uint8_t y) {
  uint32_t currentUlpCount = GetScanCount();
  uint32_t deltaCount = currentUlpCount - runtime.lastCapturedUlpCount;

  if (deltaCount == 0 || runtime.velocitySampleCount >= velocityRegressionTotalSamples)
  {
    return;
  }

  uint32_t framesToCapture = std::min<uint32_t>(deltaCount, ULP_HISTORY_SIZE);
  uint16_t historyIndex = deltaCount >= ULP_HISTORY_SIZE
                              ? (GetHistoryIndex() + ULP_HISTORY_SIZE - framesToCapture) % ULP_HISTORY_SIZE
                              : (runtime.lastCapturedHistoryIndex + 1) % ULP_HISTORY_SIZE;

  for (uint32_t frame = 0; frame < framesToCapture && runtime.velocitySampleCount < velocityRegressionTotalSamples; frame++)
  {
    AppendVelocitySample(runtime, GetHistoryReading(historyIndex, x, y));
    runtime.lastCapturedHistoryIndex = historyIndex;
    historyIndex = (historyIndex + 1) % ULP_HISTORY_SIZE;
  }

  runtime.lastCapturedUlpCount = currentUlpCount;
}

void StartVelocityCapture(FSRKeyRuntime& runtime, uint8_t x, uint8_t y) {
  ResetVelocityCapture(runtime);

  uint16_t latestHistoryIndex = GetLatestHistoryIndex();
  uint16_t previousHistoryIndex = GetPreviousHistoryIndex(latestHistoryIndex);

  runtime.lastCapturedHistoryIndex = latestHistoryIndex;
  runtime.lastCapturedUlpCount = GetScanCount();

  AppendVelocitySample(runtime, GetHistoryReading(previousHistoryIndex, x, y));
  if (latestHistoryIndex != previousHistoryIndex)
  {
    AppendVelocitySample(runtime, GetHistoryReading(latestHistoryIndex, x, y));
  }
}

Fract16 CalculateEdgeVelocity(const FSRKeyRuntime& runtime, bool risingEdge) {
  if (runtime.velocitySampleCount == 0)
  {
    return 0;
  }

  uint16_t baseline = runtime.velocitySamples[0];
  uint8_t lane1Count = 0;
  uint8_t lane2Count = 0;
  uint32_t lane1SumY = 0;
  uint32_t lane1SumXY = 0;
  uint32_t lane2SumY = 0;
  uint32_t lane2SumXY = 0;

  for (uint8_t sampleIndex = 0; sampleIndex < runtime.velocitySampleCount; sampleIndex++)
  {
    uint16_t reading = runtime.velocitySamples[sampleIndex];
    uint16_t adjustedReading = risingEdge
                                   ? (reading > baseline ? (reading - baseline) : 0)
                                   : (reading < baseline ? (baseline - reading) : 0);
    if ((sampleIndex % 2) == 0)
    {
      lane1Count++;
      lane1SumY += adjustedReading;
      lane1SumXY += (uint32_t)lane1Count * adjustedReading;
    }
    else
    {
      lane2Count++;
      lane2SumY += adjustedReading;
      lane2SumXY += (uint32_t)lane2Count * adjustedReading;
    }
  }

  uint32_t lane1Slope = CalculateRegressionSlope(lane1SumY, lane1SumXY, lane1Count);
  uint32_t lane2Slope = CalculateRegressionSlope(lane2SumY, lane2SumXY, lane2Count);
  uint32_t activeLaneCount = (lane1Count > 0 ? 1u : 0u) + (lane2Count > 0 ? 1u : 0u);
  uint32_t regressionSlope = 0;

  if (activeLaneCount > 0)
  {
    regressionSlope = (lane1Slope + lane2Slope) / activeLaneCount;
  }

  return NormalizeVelocitySlope(regressionSlope, defaultUserKeypadConfig.velocityResponse);
}

Fract16 CalculateStrikeVelocity(const FSRKeyRuntime& runtime) {
  return EnsureNoteVelocity(CalculateEdgeVelocity(runtime, true));
}

Fract16 CalculateReleaseVelocity(const FSRKeyRuntime& runtime) {
  return CalculateEdgeVelocity(runtime, false);
}

void Init() {
  gpio_config_t io_conf;
  adc_oneshot_unit_handle_t adc_handle;

  // Config Input Pins
  adc_oneshot_unit_init_cfg_t initConfig = {
      .unit_id = ADC_UNIT_1,
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
  velocitySensitivity = true;
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

uint16_t GetStableReading(uint8_t x, uint8_t y) {
  uint16_t (*samples)[Y_SIZE] = (uint16_t (*)[Y_SIZE]) & ulp_stable_sample;
  return samples[x][y];
}

uint16_t GetHistoryIndex() {
  return ulp_history_index;
}

uint16_t GetHistoryReading(uint8_t historyIndex, uint8_t x, uint8_t y) {
  if (historyIndex >= ULP_HISTORY_SIZE)
  {
    return 0;
  }

  uint16_t (*history)[ULP_HISTORY_SIZE][X_SIZE][Y_SIZE] = (uint16_t (*)[ULP_HISTORY_SIZE][X_SIZE][Y_SIZE]) & ulp_raw_history;
  return (*history)[historyIndex][x][y];
}

void Start() {
  ulp_riscv_halt();
  ulp_riscv_load_binary(ulp_fsr_keypad_bin_start, (ulp_fsr_keypad_bin_end - ulp_fsr_keypad_bin_start));
  ulp_riscv_run();
}

#define CLAMP(x, low, high) (x < low ? low : (x > high ? high : x))
IRAM_ATTR bool Scan() {
  uint16_t (*result)[Y_SIZE] = (uint16_t (*)[Y_SIZE]) & ulp_result;

  KeypadConfig config = keypadConfig;
  uint32_t timeNow = (uint32_t)MatrixOS::SYS::Millis();

  for (uint8_t y = 0; y < Y_SIZE; y++)
  {
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      Fract16 filteredReading = (Fract16)result[x][y];
      Fract16 stableReading = (Fract16)GetStableReading(x, y);
      int32_t newLowThreshold = (uint16_t)(*lowThresholds)[x][y] + lowOffset.Get();
      int32_t newHighThreshold = (uint16_t)(*highThresholds)[x][y] + highOffset.Get();

      config.lowThreshold = CLAMP(newLowThreshold, 512, UINT16_MAX);
      config.highThreshold = CLAMP(newHighThreshold, 25600, UINT16_MAX);
      uint16_t pressThreshold = (uint16_t)config.lowThreshold + (uint16_t)config.activationOffset;
      bool abovePressThreshold = (uint16_t)stableReading > pressThreshold;
      bool aboveReleaseThreshold = (uint16_t)stableReading > (uint16_t)config.lowThreshold;

      FSRKeyRuntime& runtime = keypadRuntime[x][y];
      KeypadInfo& keyState = keypadState[x][y];
      bool updated = false;

      switch (runtime.state)
      {
        case FSRRuntimeState::Idle:
          if (abovePressThreshold)
          {
            runtime.state = FSRRuntimeState::DebouncingPress;
            runtime.stateStartMs = timeNow;
            StartVelocityCapture(runtime, x, y);
          }
          else
          {
            keyState.UpdateSemantic(false, 0, 0);
            runtime.strikeVelocity = 0;
            ResetVelocityCapture(runtime);
          }
          break;

        case FSRRuntimeState::DebouncingPress:
          if (!abovePressThreshold)
          {
            runtime.state = FSRRuntimeState::Idle;
            ResetVelocityCapture(runtime);
          }
          else
          {
            CaptureNewVelocitySamples(runtime, x, y);
            if (timeNow - runtime.stateStartMs > config.debounce)
            {
              runtime.state = FSRRuntimeState::Active;
              runtime.strikeVelocity = CalculateStrikeVelocity(runtime);
              updated = keyState.UpdateSemantic(true, CalculatePressure(config, filteredReading), runtime.strikeVelocity);
            }
          }
          break;

        case FSRRuntimeState::Active:
          if (!aboveReleaseThreshold)
          {
            StartVelocityCapture(runtime, x, y);
            if (config.debounce > 0)
            {
              runtime.state = FSRRuntimeState::DebouncingRelease;
              runtime.stateStartMs = timeNow;
            }
            else
            {
              Fract16 releaseVelocity = CalculateReleaseVelocity(runtime);
              runtime.state = FSRRuntimeState::Idle;
              updated = keyState.UpdateSemantic(false, 0, releaseVelocity);
              runtime.strikeVelocity = 0;
              ResetVelocityCapture(runtime);
            }
          }
          else
          {
            updated = keyState.UpdateSemantic(true, CalculatePressure(config, filteredReading), runtime.strikeVelocity);
          }
          break;

        case FSRRuntimeState::DebouncingRelease:
          if (aboveReleaseThreshold)
          {
            runtime.state = FSRRuntimeState::Active;
            ResetVelocityCapture(runtime);
          }
          else if (timeNow - runtime.stateStartMs > config.debounce)
          {
            CaptureNewVelocitySamples(runtime, x, y);
            Fract16 releaseVelocity = CalculateReleaseVelocity(runtime);
            runtime.state = FSRRuntimeState::Idle;
            updated = keyState.UpdateSemantic(false, 0, releaseVelocity);
            runtime.strikeVelocity = 0;
            ResetVelocityCapture(runtime);
          }
          else
          {
            CaptureNewVelocitySamples(runtime, x, y);
          }
          break;
      }

      if (updated)
      {
        if (NotifyOS(InputId{1, (uint16_t)(y * X_SIZE + x)}, &keyState))
        {
          return true;
        }
      }
    }
  }
  return false;
}
} // namespace Device::KeyPad::FSR
