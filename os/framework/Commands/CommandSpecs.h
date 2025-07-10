#pragma once

// This is the system byte command format used by SYSEX and HID.

// Specific commands from each protocol depends on the protocol itself. Each will be slightly different, as midi only has 7bits. HID packet size is fixed.
// But command ID should be consistent across all protocols.
// Each protocol can also opt out of implementing some of the commands.

// System Command Byte Format
#define MATRIXOS_COMMAND_GET_OS_VERSION 0x00 // Returns [MATRIXOS_COMMAND_GET_VERSION, MATRIXOS_MINOR_VER, MATRIXOS_MAJOR_VER, MATRIXOS_PATCH_VER, MATRIXOS_BUILD_VER]
#define MATRIXOS_COMMAND_GET_DEVICE_NAME 0x01 // Returns [MATRIXOS_COMMAND_GET_DEVICE_ID, Device::device_name] device name as null terminated array
#define MATRIXOS_COMMAND_GET_DEVICE_MODEL_ID 0x02 // Returns [MATRIXOS_COMMAND_GET_DEVICE_MODEL_ID, Device::model_id] model id as null terminated array
#define MATRIXOS_COMMAND_GET_DEVICE_SERIAL 0x03 // Returns [MATRIXOS_COMMAND_GET_DEVICE_SERIAL, Device::serial_number] serial number as null terminated array
#define MATRIXOS_COMMAND_GET_DEVICE_ID 0x04 // Returns [MATRIXOS_COMMAND_GET_DEVICE_ID, Device::device_id] device id as null terminated array
#define MATRIXOS_COMMAND_GET_DEVICE_SIZE 0x05 // Returns [MATRIXOS_COMMAND_GET_DEVICE_SIZE, Device::x_size, Device::y_size] device size as null terminated array
#define MATRIXOS_COMMAND_GET_DEVICE_LED_COUNT 0x06 // Returns [MATRIXOS_COMMAND_GET_DEVICE_LED_COUNT, Device::led_count] led count as null terminated array
#define MATRIXOS_COMMAND_SET_DEVICE_ID 0x0B // [MATRIXOS_COMMAND_SET_DEVICE_ID, device_id] Sets the device id

#define MATRIXOS_COMMAND_GET_APP_ID 0x10 // Returns [MATRIXOS_COMMAND_GET_APP_ID, app_id]
#define MATRIXOS_COMMAND_GET_APP_NAME 0x11 // Returns [MATRIXOS_COMMAND_GET_APP_NAME, app_name] app name as null terminated array
#define MATRIXOS_COMMAND_GET_APP_AUTHOR 0x12 // Returns [MATRIXOS_COMMAND_GET_APP_AUTHOR, app_author] app author as null terminated array
#define MATRIXOS_COMMAND_GET_APP_VERSION 0x13 // Returns [MATRIXOS_COMMAND_GET_APP_VERSION, app_version] app version as null terminated array
#define MATRIXOS_COMMAND_ENTER_APP_VIA_ID 0x18 // [MATRIXOS_COMMAND_ENTER_APP, app_id] Enters the app with the given app_id
#define MATRIXOS_COMMAND_QUIT_APP 0x1F // [MATRIXOS_COMMAND_QUIT_APP] Quits the current app

#define MATRIXOS_COMMAND_BOOTLOADER 0x40 // [MATRIXOS_COMMAND_BOOTLOADER] Enters the bootloader
#define MATRIXOS_COMMAND_REBOOT 0x41 // [MATRIXOS_COMMAND_REBOOT] Reboots the device
#define MATRIXOS_COMMAND_SLEEP 0x42 // [MATRIXOS_COMMAND_SLEEP] Puts the device to sleep
#define MATRIXOS_COMMAND_OPEN_SETTINGS 0x43 // [MATRIXOS_COMMAND_OPEN_SETTINGS] Opens the settings menu
#define MATRIXOS_COMMAND_FACTORY_RESET 0x4F // [MATRIXOS_COMMAND_FACTORY_RESET] Resets the device to factory settings.Require user confirmation. Return [MATRIXOS_COMMAND_FACTORY_RESET, wiped as 1 or canceled as 0]

