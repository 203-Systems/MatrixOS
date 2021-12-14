UF2_FAMILY_ID = 0x5ee21072
ST_FAMILY = f1
DEPS_SUBMODULES += lib/CMSIS_5 core/stm32$(ST_FAMILY)/cmsis_device_$(ST_FAMILY) core/stm32$(ST_FAMILY)/stm32$(ST_FAMILY)xx_hal_driver

ST_CMSIS = core/stm32$(ST_FAMILY)/cmsis_device_$(ST_FAMILY)
ST_HAL_DRIVER = core/stm32$(ST_FAMILY)/stm32$(ST_FAMILY)xx_hal_driver

include $(DEVICE_PATH)/device.mk

CFLAGS += \
  -mthumb \
  -mabi=aapcs \
  -mcpu=cortex-m3 \
  -mfloat-abi=soft \
  -DCFG_TUSB_MCU=OPT_MCU_STM32F1\
  -nostartfiles

SRC_C += \
	lib\tinyusb\src\portable\st\stm32_fsdev\dcd_stm32_fsdev.c  \
	$(ST_CMSIS)/Source/Templates/system_stm32$(ST_FAMILY)xx.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_cortex.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_rcc.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_rcc_ex.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_gpio.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_dma.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_tim.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_tim_ex.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_pwr.c\
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_rtc.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_rtc_ex.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_flash.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_flash_ex.c

SRC_CPP += \
	core\STM32F1\ws2812\WS2812.cpp \

INC += \
	$(DEVICE_PATH) \
	$(FAMILY_PATH)/Drivers \
	lib/CMSIS_5/CMSIS/Core/Include \
	core/stm32$(ST_FAMILY) \
	$(ST_CMSIS)/Include \
	$(ST_HAL_DRIVER)/Inc

# For freeRTOS port source
FREERTOS_PORT = ARM_CM3
