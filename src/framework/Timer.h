#ifndef __TIMER_H
#define __TIMER_H

#include "../system/MatrixOS.h"

class Timer
{
public:
  Timer();
  bool tick(u32 ms);
  bool isLonger(u32 ms);
  u32 sinceLastTick();
  void recordCurrent();
private:
  u32 previous = 0;
};

// class MicroTimer
// {
// public:
//   MicroTimer();
//   bool tick(u32 ms);
//   bool isLonger(u32 ms);
//   u32 sinceLastTick();
//   void recordCurrent();
// private:
//   u32 previous = 0;
// };

#endif
