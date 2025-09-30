#pragma once

#include "LogLevel.h"

#ifdef RELEASE_BUILD
    #define MATRIXOS_BUILD_RELEASE
#elif defined RELEASE_CANDIDATE_BUILD
    #define MATRIXOS_BUILD_RELEASE_CANDIDATE
#elif defined BETA_BUILD
    #define MATRIXOS_BUILD_BETA
#elif defined NIGHTY_BUILD
    #define MATRIXOS_BUILD_NIGHTY
#elif defined DEVELOPMENT_BUILD
    #define MATRIXOS_BUILD_INDEV
#else
    // #define MATRIXOS_BUILD_RELEASE
    // #define MATRIXOS_BUILD_RELEASE_CANDIDATE
    // #define MATRIXOS_BUILD_BETA
    // #define MATRIXOS_BUILD_NIGHTY
    #define MATRIXOS_BUILD_INDEV
#endif

#define MATRIXOS_MAJOR_VER 3
#define MATRIXOS_MINOR_VER 0
#define MATRIXOS_PATCH_VER 3
#define MATRIXOS_RELEASE_VER 0 //This is for beta etc, doesn't take effect in Stable Release. It should also never be 0

#include "ReleaseConfig.h"

#define MATRIXOS_LOG_DEVICE
#define MATRIXOS_LOG_USBCDC
#define MATRIXOS_LOG_COLOR

#define APPLICATION_STACK_SIZE (configMINIMAL_STACK_SIZE * 32)

#define KEYEVENT_QUEUE_SIZE 16
#define MIDI_QUEUE_SIZE 128

inline const uint16_t hold_threshold = 400;

inline const uint16_t crossfade_duration = 200;