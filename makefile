MODE ?= UNDEFINED

PROJECT := MatrixOS

# Remember last used DEVICE and MODE
LAST_CONFIG_FILE := .last_config

# Load last config if it exists
ifneq ($(wildcard $(LAST_CONFIG_FILE)),)
  include $(LAST_CONFIG_FILE)
endif

# Use provided DEVICE or fall back to last used
ifeq ($(DEVICE),)
  ifdef LAST_DEVICE
    DEVICE := $(LAST_DEVICE)
    $(info Using last DEVICE: $(DEVICE))
  else
    $(error You must provide a DEVICE parameter with 'DEVICE=' on first run)
  endif
endif

# Use provided MODE or fall back to last used  
ifeq ($(MODE),UNDEFINED)
  ifdef LAST_MODE
    MODE := $(LAST_MODE)
    $(info Using last MODE: $(MODE))
  endif
endif

# Device without family
ifneq ($(wildcard ./Devices/$(DEVICE)/family.mk),)
FAMILY_PATH := Devices/$(DEVICE)
FAMILY := $(DEVICE)
endif

# Device within family
ifeq ($(FAMILY_PATH),)
  DEVICE_PATH := $(subst ,,$(wildcard Devices/*/Variants/$(DEVICE)))
  FAMILY := $(word 2, $(subst /, ,$(DEVICE_PATH)))
  FAMILY_PATH = Devices/$(FAMILY)
endif

ifeq ($(FAMILY_PATH),)
  $(error Unable to find the device $(DEVICE))
endif

BUILD := build/$(DEVICE)

include $(FAMILY_PATH)/family.mk

.PHONY: all build build-dev build-release build-beta build-rc build-nightly clean fullclean

all: save-config build

# Save config before building
save-config:
	@echo LAST_DEVICE := $(DEVICE) > $(LAST_CONFIG_FILE)
	@echo LAST_MODE := $(MODE) >> $(LAST_CONFIG_FILE)

build-dev:
	$(MAKE) MODE=DEVELOPMENT save-config build

build-release:
	$(MAKE) MODE=RELEASE save-config build

build-beta:
	$(MAKE) MODE=BETA save-config build

build-rc:
	$(MAKE) MODE=RELEASECANDIDATE save-config build

build-nightly:
	$(MAKE) MODE=NIGHTLY save-config build

clean:
ifeq ($(OS),Windows_NT)
	if exist "$(BUILD)" rmdir /s /q "$(BUILD)"
else
	rm -rf $(BUILD)
endif

fullclean:
ifeq ($(OS),Windows_NT)
	if exist "build" rmdir /s /q "build"
else
	rm -rf build
endif
