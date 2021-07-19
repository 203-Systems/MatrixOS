UF2_FAMILY_ID = 0x5ee21072
ST_FAMILY = f1
DEPS_SUBMODULES += lib/CMSIS_5 core/stm32$(ST_FAMILY)/cmsis_device_$(ST_FAMILY) core/stm32$(ST_FAMILY)/stm32$(ST_FAMILY)xx_hal_driver

ST_CMSIS = core/stm32$(ST_FAMILY)/cmsis_device_$(ST_FAMILY)
ST_HAL_DRIVER = core/stm32$(ST_FAMILY)/stm32$(ST_FAMILY)xx_hal_driver

include $(BOARD_PATH)/device.mk

CFLAGS += \
  -flto \
  -mthumb \
  -mabi=aapcs \
  -mcpu=cortex-m3 \
  -mfloat-abi=soft \
  -nostdlib -nostartfiles \
  -DCFG_TUSB_MCU=OPT_MCU_STM32F1

# suppress warning caused by vendor mcu driver
CFLAGS += -Wno-error=cast-align

SRC_C += \
	$(ST_CMSIS)/Source/Templates/system_stm32$(ST_FAMILY)xx.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_cortex.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_rcc.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_rcc_ex.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_uart.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_gpio.c

INC += \
	$(BOARD_PATH) \
	lib/CMSIS_5/CMSIS/Core/Include \
	$(ST_CMSIS)/Include \
	$(ST_HAL_DRIVER)/Inc
# For freeRTOS port source
FREERTOS_PORT = ARM_CM3
