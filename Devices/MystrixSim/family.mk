WEB_UI_DIR := Devices/MystrixSim/WebUI
WEB_PUBLIC_DIR := $(WEB_UI_DIR)/public
WEB_OUTPUT_JS := $(BUILD)/Devices/MystrixSim/MatrixOSHost.js
WEB_OUTPUT_WASM := $(BUILD)/Devices/MystrixSim/MatrixOSHost.wasm

EMCMAKE ?= emcmake
CMAKE ?= cmake
WEB_CMAKE_GENERATOR ?= Unix Makefiles
MYXSIL_DEV_HELPER := powershell -NoProfile -ExecutionPolicy Bypass -File "$(FAMILY_PATH)/tools/dev-helper.ps1"

EXTRA_CMAKE_ARGS :=
ifneq ($(strip $(RELEASE_VER)),)
EXTRA_CMAKE_ARGS += -DMATRIXOS_RELEASE_VER_OVERRIDE=$(RELEASE_VER)
endif

.PHONY: setup build configure web-copy run

setup:
ifeq ($(OS),Windows_NT)
	$(MYXSIL_DEV_HELPER) setup -RepoRoot "$(CURDIR)" -BuildDir "$(BUILD)" -WebUiDir "$(WEB_UI_DIR)" -Family "$(FAMILY)" -Device "$(DEVICE)" -Mode "$(MODE)" -ReleaseVer "$(RELEASE_VER)" -Generator "$(WEB_CMAKE_GENERATOR)"
else
	@command -v emcmake >/dev/null 2>&1 || { echo "MystrixSim requires Emscripten (emcmake/emcc) in PATH."; exit 1; }
	npm --prefix $(WEB_UI_DIR) install
endif

build:
ifeq ($(OS),Windows_NT)
	$(MYXSIL_DEV_HELPER) build -RepoRoot "$(CURDIR)" -BuildDir "$(BUILD)" -WebUiDir "$(WEB_UI_DIR)" -Family "$(FAMILY)" -Device "$(DEVICE)" -Mode "$(MODE)" -ReleaseVer "$(RELEASE_VER)" -Generator "$(WEB_CMAKE_GENERATOR)"
else
	$(MAKE) configure
	$(CMAKE) --build $(BUILD)
	$(MAKE) web-copy
endif

configure:
ifeq ($(OS),Windows_NT)
	$(MYXSIL_DEV_HELPER) configure -RepoRoot "$(CURDIR)" -BuildDir "$(BUILD)" -WebUiDir "$(WEB_UI_DIR)" -Family "$(FAMILY)" -Device "$(DEVICE)" -Mode "$(MODE)" -ReleaseVer "$(RELEASE_VER)" -Generator "$(WEB_CMAKE_GENERATOR)"
else
	$(EMCMAKE) $(CMAKE) -S . -B $(BUILD) -DFAMILY=$(FAMILY) -DDEVICE=$(DEVICE) -DMODE=$(MODE) $(EXTRA_CMAKE_ARGS) -G "$(WEB_CMAKE_GENERATOR)"
endif

web-copy:
	$(CMAKE) -E make_directory $(WEB_PUBLIC_DIR)
	$(CMAKE) -E copy_if_different $(WEB_OUTPUT_JS) $(WEB_PUBLIC_DIR)/MatrixOSHost.js
	$(CMAKE) -E copy_if_different $(WEB_OUTPUT_WASM) $(WEB_PUBLIC_DIR)/MatrixOSHost.wasm

run:
ifeq ($(OS),Windows_NT)
	$(MYXSIL_DEV_HELPER) run -RepoRoot "$(CURDIR)" -BuildDir "$(BUILD)" -WebUiDir "$(WEB_UI_DIR)" -Family "$(FAMILY)" -Device "$(DEVICE)" -Mode "$(MODE)" -Generator "$(WEB_CMAKE_GENERATOR)"
else
	npm --prefix $(WEB_UI_DIR) run dev
endif
