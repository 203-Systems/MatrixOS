MODE ?= UNDEFINED

ifeq ($(DEVICE),)
  $(error You must provide a DEVICE parameter with 'DEVICE=')
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
