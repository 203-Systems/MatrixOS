#include "Device.h"

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
        void SystemClock_Config()
        {
            RCC_OscInitTypeDef RCC_OscInitStruct = {0};
            RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

            /** Configure the main internal regulator output voltage
             */
            __HAL_RCC_PWR_CLK_ENABLE();
            __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
            /** Initializes the RCC Oscillators according to the specified parameters
             * in the RCC_OscInitTypeDef structure.
             */
            RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
            RCC_OscInitStruct.HSIState = RCC_HSI_ON;
            RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
            RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
            RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
            RCC_OscInitStruct.PLL.PLLM = 16;
            RCC_OscInitStruct.PLL.PLLN = 336;
            RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
            RCC_OscInitStruct.PLL.PLLQ = 7;
            if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
            {
                Error_Handler();
            }
            /** Initializes the CPU, AHB and APB buses clocks
             */
            RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                        |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
            RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
            RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
            RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

            if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
            {
                Error_Handler();
            }
        }

        void GPIO_Init()
        {
            GPIO_InitTypeDef GPIO_InitStruct = {0};

            /* Configure USB FS GPIOs */
            __HAL_RCC_GPIOA_CLK_ENABLE();

            /* Configure USB D+ D- Pins */
            GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
            GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
            HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

            /* Configure VBUS Pin */
            GPIO_InitStruct.Pin = GPIO_PIN_9;
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        }
    }

    void Error_Handler()
    {
        
    }
}