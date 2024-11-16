#pragma once

#include "applications/BootAnimation/BootAnimation.h"
#include "esp_sleep.h"

#define MATRIX_BOOT_BRIGHTNESS 1.0  // On Top of system brightness
#define MATRIX_BOOT_IDLE 0.25         // On Top of system brightness

class MystrixBoot : public Application{
 public:
 static Application_Info info; ;

  // CreateSavedVar(bool, notFirstBoot, false);
  Timer timer;

  Point origin = Point((Device::x_size - 1) / 2, (Device::y_size - 1) / 2);

  enum BootPhase
  {
    INIT,
    SHOW_BATTERY,
    WAITING_FOR_USB,
    MANUAL_START,
    USB_CONNECTED,
    EXPLODE,
  };

  const float battery_step = 22.5;

  BootPhase boot_phase = INIT;

  bool prevChargingState = false;
  uint32_t batteryChargingEffectStartTime = 0;
  float batteryPercentageCache = 0; // Prevent battery level change during boot animation

  uint32_t phase_start_time = 0;
  float hueList[8][2] = {
      {0.5f, 0.833f},  // Pro - Pink Cyan
      {0.5f, 0.167f}    // Standard - Yellow Cyan
  };

  int16_t manual_boot_progress = 0;

  void Setup() override;
  void Loop() override;
  bool Idle(bool ready);
  void Boot();
  uint8_t GetBatteryLevel(float percentage);
  void RenderBattery(float percentage, bool charging, uint8_t brightness);
  void BootPhaseBattery();
  void BootPhaseWaitingForUSB();
  void RenderSquareFill(float percentage);
  void BootPhaseManualStart();
  void BootPhaseUSBConnected();
  void BootPhaseExplode();
  Color BootPhaseExplodeColor(int16_t time, float hue);
  void BootPhaseExplodeQuadSetColor(uint8_t x_offset, uint8_t y_offset, Color color1, Color color2);

  void End();
};

inline Application_Info MystrixBoot::info = {
    .name = "Mystrix Boot",
    .author = "203 Systems",
    .color =  Color(0xFFFFFFFF),
    .version = 1,
    .visibility = false,
};

REGISTER_APPLICATION(MystrixBoot);