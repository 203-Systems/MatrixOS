#include "Family.h"
#include "tusb.h"

namespace Device::USB
{
  void Init()
  {
    NVIC_SetPriority(USB_HP_CAN1_TX_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_SetPriority(USBWakeUp_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);

    // Enable GPIO clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // Configure USB DM and DP pins (PA11, PA12)
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // USB Clock enable
    __HAL_RCC_USB_CLK_ENABLE();
  }
}

extern "C" {
void USB_HP_IRQHandler(void) {
  dcd_int_handler(0);
}

void USB_LP_IRQHandler(void) {
  dcd_int_handler(0);
}

void USBWakeUp_IRQHandler(void) {
  dcd_int_handler(0);
}
}