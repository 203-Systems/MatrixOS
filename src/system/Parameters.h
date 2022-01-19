#pragma once

#include "framework/LogLevel.h"

#define MATRIXOS_LOG_LEVEL  LOG_LEVEL_DEBUG
#define MATRIXOS_LOG_DEVICE
// #define MATRIXOS_LOG_USBCDC
#define MATRIXOS_LOG_COLOR

#define MATRIXOS_FLASHVERSION 0 //Each Flash data strcture change will cause this to increase
#define MATRIXOS_VERSION_STRING "2.0 Dev" //String(MAJOR_VER)+ "." +MINOR_VER+"." +PATCH_VER+(BUILD_VER == 0)?"":("b"+BUILD_VER) 
#define MATRIXOS_MAJOR_VER 2
#define MATRIXOS_MINOR_VER 0
#define MATRIXOS_PATCH_VER 0
#define MATRIXOS_BUILD_VER 1 //0 for Release, any other number will repensent beta ver

//KeyPad
#ifndef DEBOUNCE_THRESHOLD
inline uint16_t debounce_threshold = 24;
#endif

inline uint16_t hold_threshold = 400;

