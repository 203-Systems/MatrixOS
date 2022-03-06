#include "BootAnimation.h"

void BootAnimation::Start()
{
    Setup();
    //Idle animation
    bool hold = false;
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
    MatrixOS::KEYPAD::Clear();
    // while(MatrixOS::KEYPAD::GetKey(FUNCTION_KEY)){} //Prevert FN Press leak though, will remove later when key buffer can be cleared
}
