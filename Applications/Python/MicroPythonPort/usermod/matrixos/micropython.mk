MATRIXOS_MOD_DIR := $(USERMOD_DIR)
MATRIXOS_ROOT_DIR := ../../..
MATRIXOS_OS_INCLUDE_DIRS := \
	$(MATRIXOS_ROOT_DIR)/OS \
	$(wildcard $(MATRIXOS_ROOT_DIR)/OS/*/) \
	$(wildcard $(MATRIXOS_ROOT_DIR)/OS/*/*/)
MATRIXOS_QSTR_INCLUDES := \
	-I$(MATRIXOS_MOD_DIR) \
	-I. \
	-Ibuild-embed \
	-I../../../Library/micropython \
	-I../../../Library/micropython/ports/embed \
	$(addprefix -I,$(MATRIXOS_OS_INCLUDE_DIRS)) \
	-I$(MATRIXOS_ROOT_DIR)/Applications \
	-I$(MATRIXOS_ROOT_DIR)/Devices \
	-I$(MATRIXOS_ROOT_DIR)/Devices/MystrixSim \
	-I$(MATRIXOS_ROOT_DIR)/Devices/MystrixSim/FreeRTOS

SRC_USERMOD_CXX += \
	$(MATRIXOS_MOD_DIR)/../../matrixos_file.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_module.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_common.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_color.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_timer.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_sys.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_led.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_input.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_color_effects.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_nvs.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_utils.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_ui.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_ui_components.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_ui_utility.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_logging.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_filesystem.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_usb.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_hid.cpp \
	$(MATRIXOS_MOD_DIR)/matrixos_midi.cpp
CFLAGS_USERMOD += $(MATRIXOS_QSTR_INCLUDES)
CXXFLAGS_USERMOD += $(MATRIXOS_QSTR_INCLUDES)