#define MATRIXOS_COMMAND_LED_SET_COLOR_XY 0x50 // [MATRIXOS_COMMAND_LED_SET_COLOR_XY, x, y, color, layer as optional] Sets the color of the LED at x, y
#define MATRIXOS_COMMAND_LED_SET_COLOR_ID 0x51 // [MATRIXOS_COMMAND_LED_SET_COLOR_ID, id, color, layer as optional] Sets the color of the LED at id
#define MATRIXOS_COMMAND_LED_FILL 0x52 // [MATRIXOS_COMMAND_LED_FILL, color, layer as optional] Fills the entire screen with the color
#define MATRIXOS_COMMAND_LED_FILL_PARTITION 0x53 // [MATRIXOS_COMMAND_LED_FILL_PARTITION, partition, color, layer as optional] Fills the partition with the color. (Matrix OS api takes in the string name of the partition, I think we should just use index here. let's assume user knows the index. Or we take in hash)
#define MATRIXOS_COMMAND_LED_UPDATE 0x54 // [MATRIXOS_COMMAND_LED_UPDATE, layer as optional] Updates the LED screen
#define MATRIXOS_COMMAND_LED_CREATELAYER 0x55 // [MATRIXOS_COMMAND_LED_CREATELAYER, layer] Creates a new layer
#define MATRIXOS_COMMAND_LED_COPYLAYER 0x56 // [MATRIXOS_COMMAND_LED_COPYLAYER, from_layer, to_layer] Copies the content of from_layer to to_layer
#define MATRIXOS_COMMAND_LED_DESTROYLAYER 0x57 // [MATRIXOS_COMMAND_LED_DESTROYLAYER, layer] Destroys the layer
#define MATRIXOS_COMMAND_LED_SET_BRIGHTNESS 0x59 // [MATRIXOS_COMMAND_LED_SET_BRIGHTNESS, brightness] Sets the brightness of the screen
#define MATRIXOS_COMMAND_LED_FADE 0x5A // [MATRIXOS_COMMAND_LED_FADE, start_color, end_color, duration, layer as optional] Fades the screen from start color to end color in duration
#define MATRIXOS_COMMAND_GET_LED_CURRENT_LAYER 0x5C // Returns [MATRIXOS_COMMAND_LED_GET_CURRENT_LAYER, current_layer] Gets the current layer
#define MATRIXOS_COMMAND_GET_LED_BRIGHTNESS 0x5D // Returns [MATRIXOS_COMMAND_GET_BRIGHTNESS, brightness] Gets the brightness of the screen

#define MATRIXOS_COMMAND_KEYPAD_GET_KEY_XY 0x60 // [MATRIXOS_COMMAND_KEYPAD_GET_KEY_XY, x, y] Gets the key at x, y
#define MATRIXOS_COMMAND_KEYPAD_GET_KEY_ID 0x61 // [MATRIXOS_COMMAND_KEYPAD_GET_KEY_ID, id] Gets the key at id

// Followed by page specific commands IDs ,like MATRIXOS_COMMAND_PAGE_1 <command> <data>
#define MATRIXOS_COMMAND_PAGE_1 0x70 
#define MATRIXOS_COMMAND_PAGE_2 0x71
#define MATRIXOS_COMMAND_PAGE_3 0x72
#define MATRIXOS_COMMAND_PAGE_4 0x73
#define MATRIXOS_COMMAND_PAGE_5 0x74
#define MATRIXOS_COMMAND_PAGE_6 0x75
#define MATRIXOS_COMMAND_PAGE_7 0x76
#define MATRIXOS_COMMAND_PAGE_8 0x77
#define MATRIXOS_COMMAND_PAGE_9 0x78
#define MATRIXOS_COMMAND_PAGE_10 0x79
#define MATRIXOS_COMMAND_PAGE_11 0x7A
#define MATRIXOS_COMMAND_PAGE_12 0x7B
#define MATRIXOS_COMMAND_PAGE_13 0x7C
#define MATRIXOS_COMMAND_PAGE_14 0x7D
#define MATRIXOS_COMMAND_PAGE_15 0x7E
#define MATRIXOS_COMMAND_PAGE_16 0x7F 
