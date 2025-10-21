// Declare Family specific function
#pragma once

#include "Device.h"
#include "Framework.h"

#include "stm32f1xx_hal.h"
#include "WS2812/WS2812.h"

#undef USB  // CMSIS defined the USB, undef so we can use USB as MatrixOS namespace

// Family-specific defines
#define GRID_TYPE_8x8
#define FAMILY_MATRIXBLOCK5
#define MULTIPRESS 10  // Key Press will be process at once

#define DEVICE_SAVED_VAR_SCOPE "Device"

struct DeviceInfo {
  char Model[4];
  char Revision[4];
  uint8_t ProductionYear;
  uint8_t ProductionMonth;
};

namespace Device
{
  inline DeviceInfo deviceInfo;

  // Device Variable
  inline CreateSavedVar(DEVICE_SAVED_VAR_SCOPE, touchbar_enable, bool, true);

  void LoadDeviceInfo();
  void LoadVariantInfo();
  
  void SystemClock_Config();

  namespace USB
  {
    void Init();
  }

  namespace LED
  {
    inline GPIO_TypeDef* led_port = nullptr;
    inline uint16_t led_pin = 0;

    void Init();
    void Start();
  }

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

    inline GPIO_TypeDef* fn_port = nullptr;
    inline uint16_t fn_pin = 0;
    inline bool fn_active_low = true;
    inline bool velocity_sensitivity = false;

    inline KeyConfig keypad_config = {
        .apply_curve = false,
        .low_threshold = 0,
        .high_threshold = 65535,
        .activation_offset = 0,
        .debounce = 3,
    };

    inline GPIO_TypeDef* keypad_write_ports[X_SIZE];
    inline uint16_t keypad_write_pins[X_SIZE];
    inline GPIO_TypeDef* keypad_read_ports[Y_SIZE];
    inline uint16_t keypad_read_pins[Y_SIZE];

    inline uint16_t keypad_scanrate = 240;

    inline GPIO_TypeDef* touchData_Port;
    inline uint16_t touchData_Pin;
    inline GPIO_TypeDef* touchClock_Port;
    inline uint16_t touchClock_Pin;

    inline const uint8_t touchbar_size = 16;
    inline const uint16_t touchbar_scanrate = 120;
    inline uint8_t touchbar_map[touchbar_size];  // Touch number as index and touch location as value (Left touch down
                                                   // and then right touch down)

    inline KeyInfo fnState;
    inline KeyInfo keypadState[X_SIZE][Y_SIZE];
    inline KeyInfo touchbarState[touchbar_size];

    bool NotifyOS(uint16_t keyID, KeyInfo* keyInfo);  // Passthrough MatrixOS::KeyPad::NewEvent() result
  }

  namespace NVS
  {
    void Init();
  }
}