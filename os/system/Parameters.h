#pragma once

#include "framework/LogLevel.h"

// #define MATRIXOS_BUILD_RELEASE
// #define MATRIXOS_BUILD_RELEASE_CANDIDATE
// #define MATRIXOS_BUILD_BETA
// #define MATRIXOS_BUILD_NIGHTY
#define MATRIXOS_BUILD_INDEV

#define MATRIXOS_MAJOR_VER 2
#define MATRIXOS_MINOR_VER 5
#define MATRIXOS_PATCH_VER 0
#define MATRIXOS_RELEASE_VER 1 //This is for beta etc, doesn't take effect in Stable Release. It should also never be 0


#include "ReleaseConfig.h"

#define MATRIXOS_LOG_DEVICE
#define MATRIXOS_LOG_USBCDC
#define MATRIXOS_LOG_COLOR

#define APPLICATION_STACK_SIZE (configMINIMAL_STACK_SIZE * 16)

#define MATRIXOS_FLASHVERSION 0  // Each Flash data strcture change will cause this to increase

#define KEYEVENT_QUEUE_SIZE 16
#define MIDI_QUEUE_SIZE 128

#define USB_CDC_COUNT 0  // There will be one extra  used by the system, actual count is USB_CDC_COUNT + 1
#define USB_MIDI_COUNT 1
#define USB_HID_COUNT 0
#define USB_MSC_COUNT 0
#define USB_VENDOR_COUNT 0

inline uint16_t hold_threshold = 400;

// enum class EVarClass {DeviceVar, SystemVar, UserVar, AppVar};
