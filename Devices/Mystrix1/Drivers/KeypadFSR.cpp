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
#define VELOCITY_SLOPE_DENOMINATOR 4096

inline constexpr uint8_t kVelocityRegressionLaneSamples = 4;
inline constexpr uint8_t kVelocityRegressionTotalSamples = kVelocityRegressionLaneSamples * 2;

static_assert(ULP_HISTORY_SIZE >= kVelocityRegressionTotalSamples, "ULP history must fit the velocity regression window");

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
enum class VelocityResponse : uint8_t {
  Soft = 50,
  Balanced = 100,
  Hard = 200,
};

enum class PressureCurve : uint8_t {
  Linear = 10,
  Soft = 20,
  Hard = 30,
  LogFast = 40,
};

struct UserKeypadConfig {
  VelocityResponse velocityResponse;
  PressureCurve pressureCurve;
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
  uint16_t pressHistoryIndex = 0;
  Fract16 strikeVelocity = 0;
};

struct StrikeVelocityDebugInfo {
  uint16_t latestHistoryIndex = 0;
  uint16_t baseline = 0;
  uint16_t peak = 0;
  uint8_t sampleCount = 0;
  uint8_t lane1Count = 0;
  uint8_t lane2Count = 0;
  uint32_t delta = 0;
  uint32_t lane1Slope = 0;
  uint32_t lane2Slope = 0;
  uint32_t regressionSlope = 0;
  uint32_t normalized = 0;
};

inline constexpr UserKeypadConfig kDefaultUserKeypadConfig = {
    .velocityResponse = VelocityResponse::Balanced,
    .pressureCurve = PressureCurve::Linear,
};

inline constexpr uint32_t kUlpScanRateLogIntervalMs = 1000;

Fract16 (*lowThresholds)[X_SIZE][Y_SIZE] = nullptr;
Fract16 (*highThresholds)[X_SIZE][Y_SIZE] = nullptr;
FSRKeyRuntime keypadRuntime[X_SIZE][Y_SIZE] = {};
uint32_t lastUlpScanRateLogMs = 0;
uint32_t lastUlpScanRateCount = 0;

CreateSavedVar("ForceCalibration", lowOffset, int16_t, 0);
CreateSavedVar("ForceCalibration", highOffset, int16_t, 0);

uint16_t GetHistoryIndex();
uint16_t GetHistoryReading(uint8_t historyIndex, uint8_t x, uint8_t y);

inline uint32_t Fract16Multiply(uint16_t left, uint16_t right) {
  return ((uint32_t)left * (uint32_t)right) / UINT16_MAX;
}

inline Fract16 ApplyPressureCurvePreset(Fract16 normalized, PressureCurve curve) {
  uint16_t x = (uint16_t)normalized;
  uint32_t squared = Fract16Multiply(x, x);

  switch (curve)
  {
    case PressureCurve::Soft:
    {
      uint32_t eased = (uint32_t)x + (uint32_t)x - squared;
      return Fract16((uint16_t)std::min<uint32_t>(eased, UINT16_MAX));
    }
    case PressureCurve::Hard:
      return Fract16((uint16_t)squared);
    case PressureCurve::LogFast:
      return Fract16((uint16_t)Fract16Multiply((uint16_t)squared, x));
    case PressureCurve::Linear:
    default:
      return normalized;
  }
}

inline Fract16 ApplyVelocityResponsePreset(Fract16 normalized, VelocityResponse response) {
  uint16_t x = (uint16_t)normalized;
  uint32_t squared = Fract16Multiply(x, x);

  switch (response)
  {
    case VelocityResponse::Soft:
    {
      uint32_t eased = (uint32_t)x + (uint32_t)x - squared;
      return Fract16((uint16_t)std::min<uint32_t>(eased, UINT16_MAX));
    }
    case VelocityResponse::Hard:
      return Fract16((uint16_t)squared);
    case VelocityResponse::Balanced:
    default:
      return normalized;
  }
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

  uint32_t numerator = (n * sumXY) - (sumX * sumY);
  return numerator / denominator;
}

