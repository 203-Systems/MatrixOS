#pragma once

#include "BootAnimation/BootAnimation.h" // TODO Need to be fixed

#define MATRIX_BOOT_BRIGHTNESS 1.0  // On Top of system brightness
#define MATRIX_BOOT_IDLE 0.25         // On Top of system brightness

class MystrixBoot : public BootAnimation {
 public:
 inline static Application_Info info = {
     .name = "Mystrix Boot",
     .author = "203 Systems",
     .color =  Color(0xFFFFFFFF),
     .version = 1,
     .visibility = false,
 };

  // CreateSavedVar(bool, notFirstBoot, false);
  Timer timer;

  Point origin = Point((X_SIZE - 1) / 2, (Y_SIZE - 1) / 2);
  uint8_t counter;

  uint8_t boot_phase;
  uint32_t boot_phase_1_tick_time = 0;
  uint32_t boot_phase_2_start_time = 0;
  float hueList[8][2] = {
      {0.5f, 0.833f},  // Pro - Pink Cyan
      {0.5f, 0.167f}    // Standard - Yellow Cyan
  };

  void Setup(const vector<string>& args) override;
  bool Idle(bool ready) override;
  void Boot() override;
  void BootPhase1();
  void BootPhase2();
  Color BootPhase2Color(int16_t time, float hue);
  void BootPhase2QuadSetColor(uint8_t x_offset, uint8_t y_offset, Color color1, Color color2);

  void End();
};


