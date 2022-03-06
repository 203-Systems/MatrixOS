#include "applications/Application.h"

void BootAnimation::Start()
{
    Setup();
    while(status != -1)
    {
        LoopTask();
        Loop();
    }
    End();
}

void BootAnimation::Exit()
{
    status = -1;
}

void BootAnimation::LoopTask()
{
    GetKey();
    GetMidi();
}

void BootAnimation::GetKey()
{
    while(MatrixOS::KEYPAD::Available())
    {   
        uint16_t keyID = MatrixOS::KEYPAD::Get();
        KeyEvent(keyID, MatrixOS::KEYPAD::GetKey(keyID));
    }
}

void BootAnimation::GetMidi()
{   
    while(MatrixOS::MIDI::Available())
    {
        MidiEvent(MatrixOS::MIDI::Get());
    }
}