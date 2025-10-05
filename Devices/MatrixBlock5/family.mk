UF2_FAMILY_ID = 0x5ee21072
ST_FAMILY = f1
DEPS_SUBMODULES += lib/CMSIS_5 core/stm32$(ST_FAMILY)/cmsis_device_$(ST_FAMILY) core/stm32$(ST_FAMILY)/stm32$(ST_FAMILY)xx_hal_driver

# Detect whether shell style is windows or not
# https://stackoverflow.com/questions/714100/os-detecting-makefile/52062069#52062069
ifeq '$(findstring ;,$(PATH))' ';'
CMDEXE := 1
endif

ifeq ($(CMDEXE),1)
  CP = copy >nul
  RM = del
else
  CP = cp
  RM = rm
endif

ST_CMSIS = core/stm32$(ST_FAMILY)/cmsis_device_$(ST_FAMILY)
ST_HAL_DRIVER = core/stm32$(ST_FAMILY)/stm32$(ST_FAMILY)xx_hal_driver

# include $(DEVICE_PATH)/device.mk

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
	Platform\STM32F1\WS2812\WS2812.cpp \

INC += \
	$(DEVICE_PATH) \
	$(FAMILY_PATH)/Drivers \
	lib/CMSIS_5/CMSIS/Core/Include \
	Platform/stm32$(ST_FAMILY) \
	$(ST_CMSIS)/Include \
	$(ST_HAL_DRIVER)/Inc

# For freeRTOS port source
FREERTOS_PORT = ARM_CM3

.PHONY: build flash upload

$(BUILD)/$(PROJECT)-$(DEVICE).bin build:
	cmake -B $(BUILD) -Wno-dev . -DCMAKE_TOOLCHAIN_FILE=$(FAMILY_PATH)/toolchain-stm32f103.cmake -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) -DMODE=$(MODE) -G"Unix Makefiles"
	cmake --build $(BUILD) -- -j8
	@$(CP) "$(subst /,\,$(BUILD)/Devices/MatrixBlock5/$(PROJECT)-$(DEVICE).bin)" "$(subst /,\,$(BUILD)/)"
	$(if $(wildcard $(BUILD)/Devices/MatrixBlock5/$(PROJECT)-$(DEVICE).map),@$(CP) "$(subst /,\,$(BUILD)/Devices/MatrixBlock5/$(PROJECT)-$(DEVICE).map)" "$(subst /,\,$(BUILD)/)",)

flash upload: $(BUILD)/$(PROJECT)-$(DEVICE).bin
	dfu-util "-a 0" "-d 0203:0003" -D"$<" -R 

