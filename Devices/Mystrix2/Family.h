// Declear Family specific function
#pragma once

#include "Device.h"

#include "Framework.h"

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

#define ESP_PLATFORM 1

// Family-specific defines
#define GRID_TYPE_8x8
#define FAMILY_MYSTRIX
#define MULTIPRESS 10 // Key Press will be process at once
#define TOUCHBAR_SIZE 16

#define DEVICE_SAVED_VAR_SCOPE "Device"

// Factory configuration
// #define FACTORY_CONFIG //Global switch for using factory config
// #define FACTORY_DEVICE_VERSION 'P' // Pro
// #define FACTORY_DEVICE_VERSION 'U' // Ultra
#ifdef FACTORY_CONFIG
#if FACTORY_DEVICE_VERSION == 'P'
#define FACTORY_DEVICE_MODEL {'M', 'X', '2', 'P'}
#elif FACTORY_DEVICE_VERSION == 'U'
#define FACTORY_DEVICE_MODEL {'M', 'X', '2', 'U'}
#else
#error "FACTORY_DEVICE_VERSION is not correct"
#endif
#endif

#define FACTORY_DEVICE_REVISION {'U', 'P', 'T', '2'}

#define FACTORY_MFG_YEAR 26
#define FACTORY_MFG_MONTH 03

struct DeviceInfo {
  char model[4];
  char revision[4];
  uint8_t productionYear;
  uint8_t productionMonth;
};

namespace Device
{
inline DeviceInfo deviceInfo;

// Device Variable
inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, touchbarEnable, bool, true);
inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, bluetooth, bool, false);

inline gpio_num_t i2cSdaPin;
inline gpio_num_t i2cSclPin;

namespace LED
{
inline gpio_num_t ledPin;
}

void LoadDeviceInfo();
void LoadVariantInfo();

namespace USB
{
void Init();
}
namespace LED
{
void Init();
void Start();
} // namespace LED

namespace KeyPad
{
enum KeypadType { BinaryKeypad, FSRKeypad, MPEKeypad };

void Init();
void InitFN();
void InitKeyPad();
void InitTouchBar();

void Start();
void StartKeyPad();
void StartTouchBar();

// If return true, meaning the scan in interrupted
void Scan();
bool ScanKeyPad();
bool ScanFN();
bool ScanTouchBar();

inline gpio_num_t fnPin;
inline bool velocitySensitivity = true;
inline KeypadType keypadType = KeypadType::MPEKeypad;

inline KeyConfig binaryConfig = {
    .applyCurve = false,
    .lowThreshold = 0,
    .highThreshold = 65535,
    .activationOffset = 0,
    .debounce = 3,
};

inline KeyConfig keypadConfig = {
    .applyCurve = true,
    .lowThreshold = 24000,
    .highThreshold = 53248,
    .activationOffset = 1024,
    .debounce = 5,
};

inline KeyConfig mpeConfig = {
    .applyCurve = true,
    .lowThreshold = 40000,
    .highThreshold = 53248,
    .activationOffset = 2048,
    .debounce = 5,
};

inline const uint16_t keypadScanrate = 240;
inline const uint16_t touchbarScanrate = 120;

inline KeyInfo fnState;
inline KeyInfo keypadState[X_SIZE][Y_SIZE];
inline uint16_t padForce[X_SIZE][Y_SIZE] = {};
inline KeyInfo touchbarState[TOUCHBAR_SIZE]; // Virtual 16 keys to be backward compatible with Mystrix 1 apps

namespace FSR
{
void Init();
void Start();
bool Scan();
} // namespace FSR

namespace MPE
{
void Init();
void Start();
bool Scan();
} // namespace MPE

bool NotifyOS(uint16_t keyID, KeyInfo* keyInfo); // Emits InputEvent directly via MatrixOS::Input::NewEvent()
} // namespace KeyPad

namespace NVS
{
void Init();
}

namespace WIFI
{
void Init();
}

namespace BLEMIDI
{
extern bool started;
void Init(string name);
void Start();
void Stop();
} // namespace BLEMIDI

namespace Storage
{
inline gpio_num_t sdClkPin;
inline gpio_num_t sdCmdPin;
inline gpio_num_t sdD0Pin;
inline gpio_num_t sdD1Pin;
inline gpio_num_t sdD2Pin;
inline gpio_num_t sdD3Pin;
inline gpio_num_t sdDetPin;

inline bool sd4BitMode;
inline uint32_t sdFreqKhz;

void Init();
} // namespace Storage
} // namespace Device