inline Fract16 CalculatePressure(KeypadInfo& key, KeypadConfig& config, Fract16 filteredReading) {
  Fract16 normalized = key.ApplyForceCurve(config, filteredReading);
  return ApplyPressureCurvePreset(normalized, kDefaultUserKeypadConfig.pressureCurve);
}

Fract16 CalculateStrikeVelocity(uint8_t x, uint8_t y, uint16_t startHistoryIndex, StrikeVelocityDebugInfo* debugInfo = nullptr) {
  uint16_t latestHistoryIndex = GetLatestHistoryIndex();
  uint16_t baseline = GetHistoryReading(startHistoryIndex, x, y);
  uint16_t peak = baseline;
  uint8_t sampleCount = 0;
  uint16_t historyIndex = startHistoryIndex;
  uint8_t lane1Count = 0;
  uint8_t lane2Count = 0;
  uint32_t lane1SumY = 0;
  uint32_t lane1SumXY = 0;
  uint32_t lane2SumY = 0;
  uint32_t lane2SumXY = 0;

  while (true)
  {
    uint16_t reading = GetHistoryReading(historyIndex, x, y);
    if (reading > peak)
    {
      peak = reading;
    }

    uint16_t adjustedReading = reading > baseline ? (reading - baseline) : 0;
    if ((sampleCount % 2) == 0)
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

    sampleCount++;

    if (historyIndex == latestHistoryIndex || sampleCount >= kVelocityRegressionTotalSamples)
    {
      break;
    }

    historyIndex = (historyIndex + 1) % ULP_HISTORY_SIZE;
  }

  uint32_t delta = peak > baseline ? (peak - baseline) : 0;
  uint32_t lane1Slope = CalculateRegressionSlope(lane1SumY, lane1SumXY, lane1Count);
  uint32_t lane2Slope = CalculateRegressionSlope(lane2SumY, lane2SumXY, lane2Count);
  uint32_t activeLaneCount = (lane1Count > 0 ? 1u : 0u) + (lane2Count > 0 ? 1u : 0u);
  uint32_t regressionSlope = 0;

  if (activeLaneCount > 0)
  {
    regressionSlope = (lane1Slope + lane2Slope) / activeLaneCount;
  }

  uint32_t normalized = std::min<uint32_t>((regressionSlope * UINT16_MAX) / VELOCITY_SLOPE_DENOMINATOR, UINT16_MAX);

  if (debugInfo != nullptr)
  {
    debugInfo->latestHistoryIndex = latestHistoryIndex;
    debugInfo->baseline = baseline;
    debugInfo->peak = peak;
    debugInfo->sampleCount = sampleCount;
    debugInfo->lane1Count = lane1Count;
    debugInfo->lane2Count = lane2Count;
    debugInfo->delta = delta;
    debugInfo->lane1Slope = lane1Slope;
    debugInfo->lane2Slope = lane2Slope;
    debugInfo->regressionSlope = regressionSlope;
    debugInfo->normalized = normalized;
  }

  return EnsureNoteVelocity(ApplyVelocityResponsePreset(Fract16((uint16_t)normalized), kDefaultUserKeypadConfig.velocityResponse));
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

uint16_t GetFirstReading(uint8_t x, uint8_t y) {
  uint16_t (*samples)[Y_SIZE] = (uint16_t (*)[Y_SIZE]) & ulp_first_sample;
  return samples[x][y];
}

uint16_t GetStableReading(uint8_t x, uint8_t y) {
  uint16_t (*samples)[Y_SIZE] = (uint16_t (*)[Y_SIZE]) & ulp_stable_sample;
  return samples[x][y];
}

uint16_t GetSampleRetryCount(uint8_t x, uint8_t y) {
  uint16_t (*retries)[Y_SIZE] = (uint16_t (*)[Y_SIZE]) & ulp_sample_retry_count;
  return retries[x][y];
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
  uint32_t scanCount = GetScanCount();

  if (timeNow - lastUlpScanRateLogMs >= kUlpScanRateLogIntervalMs)
  {
    uint32_t elapsedMs = lastUlpScanRateLogMs == 0 ? 0 : (timeNow - lastUlpScanRateLogMs);
    uint32_t deltaCount = scanCount - lastUlpScanRateCount;
    uint32_t scansPerSecond = (elapsedMs > 0) ? ((deltaCount * 1000u) / elapsedMs) : 0;
    MLOGD("KeypadFSR", "ULP scan rate=%lu Hz delta=%lu elapsed=%lu count=%lu", scansPerSecond, deltaCount, elapsedMs, scanCount);
    lastUlpScanRateLogMs = timeNow;
    lastUlpScanRateCount = scanCount;
  }

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
            runtime.pressHistoryIndex = GetPreviousHistoryIndex(GetLatestHistoryIndex());
          }
          break;

        case FSRRuntimeState::DebouncingPress:
          if (!abovePressThreshold)
          {
            runtime.state = FSRRuntimeState::Idle;
          }
          else if (timeNow - runtime.stateStartMs > config.debounce)
          {
            StrikeVelocityDebugInfo strikeDebug;
            runtime.state = FSRRuntimeState::Active;
            runtime.strikeVelocity = CalculateStrikeVelocity(x, y, runtime.pressHistoryIndex, &strikeDebug);
            MLOGD(
                "KeypadFSR",
              "strike x=%u y=%u velocity=%u baseline=%u peak=%u delta=%lu samples=%u lane1=%u lane2=%u slope1=%lu slope2=%lu slope=%lu normalized=%lu startIdx=%u latestIdx=%u filtered=%u stable=%u result=%u retry=%u low=%u press=%u high=%u",
                x,
                y,
                (uint16_t)runtime.strikeVelocity,
                strikeDebug.baseline,
                strikeDebug.peak,
                strikeDebug.delta,
                strikeDebug.sampleCount,
              strikeDebug.lane1Count,
              strikeDebug.lane2Count,
              strikeDebug.lane1Slope,
              strikeDebug.lane2Slope,
              strikeDebug.regressionSlope,
                strikeDebug.normalized,
                runtime.pressHistoryIndex,
                strikeDebug.latestHistoryIndex,
                (uint16_t)filteredReading,
                (uint16_t)stableReading,
                GetRawReading(x, y),
                GetSampleRetryCount(x, y),
                (uint16_t)config.lowThreshold,
                pressThreshold,
                (uint16_t)config.highThreshold);
            updated = keyState.UpdateSemantic(true, CalculatePressure(keyState, config, filteredReading), runtime.strikeVelocity);
          }
          break;

        case FSRRuntimeState::Active:
          if (!aboveReleaseThreshold)
          {
            if (config.debounce > 0)
            {
              runtime.state = FSRRuntimeState::DebouncingRelease;
              runtime.stateStartMs = timeNow;
            }
            else
            {
              runtime.state = FSRRuntimeState::Idle;
              updated = keyState.UpdateSemantic(false, 0, runtime.strikeVelocity);
              runtime.strikeVelocity = 0;
            }
          }
          else
          {
            updated = keyState.UpdateSemantic(true, CalculatePressure(keyState, config, filteredReading), runtime.strikeVelocity);
          }
          break;

        case FSRRuntimeState::DebouncingRelease:
          if (aboveReleaseThreshold)
          {
            runtime.state = FSRRuntimeState::Active;
          }
          else if (timeNow - runtime.stateStartMs > config.debounce)
          {
            runtime.state = FSRRuntimeState::Idle;
            updated = keyState.UpdateSemantic(false, 0, runtime.strikeVelocity);
            runtime.strikeVelocity = 0;
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
