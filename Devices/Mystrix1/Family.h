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

#define DEVICE_SAVED_VAR_SCOPE "Device"

// Factory configuration
// #define FACTORY_CONFIG //Global switch for using factory config
// #define FACTORY_DEVICE_VERSION 'S' // Standard
// #define FACTORY_DEVICE_VERSION 'P' // Pro
#ifdef FACTORY_CONFIG
#if FACTORY_DEVICE_VERSION == 'S'
#define FACTORY_DEVICE_MODEL {'M', 'X', '1', 'S'}
#elif FACTORY_DEVICE_VERSION == 'P'
#define FACTORY_DEVICE_MODEL {'M', 'X', '1', 'P'}
#else
#error "FACTORY_DEVICE_VERSION is not correct"
#endif
#endif

#define FACTORY_DEVICE_REVISION {'R', 'E', 'V', 'C'}

#define FACTORY_MFG_YEAR 23
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

namespace HWMidi
{
inline gpio_num_t txGpio = GPIO_NUM_NC;
inline gpio_num_t rxGpio = GPIO_NUM_NC;
} // namespace HWMidi

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
inline bool velocitySensitivity = false;

inline KeyScanConfig binaryConfig = {
    .applyCurve = false,
    .lowThreshold = 0,
    .highThreshold = 65535,
    .activationOffset = 0,
    .debounce = 3,
};

inline KeyScanConfig keypadConfig = {
    .applyCurve = true,
    .lowThreshold = 1536,
    .highThreshold = 32767,
    .activationOffset = 256,
    .debounce = 10,
};

inline gpio_num_t keypadWritePins[X_SIZE];
inline gpio_num_t keypadReadPins[Y_SIZE];
inline adc_channel_t keypadReadAdcChannel[Y_SIZE];

inline uint16_t keypadScanrate = 240;

inline gpio_num_t touch_data_pin;
inline gpio_num_t touch_clock_pin;

inline const uint8_t touchbarSize = 16;
inline const uint16_t touchbarScanrate = 120;
inline uint8_t touchbarMap[touchbarSize]; // Touch number as index and touch location as value (Left touch down
                                            // and then right touch down)

inline KeypadInfo fnState = {};
inline KeypadInfo keypadState[X_SIZE][Y_SIZE] = {};
inline KeypadInfo touchbarState[touchbarSize] = {};

namespace Binary
{
void Init();
void Start();
bool Scan();
} // namespace Binary

namespace FSR
{
void Init();
void Start();
bool Scan();
} // namespace FSR

bool NotifyOS(InputId id, KeypadInfo* keyState); // Emits InputEvent directly via MatrixOS::Input::NewEvent()
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

namespace HWMidi
{
void Init();
}

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
