FREERTOS_SRC = lib/FreeRTOS-Kernel

# ---------------------------------------
# Code Include
# ---------------------------------------
INC += src/ \
	devices/ \
	src/usb/ \
	lib/tinyusb/src/ \
	lib/printf/ \
	$(FREERTOS_SRC)/include \
	$(FREERTOS_SRC)/portable/GCC/$(FREERTOS_PORT)

# ---------------------------------------
# Code Source
# ---------------------------------------
SRC_CPP += main.cpp
SRC_C += $(wildcard src/*.c src/*/*.c src/*/*/*.c src/*/*/*/*.c src/*/*/*/*/*.c) #Lazy solution for recursive find. I gived up tring to find something cleaner
SRC_CPP += $(wildcard src/*.cpp src/*/*.cpp src/*/*/*.cpp src/*/*/*/*.cpp src/*/*/*/*/*.cpp) #Same as above

# Library source
SRC_C += \
	lib\tinyusb\src\tusb.c \
	lib\tinyusb\src\common\tusb_fifo.c \
	lib\tinyusb\src\device\usbd.c \
	lib\tinyusb\src\device\usbd_control.c \
	lib\tinyusb\src\class\audio\audio_device.c \
	lib\tinyusb\src\class\cdc\cdc_device.c \
	lib\tinyusb\src\class\dfu\dfu_device.c \
	lib\tinyusb\src\class\dfu\dfu_rt_device.c \
	lib\tinyusb\src\class\hid\hid_device.c \
	lib\tinyusb\src\class\midi\midi_device.c \
	lib\tinyusb\src\class\msc\msc_device.c \
	lib\tinyusb\src\class\net\ecm_rndis_device.c \
	lib\tinyusb\src\class\net\ncm_device.c \
	lib\tinyusb\src\class\usbtmc\usbtmc_device.c \
	lib\tinyusb\src\class\video\video_device.c \
	lib\tinyusb\src\class\vendor\vendor_device.c \
	lib\printf\printf.c

# Include all source C in family & device folder
SRC_C += $(subst ,,$(wildcard $(DEVICE_PATH)/*.c))
SRC_CPP += $(subst ,,$(wildcard $(DEVICE_PATH)/*.cpp))
SRC_C += $(subst ,,$(wildcard $(FAMILY_PATH)/drivers/*.c))
SRC_CPP += $(subst ,,$(wildcard $(FAMILY_PATH)/drivers/*.cpp))
SRC_C += $(subst ,,$(wildcard $(FAMILY_PATH)/*.c))
SRC_CPP += $(subst ,,$(wildcard $(FAMILY_PATH)/*.cpp))

# FreeRTOS source, all files in port folder
SRC_C += \
	$(FREERTOS_SRC)/list.c \
	$(FREERTOS_SRC)/queue.c \
	$(FREERTOS_SRC)/tasks.c \
	$(FREERTOS_SRC)/timers.c \
	$(subst ,,$(wildcard $(FREERTOS_SRC)/portable/GCC/$(FREERTOS_PORT)/*.c))

INC   += $(FAMILY_PATH)