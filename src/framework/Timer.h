#ifndef __TIMER_H
#define __TIMER_H

#include "MatrixOS.h"

class Timer
{
  public:
    Timer();
    bool Tick(u32 ms);
    bool IsLonger(u32 ms);
    u32 SinceLastTick();
    void RecordCurrent();
  private:
    u32 previous = 0;
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

#endif
