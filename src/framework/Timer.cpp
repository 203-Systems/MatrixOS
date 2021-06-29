#include "Timer.h"

Timer::Timer()
{
  Timer::recordCurrent();
}

bool Timer::tick(u32 ms)
{
  if(MatrixOS::SYS::Millis() < previous)
    previous = 0;

  if(Timer::isLonger(ms))
  {
    Timer::recordCurrent();
    return true;
  }
  return false;
}

bool Timer::isLonger(u32 ms)
{
  return (previous + ms) <= MatrixOS::SYS::Millis();
}

u32 Timer::sinceLastTick()
{
  return MatrixOS::SYS::Millis() - previous;
}

void Timer::recordCurrent()
{
  previous = MatrixOS::SYS::Millis();
}

// MicroTimer::MicroTimer()
// {
//   MicroTimer::recordCurrent();
// }

// bool MicroTimer::tick(u32 ms)
// {
//   if(micros() < previous)
//     previous = 0;

//   if(MicroTimer::isLonger(ms))
//   {
//     MicroTimer::recordCurrent();
//     return true;
//   }
//   return false;
// }

// bool MicroTimer::isLonger(u32 ms)
// {
//   return (previous + ms) <= micros();
// }

// u32 MicroTimer::sinceLastTick()
// {
//   return micros() - previous;
// }

// void MicroTimer::recordCurrent()
// {
//   previous = micros();
// }
