// Matrix Founder Edition Variant Configuration
#include "Device.h"
#include "WS2812/WS2812.h"
#include "Config.h"

TIM_HandleTypeDef htim8;
DMA_HandleTypeDef hdma_tim8_ch2;

extern "C" void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);

void MX_DMA_Init(void) {
  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Channel4_5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Channel4_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Channel4_5_IRQn);
}

void MX_TIM8_Init(void) {
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = WS2812::GetTimerPeriod();
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
  { Device::ErrorHandler(); }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  { Device::ErrorHandler(); }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  { Device::ErrorHandler(); }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
  { Device::ErrorHandler(); }

  HAL_TIM_MspPostInit(&htim8);
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (htim->Instance == TIM8)
  {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**TIM8 GPIO Configuration
    PC7     ------> TIM8_CH2
    */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  }
}

extern "C" {
void DMA2_Channel4_5_IRQHandler(void) {
  HAL_DMA_IRQHandler(&hdma_tim8_ch2);
}
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm) {
  if (htim_pwm->Instance == TIM8)
  {
    /* Peripheral clock enable */
    __HAL_RCC_TIM8_CLK_ENABLE();

    /* TIM8 DMA Init */
    hdma_tim8_ch2.Instance = DMA2_Channel5;
    hdma_tim8_ch2.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tim8_ch2.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tim8_ch2.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tim8_ch2.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;  // WORD for TIM CCR register
    hdma_tim8_ch2.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;     // BYTE for pwmBuffer
    hdma_tim8_ch2.Init.Mode = DMA_NORMAL;
    hdma_tim8_ch2.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_tim8_ch2) != HAL_OK)
    { Device::ErrorHandler(); }

    __HAL_LINKDMA(htim_pwm, hdma[TIM_DMA_ID_CC2], hdma_tim8_ch2);
  }
}

void LoadFounderEdition() {
  using namespace MatrixBlock5::FounderEdition;

  static_assert(kGridColumns == X_SIZE, "FounderEdition grid width mismatch");
  static_assert(kGridRows == Y_SIZE, "FounderEdition grid height mismatch");
  static_assert(kTouchbarEntries == Device::KeyPad::touchbar_size, "FounderEdition touchbar size mismatch");

  Device::LED::led_port = kLedPin.port;
  Device::LED::led_pin = kLedPin.pin;
  for (size_t i = 0; i < kBrightnessSteps; ++i)
  { Device::LED::brightness_level[i] = kBrightnessLevels[i]; }

  Device::KeyPad::keypad_scanrate = kKeypadScanRateHz;
  Device::KeyPad::fn_port = kFnPin.port;
  Device::KeyPad::fn_pin = kFnPin.pin;

  for (size_t i = 0; i < kGridColumns; ++i)
  {
    Device::KeyPad::keypad_write_ports[i] = kKeypadColumns[i].port;
    Device::KeyPad::keypad_write_pins[i] = kKeypadColumns[i].pin;
  }

  for (size_t i = 0; i < kGridRows; ++i)
  {
    Device::KeyPad::keypad_read_ports[i] = kKeypadRows[i].port;
    Device::KeyPad::keypad_read_pins[i] = kKeypadRows[i].pin;
  }

  Device::KeyPad::touchData_Port = kTouchDataPin.port;
  Device::KeyPad::touchData_Pin = kTouchDataPin.pin;
  Device::KeyPad::touchClock_Port = kTouchClockPin.port;
  Device::KeyPad::touchClock_Pin = kTouchClockPin.pin;

  for (size_t i = 0; i < kTouchbarEntries; ++i)
  { Device::KeyPad::touchbar_map[i] = kTouchbarMap[i]; }

  Device::name = kDeviceName;
  Device::model = kDeviceModel;
  Device::manufacturer_name = kManufacturerName;
  Device::product_name = kProductName;
  Device::usb_vid = kUsbVid;
  Device::usb_pid = kUsbPid;
}
