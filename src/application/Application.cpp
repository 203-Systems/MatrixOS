#include "Application.h"

void Application::Start()
{
    // ESP_LOGI("Application", "Start Application");
    Setup();
    // ESP_LOGI("Application", "Setup Complete");
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = 100;
    while(status != -1)
    {
        // ESP_LOGI("Application", "Loop");
        LoopTask();
        Loop();
        // vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
    End();
}

void Application::Exit()
{
    status = -1;
}

void Application::LoopTask()
{
    // MatrixOS::SYS::SystemTask();
    // taskYIELD();
    GetKey();
    GetMidi();
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