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

void BootAnimation::KeyEvent(uint16_t KeyID, KeyInfo keyInfo)
{
    if(KeyID == FUNCTION_KEY && keyInfo.state == PRESSED)
    {
        Exit();
    }
}