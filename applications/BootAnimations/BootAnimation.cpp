#include "BootAnimation.h"

void BootAnimation::Loop()
{
    bool idle_hold = false;
    while(!MatrixOS::USB::Connected() || idle_hold)
    {
        LoopTask();
        idle_hold = Idle(MatrixOS::USB::Connected());
    }
    while(true)
    {
        LoopTask();
        Boot();
    }
}