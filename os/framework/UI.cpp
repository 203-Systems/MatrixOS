#include "UI.h"

UI::UI(string name, Color color)
{
    this->name = name;
    this->nameColor = color;
}

//TODO, make new led layer
void UI::Start()
{
    // MatrixOS::LED::PauseAutoUpdate();
    Setup();
    while(status != -1)
    {   
        Loop();
        RenderUI();
        GetKey();
        GetMidi();
    }
    End();
    MatrixOS::LED::Fill(0);
    // MatrixOS::LED::Update();
    // MatrixOS::LED::StartAutoUpdate();
}

void UI::Exit()
{
    status = -1;
}

void UI::RenderUI()
{   
    if(uiTimer.Tick(uiFps))
    {
        MatrixOS::LED::Fill(0);
        for (auto const& uiElementMap : uiElementsMap)
        {   
            Point xy = uiElementMap.first;
            UIElement* uiElement = uiElementMap.second;
            MatrixOS::LED::SetColor(xy, uiElement->color);
        }
        Render();
        MatrixOS::LED::Update();
    }
}

void UI::GetKey()
{
    while(MatrixOS::KEYPAD::Available())
    {   
        uint16_t keyID = MatrixOS::KEYPAD::Get();
        bool action = KeyEvent(keyID, MatrixOS::KEYPAD::GetKey(keyID));
        if(!action)
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
    if(xy && uiElementsMap.count(xy)) //Key Found
    {   
        MatrixOS::Logging::LogDebug("UI", "Key Event %d %d", xy.x, xy.y);
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
    else
    {
        MatrixOS::Logging::LogDebug("UI", "Key Event %d", keyID);
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