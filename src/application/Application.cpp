#include "Application.h"

void Application::Start()
{
    Setup();
    while(status != -1)
    {
        MatrixOS::SYS::SystemTask();
        GetKey();
        GetMidi();
        Loop();
    }
    End();
}

void Application::Exit()
{
    status = -1;
}

void Application::GetKey()
{
    while(MatrixOS::KEYPAD::Available())
    {   
        uint16_t keyID = MatrixOS::KEYPAD::Get();
        KeyEvent(keyID, MatrixOS::KEYPAD::GetKey(keyID));
    }
}

void Application::GetMidi()
{
    while(MatrixOS::MIDI::Available())
    {
        MidiEvent(MatrixOS::MIDI::Get());
    }
}