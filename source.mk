# ---------------------------------------
# Code Include
# ---------------------------------------
INC += src/
INC += devices/
INC += src/usb/ #Optimize for TinyUSB
INC += lib/tinyusb/src/
INC += lib/printf/

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
	lib\tinyusb\src\class\net\net_device.c \
	lib\tinyusb\src\class\usbtmc\usbtmc_device.c \
	lib\tinyusb\src\class\vendor\vendor_device.c \
	lib\printf\printf.c

# Include all source C in family & board folder
SRC_C += $(subst ,,$(wildcard $(BOARD_PATH)/*.c))
SRC_CPP += $(subst ,,$(wildcard $(BOARD_PATH)/*.cpp))
SRC_C += $(subst ,,$(wildcard $(FAMILY_PATH)/*.c))
SRC_CPP += $(subst ,,$(wildcard $(FAMILY_PATH)/*.cpp))

INC   += $(FAMILY_PATH)