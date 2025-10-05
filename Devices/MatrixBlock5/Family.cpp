#include "Device.h"
#include "MatrixOS.h"
#include "UI/UI.h"
#include "MatrixOSConfig.h"

#include "timers.h"

// Forward declare TinyUSB interrupt handler
extern "C" void dcd_int_handler(uint8_t rhport);

// Variant loader function
void LoadFounderEdition();


namespace Device
{
  void DeviceInit() {
    HAL_Init();
    SystemClock_Config();

    USB::Init();
    LED::Init();
    KeyPad::Init();
    // NVS::Init(); //Not working TODO FIX
  }

  void DeviceStart() {
    KeyPad::Start();
    LED::Start();
  
    // Keypad scanning is now handled by FreeRTOS timer, no need to call Scan() directly
    //Use keyInfo->Force() instead KeyInfo->Active() because it might still be debouncing
    if (KeyPad::GetKey(KeyPad::XY2ID(Point(0, 0)))->Force() && KeyPad::GetKey(KeyPad::XY2ID(Point(1, 1)))->Force())
    { MatrixOS::SYS::ExecuteAPP("203 Systems", "Matrix Factory Menu"); }
    else if (KeyPad::GetKey(KeyPad::XY2ID(Point(6, 6)))->Force() &&
             KeyPad::GetKey(KeyPad::XY2ID(Point(7, 7)))->Force())
    {
      KeyPad::Clear();
      MatrixOS::UserVar::brightness.Set(LED::brightness_level[0]);
    }
    else if (KeyPad::GetKey(KeyPad::XY2ID(Point(0, 5)))->Force() &&
             KeyPad::GetKey(KeyPad::XY2ID(Point(1, 6)))->Force() &&
             KeyPad::GetKey(KeyPad::XY2ID(Point(0, 7)))->Force())
    {
      MatrixOS::LED::SetColor(Point(2, 2), Color(0xFF00FF));
      MatrixOS::LED::SetColor(Point(5, 2), Color(0xFF00FF));
      MatrixOS::LED::SetColor(Point(2, 5), Color(0xFF00FF));
      MatrixOS::LED::SetColor(Point(5, 5), Color(0xFF00FF));
      MatrixOS::LED::Update();
      MatrixOS::SYS::DelayMs(1500);
      NVS::Clear();
      MatrixOS::SYS::Reboot();
    }
  }

  void LoadDeviceInfo() {
#ifndef FACTORY_CONFIG
    // For MatrixBlock5, we'll use default device info since there's no EFUSE
    DeviceInfo defaultDeviceInfo{{'M', 'B', '5', 'F'}, {'F', 'O', 'U', 'N'}, 24, 1};
    memcpy(&deviceInfo, &defaultDeviceInfo, sizeof(DeviceInfo));
#endif
    LoadVariantInfo();
  }

  void LoadVariantInfo() {
    // For MatrixBlock5, we'll load the Founder Edition variant
    // This will set up the GPIO configurations for the specific hardware
    LoadFounderEdition();
  }

  void DeviceSettings()
  {
    UI deviceSettings("Device Settings", Color(0x00FFAA));

    UIToggle touchbarToggle;
    touchbarToggle.SetName("Touchbar");
    touchbarToggle.SetColor(Color(0x7957FB));
    touchbarToggle.SetValuePointer(&touchbar_enable);
    touchbarToggle.OnPress([&]() -> void { touchbar_enable.Save(); });
    deviceSettings.AddUIComponent(touchbarToggle, Point(0, 0));

    deviceSettings.Start();
  }

  void Bootloader() {
    // Write bootloader magic to RTC backup register
    RTC_HandleTypeDef RtcHandle;
    RtcHandle.Instance = RTC;
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&RtcHandle, 10, 0x424C);
    HAL_PWR_DisableBkUpAccess();

