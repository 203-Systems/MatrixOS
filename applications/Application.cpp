#include "MatrixOS.h"
#include "Application.h"

void Application::Start(void* args)
{
    this->args = args;
    Setup();
    while(true)
    {
        LoopTask();
        Loop();
    }
}

void Application::Exit()
{
    End();
    MatrixOS::SYS::ExitAPP();
}

void Application::LoopTask()
{
    GetKey();
    GetMidi();
}

void Application::GetKey()
{
    struct KeyEvent keyEvent;
    while (MatrixOS::KEYPAD::Get(&keyEvent))
    {   
        KeyEvent(keyEvent.id, &keyEvent.info);
    }
}

void Application::GetMidi()
{   
    struct MidiPacket midiPacket;
    while (MatrixOS::MIDI::Get(&midiPacket))
    {   
        MidiEvent(midiPacket);
    }
}