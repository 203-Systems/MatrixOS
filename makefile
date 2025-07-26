MODE ?= UNDEFINED

PROJECT := MatrixOS

# Remember last used DEVICE and MODE
LAST_CONFIG_FILE := .last_config

$(info Checking for config file: $(LAST_CONFIG_FILE))
$(info Wildcard result: $(wildcard $(LAST_CONFIG_FILE)))

# Load last config if it exists
ifneq ($(wildcard $(LAST_CONFIG_FILE)),)
  $(info Config file exists, including it...)
  include $(LAST_CONFIG_FILE)
  $(info After include: LAST_DEVICE=$(LAST_DEVICE) LAST_MODE=$(LAST_MODE))
else
  $(info Config file does not exist)
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

# Save current values for next time
$(shell echo LAST_DEVICE := $(DEVICE) > $(LAST_CONFIG_FILE))
$(shell echo LAST_MODE := $(MODE) >> $(LAST_CONFIG_FILE))

.PHONY: all build build-dev build-release build-beta build-rc build-nightly

all: build

build-dev:
	$(MAKE) MODE=DEVELOPMENT build

build-release:
	$(MAKE) MODE=RELEASE build

build-beta:
	$(MAKE) MODE=BETA build

build-rc:
	$(MAKE) MODE=RELEASECANDIDATE build

build-nightly:
	$(MAKE) MODE=NIGHTLY build
