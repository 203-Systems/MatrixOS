#pragma once

/*
 * Device Feature Configuration
 * 
 * This file contains default values for device feature flags.
 * Device implementations can override these by defining the macros
 * before including Device.h or in their device-specific headers.
 */

// FileSystem Support
#ifndef DEVICE_STORAGE
#define DEVICE_STORAGE 0
#endif

// Battery Support
#ifndef DEVICE_BATTERY
#define DEVICE_BATTERY 0
#endif

// Function Key Support
#ifndef FUNCTION_KEY
#define FUNCTION_KEY 0
#endif

// Default Shell Application
#ifndef OS_SHELL
#define OS_SHELL APPID("203 Systems", "Shell")
#endif

// Boot Animation
#ifndef DEFAULT_BOOTANIMATION
#define DEFAULT_BOOTANIMATION 0
#endif