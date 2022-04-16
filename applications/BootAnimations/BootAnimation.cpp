#include "BootAnimation.h"

void BootAnimation::Start()
{
    Setup();
    //Idle animation
    bool hold = false;
    for(uint8_t i = 0; i < 100; i++) //Add small delay for USB to be connected
    {
        MatrixOS::SYS::DelayMs(5);
        if(MatrixOS::USB::Connected())
            break;
    }
    while(status != -1 && (!MatrixOS::USB::Connected() || hold))
    {
        LoopTask();
        hold = Idle(MatrixOS::USB::Connected());
    }

    while(status != -1)
    {
        LoopTask();
        Boot();
    }

    End();
}

void BootAnimation::Exit()
{
    status = -1;
}

void BootAnimation::LoopTask()
{
    if(MatrixOS::KEYPAD::GetKey(FUNCTION_KEY))
    {
        Exit();
    }
    MatrixOS::KEYPAD::Clear(); //Prevert FN Press leak though.
}
