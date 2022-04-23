#include "UI.h"

UI::UI(string name, Color color)
{
    this->name = name;
    this->nameColor = color;
}

//TODO, make new led layer
void UI::Start()
{
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
    if(keyID == FUNCTION_KEY)
    {
        if(keyInfo.state == RELEASED && !fn_released)
        {
            fn_released = true;
            return;
        }
        else if(keyInfo.state == ((func_hold_callback == nullptr) ? PRESSED : RELEASED))
        {
            Exit();
            return;
        }

        if(keyInfo.state == HOLD)
        {
            MatrixOS::Logging::LogDebug("UI", "Function Hold");
            func_hold_callback();
            return;
        }
    }
    Point xy = MatrixOS::KEYPAD::ID2XY(keyID);
    if(xy && uiElementsMap.count(xy)) //Key Found
    {   
        MatrixOS::Logging::LogDebug("UI", "Key Event %d %d", xy.x, xy.y);
        if(keyInfo.state == RELEASED && keyInfo.hold == false) 
        {
            uiElementsMap[xy]->Callback();
            return;
        }
        else if(keyInfo.state == HOLD)
        {
            uiElementsMap[xy]->HoldCallback();
            return;
        }
    }
    else
    {
        // MatrixOS::Logging::LogDebug("UI", "Key Event %d", keyID);
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

void UI::AddFuncKeyHold(std::function<void()> callback)
{
    func_hold_callback = callback;
}

void UI::ClearUIElements()
{
    uiElements.clear();
}