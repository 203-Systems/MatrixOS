ifneq ($(lastword a b),b)
$(error This Makefile require make 3.81 or newer)
endif

# Detect whether shell style is windows or not
# https://stackoverflow.com/questions/714100/os-detecting-makefile/52062069#52062069
ifeq '$(findstring ;,$(PATH))' ';'
CMDEXE := 1
endif

# Set TOP to be the path to get from the current directory (where make was
# invoked) to the top of the tree. $(lastword $(MAKEFILE_LIST)) returns
# the name of this makefile relative to where make was invoked.

THIS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
TOP := $(patsubst %makefile,%,$(THIS_MAKEFILE))

ifeq ($(CMDEXE),1)
TOP := $(subst \,/,$(shell for %%i in ( $(TOP) ) do echo %%~fi))
else
TOP := $(dir $(realpath $(THIS_MAKEFILE)))
endif
# $(info Top directory is $(TOP))

# ifeq ($(CMDEXE),1)
# CURRENT_PATH := $(subst ,,$(subst \,/,$(shell echo %CD%)))
# else
# CURRENT_PATH := $(shell realpath --relative-to=$(TOP) `pwd`)
# endif
#
# $(info Path from top is $(CURRENT_PATH))

# Build directory
# ifeq ($(CMDEXE),1)
# $(shell if exist build\$(DEVICE) rd build\$(DEVICE) /s /q)
# endif

# Build directory
BUILD := build/$(DEVICE)

PROJECT := $(notdir $(CURDIR))
BIN := _bin/$(DEVICE)/$(notdir $(CURDIR))

# Handy check parameter function
check_defined = \
    $(strip $(foreach 1,$1, \
    $(call __check_defined,$1,$(strip $(value 2)))))
__check_defined = \
    $(if $(value $1),, \
    $(error Undefined make flag: $1$(if $2, ($2))))

#-------------- Select the device to build for. ------------

# Device without family
ifneq ($(wildcard ./devices/$(DEVICE)/device.mk),)
DEVICE_PATH := devices/$(DEVICE)
FAMILY :=
endif

