#pragma once

#include "framework/LogLevel.h"

// #define MATRIXOS_BUILD_RELEASE
#define MATRIXOS_BUILD_RELEASE_CANDIDATE
// #define MATRIXOS_BUILD_BETA
// #define MATRIXOS_BUILD_NIGHTY
// #define MATRIXOS_BUILD_INDEV

#define MATRIXOS_MAJOR_VER 2
#define MATRIXOS_MINOR_VER 0
#define MATRIXOS_PATCH_VER 3

#include "ReleaseConfig.h"

#define MATRIXOS_LOG_DEVICE
#define MATRIXOS_LOG_USBCDC
#define MATRIXOS_LOG_COLOR

#define APPLICATION_STACK_SIZE     (configMINIMAL_STACK_SIZE * 16)

#define MATRIXOS_FLASHVERSION 0 //Each Flash data strcture change will cause this to increase 

//KeyPad
#ifndef DEBOUNCE_THRESHOLD
inline uint16_t debounce_threshold = 10;
#else
inline uint16_t debounce_threshold = DEBOUNCE_THRESHOLD;
#endif

inline uint16_t hold_threshold = 400;

// enum class EVarClass {DeviceVar, SystemVar, UserVar, AppVar};

