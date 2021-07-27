#include "Device.h"
#include "Config.h"
#include "framework/Framework.h"
#include "stm32f1xx_hal.h"

namespace Device
{
    void Device_Init()
    {
        SystemClock_Config();
        SysTick_Config(SystemCoreClock / 1000); //CMSIS

        GPIO_Init();
    }

    void Bootloader()
    {
        
    }

    void Reboot()
    {
        // NVIC_SystemReset();
    }

    void Delay(uint32_t interval)
    {
        HAL_Delay(interval);
    }

    uint32_t Millis()
    {
        return HAL_GetTick();
    }

    namespace
    {
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
        void SystemClock_Config(void)
        {
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

        void GPIO_Init()
        {
            GPIO_InitTypeDef GPIO_InitStruct = {0};

            /* Configure USB FS GPIOs */
            __HAL_RCC_GPIOA_CLK_ENABLE();

            /* Configure USB D+ D- Pins */
            GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
            HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

            // USB Clock enable
            __HAL_RCC_USB_CLK_ENABLE();
        }
    }

    void Error_Handler()
    {
        
    }
}

void OTG_FS_IRQHandler(void)
{
  tud_int_handler(0);
}