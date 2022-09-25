#pragma once

#include "framework/LogLevel.h"

// #define MATRIXOS_BUILD_RELEASE
#define MATRIXOS_BUILD_RELEASE_CANDIDATE
// #define MATRIXOS_BUILD_BETA
// #define MATRIXOS_BUILD_NIGHTY
// #define MATRIXOS_BUILD_INDEV

#define MATRIXOS_MAJOR_VER 2
#define MATRIXOS_MINOR_VER 1
#define MATRIXOS_PATCH_VER 1

#include "ReleaseConfig.h"

#define MATRIXOS_LOG_DEVICE
#define MATRIXOS_LOG_USBCDC
#define MATRIXOS_LOG_COLOR

#define APPLICATION_STACK_SIZE     (configMINIMAL_STACK_SIZE * 16)

#define MATRIXOS_FLASHVERSION 0 //Each Flash data strcture change will cause this to increase 

inline uint16_t hold_threshold = 400;

// enum class EVarClass {DeviceVar, SystemVar, UserVar, AppVar};

