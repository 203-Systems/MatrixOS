WEB_UI_DIR := Devices/MystrixSIL/web-ui
WEB_PUBLIC_DIR := $(WEB_UI_DIR)/public
WEB_OUTPUT_JS := $(BUILD)/Devices/MystrixSIL/MatrixOSHost.js
WEB_OUTPUT_WASM := $(BUILD)/Devices/MystrixSIL/MatrixOSHost.wasm

EMCMAKE ?= emcmake
CMAKE ?= cmake
WEB_CMAKE_GENERATOR ?= Unix Makefiles

.PHONY: build configure web-copy run

build:
ifeq ($(wildcard $(BUILD)/CMakeCache.txt),)
	$(MAKE) configure
endif
	$(CMAKE) --build $(BUILD)
	$(MAKE) web-copy

configure:
	$(EMCMAKE) $(CMAKE) -S . -B $(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) -DMODE=$(MODE) -G "$(WEB_CMAKE_GENERATOR)"

web-copy:
	$(CMAKE) -E make_directory $(WEB_PUBLIC_DIR)
	$(CMAKE) -E copy_if_different $(WEB_OUTPUT_JS) $(WEB_PUBLIC_DIR)/MatrixOSHost.js
	$(CMAKE) -E copy_if_different $(WEB_OUTPUT_WASM) $(WEB_PUBLIC_DIR)/MatrixOSHost.wasm

run:
	npm --prefix $(WEB_UI_DIR) run dev