# Device within family
ifeq ($(DEVICE_PATH),)
  DEVICE_PATH := $(subst ,,$(wildcard devices/*/Varients/$(DEVICE)))
  FAMILY := $(word 2, $(subst /, ,$(DEVICE_PATH)))
  FAMILY_PATH = devices/$(FAMILY)
endif

$(info Device Path: $(DEVICE_PATH))
$(info Family: $(FAMILY))
$(info Family Path: $(FAMILY_PATH))

ifeq ($(DEVICE_PATH),)
  $(info You must provide a DEVICE parameter with 'DEVICE=')
  $(error Invalid DEVICE specified)
endif


ifeq ($(FAMILY),)
  include devices/$(DEVICE)/device.mk
else
  # Include Family and Device specific defs
  include $(FAMILY_PATH)/family.mk
endif

# Fetch submodules depended by family
fetch_submodule_if_empty = $(if $(wildcard $1/*),,$(info $(shell git -C submodule update --init $1)))
ifdef DEPS_SUBMODULES
  $(foreach s,$(DEPS_SUBMODULES),$(call fetch_submodule_if_empty,$(s)))
endif

#-------------- Cross Compiler  ------------
# Can be set by board, default to ARM GCC
CROSS_COMPILE ?= arm-none-eabi-
# Allow for -Os to be changed by board makefiles in case -Os is not allowed
CFLAGS_OPTIMIZED ?= -Os

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
GDB = $(CROSS_COMPILE)gdb
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size
MKDIR = mkdir

ifeq ($(CMDEXE),1)
  CP = copy
  RM = del
  PYTHON = python
else
  SED = sed
  CP = cp
  RM = rm
  PYTHON = python3
endif

#-------------- Source files and compiler flags --------------

# Compiler Flags
CFLAGS += \
  -ggdb \
  -fdata-sections \
  -ffunction-sections \
  -fsingle-precision-constant \
  -fno-strict-aliasing \
  -Wdouble-promotion \
  -Wstrict-overflow \
  -Wextra \
  -Wfloat-equal \
  -Wundef \
  -Wwrite-strings \
  -Wsign-compare \
  -Wmissing-format-attribute \
  -Wunreachable-code \
  -Wcast-align \
  -Wcast-function-type

CPPFLAGS += \
	-std=gnu++17 \
	# -lstdc++

# Debugging/Optimization
ifeq ($(DEBUG), 1)
  CFLAGS += -Og -g
else
  CFLAGS += $(CFLAGS_OPTIMIZED)
endif

# Log level is mapped to TUSB DEBUG option
ifneq ($(LOG),)
  CMAKE_DEFSYM +=	-DLOG=$(LOG)
  CFLAGS += -DCFG_TUSB_DEBUG=$(LOG)
endif

# Logger: default is uart, can be set to rtt or swo
ifneq ($(LOGGER),)
	CMAKE_DEFSYM +=	-DLOGGER=$(LOGGER)
endif

ifeq ($(LOGGER),rtt)
  CFLAGS += -DLOGGER_RTT -DSEGGER_RTT_MODE_DEFAULT=SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL
  RTT_SRC = lib/SEGGER_RTT
  INC   += $(TOP)/$(RTT_SRC)/RTT
  SRC_C += $(RTT_SRC)/RTT/SEGGER_RTT.c
else ifeq ($(LOGGER),swo)
  CFLAGS += -DLOGGER_SWO
endif

include source.mk

# ---------------------------------------
# Common make rules for all device
# ---------------------------------------

# Set all as default goal
.DEFAULT_GOAL := all

# ESP32-SX and RP2040 has its own CMake build system
ifneq ($(MCU),esp32s2)
ifneq ($(MCU),esp32s3)
ifneq ($(MCU),rp2040)

# ---------------------------------------
# GNU Make build system
# ---------------------------------------

# libc
LIBS += -lgcc -lm -lnosys

ifneq ($(DEVICE), spresense)
LIBS += -lc
endif

CFLAGS += $(addprefix -I,$(INC))

LDFLAGS += $(CFLAGS) -Wl,-T,$(LD_FILE) -Wl,-Map=$@.map -Wl,-cref -Wl,-gc-sections
ifneq ($(SKIP_NANOLIB), 1)
LDFLAGS += -specs=nosys.specs -specs=nano.specs
endif

ASFLAGS += $(CFLAGS)

CPPFLAGS += ${CFLAGS}

# Assembly files can be name with upper case .S, convert it to .s
SRC_S := $(SRC_S:.S=.s)

# Due to GCC LTO bug https://bugs.launchpad.net/gcc-arm-embedded/+bug/1747966
# assembly file should be placed first in linking order
# '_asm' suffix is added to object of assembly file

OBJ += $(addprefix $(BUILD)/obj/, $(SRC_S:.s=_asm.o))
OBJ += $(addprefix $(BUILD)/obj/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/obj/, $(SRC_CPP:.cpp=.o))

# Verbose mode
ifeq ("$(V)","1")
$(info Shell$(SHELL))
$(info DEVICE_PATH  $(DEVICE_PATH)) $(info )
$(info FAMILY_PATH  $(FAMILY_PATH)) $(info )
$(info INC  $(INC)) $(info )
$(info SRC C  $(SRC_C)) $(info )
$(info SRC CPP  $(SRC_CPP)) $(info )
$(info SRC S  $(SRC_S)) $(info )
$(info CFLAGS  $(CFLAGS) ) $(info )
$(info CPPFLAGS  $(CPPFLAGS) ) $(info )
$(info LDFLAGS $(LDFLAGS)) $(info )
$(info ASFLAGS $(ASFLAGS)) $(info )
endif

all: $(BUILD)/$(PROJECT).bin $(BUILD)/$(PROJECT).hex size

uf2: $(BUILD)/$(PROJECT).uf2

OBJ_DIRS = $(sort $(dir $(OBJ)))
$(OBJ): | $(OBJ_DIRS)
$(OBJ_DIRS):
ifeq ($(CMDEXE),1)
	@$(MKDIR) $(subst /,\,$@)
else
	@$(MKDIR) -p $@
endif

$(BUILD)/$(PROJECT).elf: $(OBJ)
	@echo LINK $@
	@$(CXX) -o $@ $(LDFLAGS) $^ -Wl,--start-group $(LIBS) -Wl,--end-group

$(BUILD)/$(PROJECT).bin: $(BUILD)/$(PROJECT).elf
	@echo CREATE $@
	@$(OBJCOPY) -O binary $^ $@

$(BUILD)/$(PROJECT).hex: $(BUILD)/$(PROJECT).elf
	@echo CREATE $@
	@$(OBJCOPY) -O ihex $^ $@

# UF2 generation, iMXRT need to strip to text only before conversion
ifeq ($(FAMILY),imxrt)
$(BUILD)/$(PROJECT).uf2: $(BUILD)/$(PROJECT).elf
	@echo CREATE $@
	@$(OBJCOPY) -O ihex -R .flash_config -R .ivt $^ $(BUILD)/$(PROJECT)-textonly.hex
	$(PYTHON) tools/uf2/utils/uf2conv.py -f $(UF2_FAMILY_ID) -c -o $@ $(BUILD)/$(PROJECT)-textonly.hex
else
$(BUILD)/$(PROJECT).uf2: $(BUILD)/$(PROJECT).hex
	@echo CREATE $@
	$(PYTHON) tools/uf2/utils/uf2conv.py -f $(UF2_FAMILY_ID) -c -o $@ $^
endif

copy-artifact: $(BUILD)/$(PROJECT).bin $(BUILD)/$(PROJECT).hex $(BUILD)/$(PROJECT).uf2

# We set vpath to point to the top of the tree so that the source files
# can be located. By following this scheme, it allows a single build rule
# to be used to compile all .c files.

$(BUILD)/obj/%.o: %.c
	@echo CC $(notdir $@)
	@$(CC) $(CFLAGS) -c -MD -o $@ $<

# ASM sources .cpp
$(BUILD)/obj/%.o: %.cpp
	@echo CC $(notdir $@)
	@$(CXX) $(CPPFLAGS) -c -MD -o $@ $<

# ASM sources lower case .s
$(BUILD)/obj/%_asm.o: %.s
	@echo AS $(notdir $@)
	@$(CC) -x assembler-with-cpp $(ASFLAGS) -c -o $@ $<

# ASM sources upper case .S
$(BUILD)/obj/%_asm.o: %.S
	@echo AS $(notdir $@)
	@$(CC) -x assembler-with-cpp $(ASFLAGS) -c -o $@ $<

size: $(BUILD)/$(PROJECT).elf
	-@echo ''
	@$(SIZE) $<
	-@echo ''

.PHONY: clean
clean:
ifeq ($(CMDEXE),1)
	rd /S /Q $(subst /,\,$(BUILD))
else
	$(RM) -rf $(BUILD)
endif

endif
endif
endif # GNU Make

# ---------------------------------------
# Flash Targets
# ---------------------------------------

# Flash binary using Jlink
ifeq ($(OS),Windows_NT)
  JLINKEXE = JLink.exe
else
  JLINKEXE = JLinkExe
endif

JLINK_IF ?= swd

# Flash using jlink
flash-jlink: $(BUILD)/$(PROJECT).hex
	@echo halt > $(BUILD)/$(DEVICE).jlink
	@echo r > $(BUILD)/$(DEVICE).jlink
	@echo loadfile $^ >> $(BUILD)/$(DEVICE).jlink
	@echo r >> $(BUILD)/$(DEVICE).jlink
	@echo go >> $(BUILD)/$(DEVICE).jlink
	@echo exit >> $(BUILD)/$(DEVICE).jlink
	$(JLINKEXE) -device $(JLINK_DEVICE) -if $(JLINK_IF) -JTAGConf -1,-1 -speed auto -CommandFile $(BUILD)/$(DEVICE).jlink

# flash STM32 MCU using stlink with STM32 Cube Programmer CLI
flash-stlink: $(BUILD)/$(PROJECT).bin
	STM32_Programmer_CLI --connect port=swd --write $< $(FLASH_ADDRESS) --go

flash-stlink-elf: $(BUILD)/$(PROJECT).elf
	STM32_Programmer_CLI --connect port=swd --write $< --go

# flash with pyocd
flash-pyocd: $(BUILD)/$(PROJECT).hex
	pyocd flash -t $(PYOCD_TARGET) $<
	pyocd reset -t $(PYOCD_TARGET)

# flash with Black Magic Probe

# This symlink is created by https://github.com/blacksphere/blackmagic/blob/master/driver/99-blackmagic.rules
BMP ?= /dev/ttyBmpGdb

flash-bmp: $(BUILD)/$(PROJECT).elf
	$(GDB) --batch -ex 'target extended-remote $(BMP)' -ex 'monitor swdp_scan' -ex 'attach 1' -ex load  $<

debug-bmp: $(BUILD)/$(PROJECT).elf
	$(GDB) -ex 'target extended-remote $(BMP)' -ex 'monitor swdp_scan' -ex 'attach 1' $<

#-------------- Artifacts --------------

# Create binary directory
$(BIN):
	@$(Founde) -p $@

# Copy binaries .elf, .bin, .hex, .uf2 to BIN for upload
# due to large size of combined artifacts, only uf2 is uploaded for now
copy-artifact: $(BIN)
	@$(CP) $(BUILD)/$(PROJECT).uf2 $(BIN)
	#@$(CP) $(BUILD)/$(PROJECT).bin $(BIN)
	#@$(CP) $(BUILD)/$(PROJECT).hex $(BIN)
	#@$(CP) $(BUILD)/$(PROJECT).elf $(BIN)

# Print out the value of a make variable.
# https://stackoverflow.com/questions/16467718/how-to-print-out-a-variable-in-makefile
print-%:
	@echo $* = $($*)
