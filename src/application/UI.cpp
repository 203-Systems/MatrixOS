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
        KeyEvent(keyID, MatrixOS::KEYPAD::GetKey(keyID));
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
    Point xy = MatrixOS::KEYPAD::ID2XY(keyID);
    if(xy && uiElementsMap.count(xy)) //Not Found
    {   
        if(keyInfo.state == RELEASED)
        {
            uiElementsMap[xy]->Callback();
            return;
        }
        else if(keyInfo.state == HOLD)
        {
            if(uiElementsMap[xy]->hold_callback == NULL)
            {
                //TextScroll;
            }
            else
            {
              uiElementsMap[xy]->HoldCallback();
              return;
            }
        }
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

void UI::AddUIElement(UIElement uiElement, uint16_t count, ...)
{
    uiElements.push_back(uiElement);
    va_list valst;
    va_start(valst, count);
    for(uint8_t i = 0; i < count; i ++)
    {
        uiElementsMap[(Point)va_arg(valst, Point)] = &(uiElements.back());
    }
}

void UI::ClearUIElements()
{
    uiElements.clear();
}