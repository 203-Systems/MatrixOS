#pragma once

#include <stdint.h>

// Avoid recuesive include
namespace MatrixOS::SYS
{
  uint32_t Millis(void);
}

class Timer {
 public:
  Timer();
  bool Tick(uint32_t ms, bool continuous_mode = false);
  bool IsLonger(uint32_t ms);
  uint32_t SinceLastTick();
  void RecordCurrent();

 private:
  uint32_t previous = 0;
};

// class MicroTimer
// {
// public:
//   MicroTimer();
//   bool Tick(u32 ms);
//   bool IsLonger(u32 ms);
//   u32 SinceLastTick();
//   void RecordCurrent();
// private:
//   u32 previous = 0;
// };
