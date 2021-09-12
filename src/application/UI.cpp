#include "UI.h"

void UI::Start()
{
    Setup();
    while(status != -1)
    {   
        Render();
        Loop();
        MatrixOS::SYS::SystemTask();
        GetKey();
        GetMidi();
    }
    End();
    MatrixOS::LED::Fill(0);
}

void UI::Exit()
{
    status = -1;
}

void UI::Render()
{
    MatrixOS::LED::Fill(0);
    for (auto const& uiElementMap : uiElementsMap)
    {   
        Point xy = uiElementMap.first;
        UIElement* uiElement = uiElementMap.second;
        MatrixOS::LED::SetColor(xy, uiElement->color);
    }
}

void UI::GetKey()
{
    while(MatrixOS::KEYPAD::Available())
    {   
        uint16_t keyID = MatrixOS::KEYPAD::Get();
        UIKeyEvent(keyID, MatrixOS::KEYPAD::GetKey(keyID));
    }
}

void UI::UIKeyEvent(uint16_t keyID, KeyInfo keyInfo)
{
    if(keyID == 0 && keyInfo.state == PRESSED)
    {
        Exit();
        return;
    }
}

void UI::GetMidi()
{
    while(MatrixOS::MIDI::Available())
    {
        MidiEvent(MatrixOS::MIDI::Get());
    }
}

void UI::AddUIElement(UIElement uiElement, Point xy)
{
    uiElements.push_back(uiElement);
    uiElementsMap[xy] = &(uiElements.back());
}

void UI::ClearUIElements()
{
    uiElements.clear();
}