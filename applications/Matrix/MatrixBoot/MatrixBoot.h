#pragma once

#include "applications/BootAnimation/BootAnimation.h"

#define MATRIX_BOOT_BRIGHTNESS 255  // On Top of system brightness
#define MATRIX_BOOT_IDLE 64         // On Top of system brightness

class MatrixBoot : public BootAnimation {
 public:
 static Application_Info info; 

  string name = "Matrix Boot";
  string author = "203 Electronics";
  uint32_t version = 0;

  Timer timer;

  Point origin = Point((Device::x_size - 1) / 2, (Device::y_size - 1) / 2);
  uint8_t counter;

  uint8_t boot_phase;
  uint32_t boot_phase_1_tick_time = 0;
  uint32_t boot_phase_2_start_time = 0;
  // Color colorList[5] = {Color(64, 64, 64), Color(127, 0, 0), Color(0, 127, 0), Color(0, 0, 127), Color(0, 0, 0)};

  void Setup() override;
  bool Idle(bool ready) override;
  void Boot() override;
  void BootPhase1();
  void BootPhase2();
  Color BootPhase2Color(int16_t time, uint8_t hue);
  void BootPhase2QuadSetColor(uint8_t x_offset, uint8_t y_offset, Color color1, Color color2);

  void End();
};

inline Application_Info MatrixBoot::info = {
    .name = "Matrix Boot",
    .author = "203 Electronics",
    .color =  Color(0xFFFFFFFF),
    .version = 1,
    .visibility = false,
};

REGISTER_APPLICATION(MatrixBoot);