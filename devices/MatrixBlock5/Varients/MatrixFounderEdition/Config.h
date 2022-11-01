// Define Device Specific Macro, Value and private function
#pragma once

#include "Family.h"

#define GRID_8x8
#define MODEL MXFE1
#define MULTIPRESS 10  // Key Press will be process at once

extern TIM_HandleTypeDef htim8;
extern DMA_HandleTypeDef hdma_tim8_ch2;

namespace Device
{
  const string name = "Matrix Founder Edition";
  const string model = "MXFE1";

  const string manufaturer_name = "203 Electronics";
  const string product_name = "Matrix";
  const uint16_t usb_vid = 0x0203;
  const uint16_t usb_pid = 0x1040;  //(Device Class)0001 (Device Code)000001 (Reserved for Device ID (0~63))000000

  const uint16_t numsOfLED = 64;
  const uint8_t x_size = 8;
  const uint8_t y_size = 8;

#define MAX_LED_LAYERS 5
  inline uint16_t keypad_scanrate = 60;
  inline uint16_t fps = 120;  // Depends on the FreeRTOS tick speed

  inline uint8_t brightness_level[8] = {8, 12, 24, 40, 64, 90, 128, 168};

  const uint8_t touchbar_size = 8;  // Not required by the API, private use. 16 Physical but 8 virtualized key.

  const uint16_t page_size = 2048;
  const uint8_t nums_of_page = 32;  // Total size has to smaller than 64kb because address constrain
  const uint32_t nvs_address = 0x8070000;

  namespace KeyPad
  {
    inline KeyInfo fnState;
    inline KeyInfo keypadState[x_size][y_size];
    inline KeyInfo touchbarState[x_size];

    void FNScan();
    void KeyPadScan();
    void TouchBarScan();

    bool addToList(uint16_t keyID);  // Return true when list is full.
    void clearList();
    bool isListFull();
  }
}

extern "C" {
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim);
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void DMA2_Channel4_5_IRQHandler(void);
}

struct GPIO {
  GPIO_TypeDef* port;
  uint16_t pin;

  GPIO(GPIO_TypeDef* port, uint16_t pin) {
    this->port = port;
    this->pin = pin;
  }
};

#define FN_Pin GPIO_PIN_0
#define FN_GPIO_Port GPIOA

#define Key1_Pin GPIO_PIN_15
#define Key1_GPIO_Port GPIOB
#define Key2_Pin GPIO_PIN_14
#define Key2_GPIO_Port GPIOB
#define Key3_Pin GPIO_PIN_13
#define Key3_GPIO_Port GPIOB
#define Key4_Pin GPIO_PIN_12
#define Key4_GPIO_Port GPIOB
#define Key5_Pin GPIO_PIN_6
#define Key5_GPIO_Port GPIOC
#define Key6_Pin GPIO_PIN_15
#define Key6_GPIO_Port GPIOC
#define Key7_Pin GPIO_PIN_14
#define Key7_GPIO_Port GPIOC
#define Key8_Pin GPIO_PIN_13
#define Key8_GPIO_Port GPIOC

#define KeyRead1_Pin GPIO_PIN_1
#define KeyRead1_GPIO_Port GPIOB
#define KeyRead2_Pin GPIO_PIN_0
#define KeyRead2_GPIO_Port GPIOB
#define KeyRead3_Pin GPIO_PIN_2
#define KeyRead3_GPIO_Port GPIOA
#define KeyRead4_Pin GPIO_PIN_1
#define KeyRead4_GPIO_Port GPIOA
#define KeyRead5_Pin GPIO_PIN_3
#define KeyRead5_GPIO_Port GPIOC
#define KeyRead6_Pin GPIO_PIN_3
#define KeyRead6_GPIO_Port GPIOA
#define KeyRead7_Pin GPIO_PIN_5
#define KeyRead7_GPIO_Port GPIOC
#define KeyRead8_Pin GPIO_PIN_4
#define KeyRead8_GPIO_Port GPIOC

#define TouchData_Pin GPIO_PIN_6
#define TouchData_GPIO_Port GPIOA
#define TouchClock_Pin GPIO_PIN_7
#define TouchClock_GPIO_Port GPIOA

inline GPIO keypad_write_pins[] = {
    GPIO(Key1_GPIO_Port, Key1_Pin), GPIO(Key2_GPIO_Port, Key2_Pin), GPIO(Key3_GPIO_Port, Key3_Pin),
    GPIO(Key4_GPIO_Port, Key4_Pin), GPIO(Key5_GPIO_Port, Key5_Pin), GPIO(Key6_GPIO_Port, Key6_Pin),
    GPIO(Key7_GPIO_Port, Key7_Pin), GPIO(Key8_GPIO_Port, Key8_Pin),
};

inline GPIO keypad_read_pins[] = {
    GPIO(KeyRead1_GPIO_Port, KeyRead1_Pin), GPIO(KeyRead2_GPIO_Port, KeyRead2_Pin),
    GPIO(KeyRead3_GPIO_Port, KeyRead3_Pin), GPIO(KeyRead4_GPIO_Port, KeyRead4_Pin),
    GPIO(KeyRead5_GPIO_Port, KeyRead5_Pin), GPIO(KeyRead6_GPIO_Port, KeyRead6_Pin),
    GPIO(KeyRead7_GPIO_Port, KeyRead7_Pin), GPIO(KeyRead8_GPIO_Port, KeyRead8_Pin),
};

inline GPIO fn_pin = GPIO(FN_GPIO_Port, FN_Pin);

inline GPIO touch_clock_pin = GPIO(TouchClock_GPIO_Port, TouchClock_Pin);
inline GPIO touch_data_pin = GPIO(TouchData_GPIO_Port, TouchData_Pin);