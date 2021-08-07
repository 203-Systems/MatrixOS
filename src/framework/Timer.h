#pragma once

#include "MatrixOS.h"

class Timer
{
  public:
    Timer();
    bool Tick(uint32_t ms);
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
