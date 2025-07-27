#include "Timer.h"

Timer::Timer() {
  Timer::RecordCurrent();
}

bool Timer::Tick(uint32_t ms, bool continuous_mode) {

  if(ms == UINT32_MAX)
    return false;
  if (MatrixOS::SYS::Millis() < previous)
    previous = 0;

  if (Timer::IsLonger(ms))
  {
    if (continuous_mode)
    { previous += ms; }
    else
    { Timer::RecordCurrent(); }
    return true;
  }
  return false;
}

bool Timer::IsLonger(uint32_t ms) {
  return (previous + ms) <= MatrixOS::SYS::Millis();
}

uint32_t Timer::SinceLastTick() {
  return MatrixOS::SYS::Millis() - previous;
}

void Timer::RecordCurrent() {
  previous = MatrixOS::SYS::Millis();
}

// MicroTimer::MicroTimer()
// {
//   MicroTimer::RecordCurrent();
// }

// bool MicroTimer::Tick(uint32_t ms)
// {
//   if(micros() < previous)
//     previous = 0;

//   if(MicroTimer::IsLonger(ms))
//   {
//     MicroTimer::RecordCurrent();
//     return true;
//   }
//   return false;
// }

// bool MicroTimer::IsLonger(uint32_t ms)
// {
//   return (previous + ms) <= micros();
// }

// uint32_t MicroTimer::SinceLastTick()
// {
//   return micros() - previous;
// }

// void MicroTimer::RecordCurrent()
// {
//   previous = micros();
// }
