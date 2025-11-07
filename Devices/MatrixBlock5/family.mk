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

.PHONY: build flash upload

build: $(BUILD)/$(PROJECT)-$(DEVICE).bin

$(BUILD)/$(PROJECT)-$(DEVICE).elf:
	cmake -B $(BUILD) -Wno-dev . -DCMAKE_TOOLCHAIN_FILE=$(FAMILY_PATH)/toolchain-stm32f103.cmake -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) -DMODE=$(MODE) -G"Unix Makefiles"
	cmake --build $(BUILD) -- -j8

$(BUILD)/$(PROJECT)-$(DEVICE).bin: $(BUILD)/$(PROJECT)-$(DEVICE).elf
	arm-none-eabi-objcopy -O binary "$<" "$@"

flash upload: $(BUILD)/$(PROJECT)-$(DEVICE).bin
	dfu-util "-a 0" "-d 0203:0003" -D"$<" -R 

