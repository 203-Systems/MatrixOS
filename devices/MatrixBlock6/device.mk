UF2_FAMILY_ID = 0x57755a57
ST_FAMILY = f4
DEPS_SUBMODULES += lib/CMSIS_5 core/stm32$(ST_FAMILY)/cmsis_device_$(ST_FAMILY) core/stm32$(ST_FAMILY)/stm32$(ST_FAMILY)xx_hal_driver

ST_CMSIS = core/stm32$(ST_FAMILY)/cmsis_device_$(ST_FAMILY)
ST_HAL_DRIVER = core/stm32$(ST_FAMILY)/stm32$(ST_FAMILY)xx_hal_driver

CFLAGS += -DSTM32F401xE

LD_FILE = $(BOARD_PATH)/STM32F401RETx_FLASH.ld

SRC_S += $(ST_CMSIS)/Source/Templates/gcc/startup_stm32f401xc.s

# For flash-jlink target
JLINK_DEVICE = stm32f401re

# flash target ROM bootloader
flash: $(BUILD)/$(PROJECT).bin
	dfu-util -R -a 0 --dfuse-address 0x08008000 -D $<

CFLAGS += \
  -flto \
  -mthumb \
  -mabi=aapcs \
  -mcpu=cortex-m4 \
  -mfloat-abi=hard \
  -mfpu=fpv4-sp-d16 \
  -nostdlib -nostartfiles \
  -DCFG_TUSB_MCU=OPT_MCU_STM32F4

# suppress warning caused by vendor mcu driver
CFLAGS += -Wno-error=cast-align

SRC_C += \
	$(ST_CMSIS)/Source/Templates/system_stm32$(ST_FAMILY)xx.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_cortex.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_rcc.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_rcc_ex.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_uart.c \
	$(ST_HAL_DRIVER)/Src/stm32$(ST_FAMILY)xx_hal_gpio.c \
	# src/portable/st/synopsys/dcd_synopsys.c

INC += \
	$(BOARD_PATH) \
	lib/CMSIS_5/CMSIS/Core/Include \
	$(ST_CMSIS)/Include \
	$(ST_HAL_DRIVER)/Inc

# For freeRTOS port source
FREERTOS_PORT = ARM_CM4F

# flash target using on-board 