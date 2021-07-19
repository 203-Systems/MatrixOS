#include "Timer.h"

Timer::Timer()
{
  Timer::RecordCurrent();
}

bool Timer::Tick(u32 ms)
{
  if(MatrixOS::SYS::Millis() < previous)
    previous = 0;

  if(Timer::IsLonger(ms))
  {
    Timer::RecordCurrent();
    return true;
  }
  return false;
}

bool Timer::IsLonger(u32 ms)
{
  return (previous + ms) <= MatrixOS::SYS::Millis();
}

u32 Timer::SinceLastTick()
{
  return MatrixOS::SYS::Millis() - previous;
}

void Timer::RecordCurrent()
{
  previous = MatrixOS::SYS::Millis();
}

// MicroTimer::MicroTimer()
// {
//   MicroTimer::RecordCurrent();
// }

// bool MicroTimer::Tick(u32 ms)
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

// bool MicroTimer::IsLonger(u32 ms)
// {
//   return (previous + ms) <= micros();
// }

// u32 MicroTimer::SinceLastTick()
// {
//   return micros() - previous;
// }

// void MicroTimer::RecordCurrent()
// {
//   previous = micros();
// }
