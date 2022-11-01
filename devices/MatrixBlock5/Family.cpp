#include "Device.h"

namespace MatrixOS::SYS
{
  void ExecuteAPP(string author, string app_name);
}

namespace Device
{
  void DeviceInit() {
    HAL_Init();
    NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(USBWakeUp_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    SystemClock_Config();

    USB_Init();
    LED_Init();
    KeyPad_Init();
    // TouchBar_Init();
    // NVS::Init(); //Not working TODO FIX
  }

  void PostBootTask() {
    if (KeyPad::GetKey(Point(0, 0)) && KeyPad::GetKey(Point(1, 1)))
    { MatrixOS::SYS::ExecuteAPP("203 Electronics", "Matrix Factory Menu"); }
  }

  void DeviceTask() {}

  void Bootloader() {
    BKP::Write(10, 0x424C);
    Reboot();
  }

  void Reboot() {
    NVIC_SystemReset();
  }

  void Delay(uint32_t interval) {
    HAL_Delay(interval);
  }

  uint32_t Millis() {
    return HAL_GetTick();
  }

  void Log(string format, va_list valst) {}

  string GetSerial() {
    return "<Serial Number>";  // TODO
  }

  void ErrorHandler() {}

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
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    { ErrorHandler(); }
    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    { ErrorHandler(); }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    { ErrorHandler(); }
  }

}

namespace MatrixOS::SYS
{
  void ErrorHandler(string error);
}

extern "C" {
void USB_HP_IRQHandler(void) {
  tud_int_handler(0);
}

void USB_LP_IRQHandler(void) {
  tud_int_handler(0);
}

void USBWakeUp_IRQHandler(void) {
  tud_int_handler(0);
}

// void SysTick_Handler(void)
// {
//     HAL_IncTick();
// }

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
