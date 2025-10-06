// Matrix Founder Edition Variant Configuration
#include "Device.h"
#include "WS2812/WS2812.h"

// TIM8 handle for WS2812 LED driver
TIM_HandleTypeDef htim8;
DMA_HandleTypeDef hdma_tim8_ch2;

// Forward declarations
extern "C" void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);

// Timer and DMA initialization functions (called from LED.cpp)
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

// HAL MSP callbacks for TIM8
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
  // Configure LED
  Device::LED::led_port = GPIOC;
  Device::LED::led_pin = GPIO_PIN_7;

  // Configure FN key
  Device::KeyPad::fn_port = GPIOA;
  Device::KeyPad::fn_pin = GPIO_PIN_8;

  // Configure keypad write pins (columns)
  Device::KeyPad::keypad_write_ports[0] = GPIOB; Device::KeyPad::keypad_write_pins[0] = GPIO_PIN_12; // Key1
  Device::KeyPad::keypad_write_ports[1] = GPIOB; Device::KeyPad::keypad_write_pins[1] = GPIO_PIN_13; // Key2
  Device::KeyPad::keypad_write_ports[2] = GPIOB; Device::KeyPad::keypad_write_pins[2] = GPIO_PIN_14; // Key3
  Device::KeyPad::keypad_write_ports[3] = GPIOB; Device::KeyPad::keypad_write_pins[3] = GPIO_PIN_15; // Key4
  Device::KeyPad::keypad_write_ports[4] = GPIOC; Device::KeyPad::keypad_write_pins[4] = GPIO_PIN_6;  // Key5
  Device::KeyPad::keypad_write_ports[5] = GPIOC; Device::KeyPad::keypad_write_pins[5] = GPIO_PIN_7;  // Key6
  Device::KeyPad::keypad_write_ports[6] = GPIOC; Device::KeyPad::keypad_write_pins[6] = GPIO_PIN_8;  // Key7
  Device::KeyPad::keypad_write_ports[7] = GPIOC; Device::KeyPad::keypad_write_pins[7] = GPIO_PIN_9;  // Key8

  // Configure keypad read pins (rows)
  Device::KeyPad::keypad_read_ports[0] = GPIOB; Device::KeyPad::keypad_read_pins[0] = GPIO_PIN_0;  // KeyRead1
  Device::KeyPad::keypad_read_ports[1] = GPIOB; Device::KeyPad::keypad_read_pins[1] = GPIO_PIN_1;  // KeyRead2
  Device::KeyPad::keypad_read_ports[2] = GPIOA; Device::KeyPad::keypad_read_pins[2] = GPIO_PIN_11; // KeyRead3
  Device::KeyPad::keypad_read_ports[3] = GPIOA; Device::KeyPad::keypad_read_pins[3] = GPIO_PIN_12; // KeyRead4
  Device::KeyPad::keypad_read_ports[4] = GPIOC; Device::KeyPad::keypad_read_pins[4] = GPIO_PIN_4;  // KeyRead5
  Device::KeyPad::keypad_read_ports[5] = GPIOA; Device::KeyPad::keypad_read_pins[5] = GPIO_PIN_15; // KeyRead6
  Device::KeyPad::keypad_read_ports[6] = GPIOC; Device::KeyPad::keypad_read_pins[6] = GPIO_PIN_5;  // KeyRead7
  Device::KeyPad::keypad_read_ports[7] = GPIOC; Device::KeyPad::keypad_read_pins[7] = GPIO_PIN_3;  // KeyRead8

  // Configure touchbar
  Device::KeyPad::touchData_Port = GPIOA;
  Device::KeyPad::touchData_Pin = GPIO_PIN_0;
  Device::KeyPad::touchClock_Port = GPIOA;
  Device::KeyPad::touchClock_Pin = GPIO_PIN_1;

  // Set device name and model
  Device::name = "MatrixBlock5";
  Device::model = "MB5F";
  Device::product_name = "MatrixBlock5";
}