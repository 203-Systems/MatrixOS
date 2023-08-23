CFLAGS += -DSTM32F103xE

LD_FILE = $(DEVICE_PATH)/STM32F103RETx_FLASH.ld

SRC_S += $(ST_CMSIS)/Source/Templates/gcc/startup_stm32f103xe.s

# For flash-jlink target
JLINK_DEVICE = stm32f103re

# flash target ROM bootloader
# flash: $(BUILD)/$(PROJECT).bin
# 	dfu-util -R -a 0 --dfuse-address 0x08002000 -D $<
FLASH_ADDRESS = 0x08002000

# flash: flash-stlink-elf

flash: $(BUILD)/$(PROJECT).bin
	dfu-util "-a 0" "-d 0203:0003" -D"$<" -R 