    Reboot();
  }

  void Reboot() {
    NVIC_SystemReset();
  }

  void Delay(uint32_t interval) {
    vTaskDelay(pdMS_TO_TICKS(interval));
  }

  uint32_t Millis() {
    return ((((uint64_t)xTaskGetTickCount()) * 1000) / configTICK_RATE_HZ);
  }

  void Log(string &format, va_list &valst) {
    // For STM32, we can use printf or custom logging
    // vprintf(format.c_str(), valst);
  }

  string GetSerial() {
    // For MatrixBlock5, we could use MCU unique ID if available
    return "MXB5-" + std::to_string(HAL_GetUIDw0() ^ HAL_GetUIDw1() ^ HAL_GetUIDw2());
  }

  void ErrorHandler() {}

  uint64_t Micros() {
    return HAL_GetTick() * 1000;  // Approximate micros for STM32
  }

  /**
   * @brief  System Clock Configuration
   *         The system Clock is configured as follow :
   *            System Clock source            = PLL (HSE)
   *            SYSCLK(Hz)                     = 72000000
   *            HCLK(Hz)                       = 72000000
   *            AHB Prescaler                  = 1
   *            APB1 Prescaler                 = 2
   *            APB2 Prescaler                 = 1
   *            HSE Frequency(Hz)              = 8000000
   *            HSE PREDIV1                    = 1
   *            PLLMUL                         = 9
   *            Flash Latency(WS)              = 2
   * @param  None
   * @retval None
   */
  void SystemClock_Config(void) {
    RCC_ClkInitTypeDef clkinitstruct = {0};
    RCC_OscInitTypeDef oscinitstruct = {0};
    RCC_PeriphCLKInitTypeDef rccperiphclkinit = {0};

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    oscinitstruct.OscillatorType  = RCC_OSCILLATORTYPE_HSE;
    oscinitstruct.HSEState        = RCC_HSE_ON;
    oscinitstruct.HSEPredivValue  = RCC_HSE_PREDIV_DIV1;
    oscinitstruct.PLL.PLLMUL      = RCC_PLL_MUL9;
    oscinitstruct.PLL.PLLState    = RCC_PLL_ON;
    oscinitstruct.PLL.PLLSource   = RCC_PLLSOURCE_HSE;
    HAL_RCC_OscConfig(&oscinitstruct);

    /* USB clock selection */
    rccperiphclkinit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    rccperiphclkinit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
    HAL_RCCEx_PeriphCLKConfig(&rccperiphclkinit);

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    clkinitstruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    clkinitstruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clkinitstruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clkinitstruct.APB1CLKDivider = RCC_HCLK_DIV2;
    clkinitstruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clkinitstruct, FLASH_LATENCY_2);
  }
}

namespace MatrixOS::SYS
{
  void ErrorHandler(string error);
}

extern "C" {
void _init(void) {
  ;
}

void NMI_Handler(void) {
  while (true)
  {}
}

void HardFault_Handler(void) {
  MatrixOS::SYS::ErrorHandler("Hard Fault");
  while (true)
  {}
}

void MemManage_Handler(void) {
  while (true)
  {}
}

void BusFault_Handler(void) {
  while (true)
  {}
}

void UsageFault_Handler(void) {
  while (true)
  {}
}

// void SVC_Handler (void)
// {
//     while(true){

//     }
// }

void DebugMon_Handler(void) {
  while (true)
  {}
}

// void PendSV_Handler (void)
// {
//     while(true){

//     }
// }

void vApplicationMallocFailedHook(void) {
  taskDISABLE_INTERRUPTS();
}

void vApplicationStackOverflowHook(xTaskHandle pxTask, char* pcTaskName) {
  (void)pxTask;
  (void)pcTaskName;

  taskDISABLE_INTERRUPTS();
}

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 * used by the Idle task. */
void vApplicationGetIdleTaskMemory(StaticTask_t** ppxIdleTaskTCBBuffer, StackType_t** ppxIdleTaskStackBuffer,
                                   uint32_t* pulIdleTaskStackSize) {
  /* If the buffers to be provided to the Idle task are declared inside this
   * function then they must be declared static - otherwise they will be allocated on
   * the stack and so not exists after this function exits. */
  static StaticTask_t xIdleTaskTCB;
  static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
      state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
      Note that, as the array is necessarily of type StackType_t,
      configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
 * application must provide an implementation of vApplicationGetTimerTaskMemory()
 * to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t** ppxTimerTaskTCBBuffer, StackType_t** ppxTimerTaskStackBuffer,
                                    uint32_t* pulTimerTaskStackSize) {
  /* If the buffers to be provided to the Timer task are declared inside this
   * function then they must be declared static - otherwise they will be allocated on
   * the stack and so not exists after this function exits. */
  static StaticTask_t xTimerTaskTCB;
  static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

  /* Pass out a pointer to the StaticTask_t structure in which the Timer
      task's state will be stored. */
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

  /* Pass out the array that will be used as the Timer task's stack. */
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;

  /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
      Note that, as the array is necessarily of type StackType_t,
      configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
}

extern "C" {
void* __dso_handle = 0;
}
