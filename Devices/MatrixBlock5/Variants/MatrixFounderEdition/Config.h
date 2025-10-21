#pragma once

#include "../../MatrixOSConfig.h"
#include "stm32f1xx_hal.h"

#include <cstddef>
#include <cstdint>

namespace MatrixBlock5 {
namespace FounderEdition {

struct GpioPin {
  GPIO_TypeDef* port;
  uint16_t pin;
};

inline constexpr const char* kDeviceName = "MatrixBlock5";
inline constexpr const char* kDeviceModel = "MB5F";
inline constexpr const char* kManufacturerName = "203 Systems";
inline constexpr const char* kProductName = "MatrixBlock5";
inline constexpr uint16_t kUsbVid = 0x0203;
inline constexpr uint16_t kUsbPid = 0x1041;

inline constexpr uint16_t kKeypadScanRateHz = 60;

inline constexpr size_t kGridColumns = X_SIZE;
inline constexpr size_t kGridRows = Y_SIZE;
inline constexpr size_t kBrightnessSteps = 8;
inline constexpr size_t kTouchbarEntries = 16;

inline const uint8_t kBrightnessLevels[kBrightnessSteps] = {8, 12, 24, 40, 64, 90, 128, 168};

inline const GpioPin kLedPin{GPIOC, GPIO_PIN_7};
inline const GpioPin kFnPin{GPIOA, GPIO_PIN_8};

inline const GpioPin kKeypadColumns[kGridColumns] = {
    {GPIOB, GPIO_PIN_12},
    {GPIOB, GPIO_PIN_13},
    {GPIOB, GPIO_PIN_14},
    {GPIOB, GPIO_PIN_15},
    {GPIOC, GPIO_PIN_6},
    {GPIOC, GPIO_PIN_7},
    {GPIOC, GPIO_PIN_8},
    {GPIOC, GPIO_PIN_9},
};

inline const GpioPin kKeypadRows[kGridRows] = {
    {GPIOB, GPIO_PIN_0},
    {GPIOB, GPIO_PIN_1},
    {GPIOA, GPIO_PIN_11},
    {GPIOA, GPIO_PIN_12},
    {GPIOC, GPIO_PIN_4},
    {GPIOA, GPIO_PIN_15},
    {GPIOC, GPIO_PIN_5},
    {GPIOC, GPIO_PIN_3},
};

inline const GpioPin kTouchDataPin{GPIOA, GPIO_PIN_0};
inline const GpioPin kTouchClockPin{GPIOA, GPIO_PIN_1};

inline const uint8_t kTouchbarMap[kTouchbarEntries] = {4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8, 0, 1, 2, 3};

}  // namespace FounderEdition
}  // namespace MatrixBlock5

extern TIM_HandleTypeDef htim8;
extern DMA_HandleTypeDef hdma_tim8_ch2;

namespace Device {
void LoadFounderEdition();
}

extern "C" {
void MX_DMA_Init(void);
void MX_TIM8_Init(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);
void DMA2_Channel4_5_IRQHandler(void);
}
