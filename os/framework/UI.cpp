#include "UI.h"

UI::UI(string name, Color color)
{
    this->name = name;
    this->nameColor = color;
}

//TODO, make new led layer
void UI::Start()
{
    MatrixOS::KEYPAD::Clear();
    Setup();
    while(status != -1)
    {   
        LoopTask();
        Loop();
        RenderUI();
    }
    End();
    MatrixOS::LED::Fill(0);
}

void UI::Exit()
{
    status = -1;
}

void UI::LoopTask()
{
    GetKey();
    GetMidi();
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
    // MatrixOS::Logging::LogDebug("UI Key Event", "%d - %d", keyID, keyInfo.state);
    if(keyID == FUNCTION_KEY)
    {
        if(!disableExit && keyInfo.state == ((func_hold_callback == nullptr) ? PRESSED : RELEASED))
        {
            MatrixOS::Logging::LogDebug("UI", "Function Key Exit");
            Exit();
            return;
        }

        if(keyInfo.state == HOLD)
        {
            MatrixOS::Logging::LogDebug("UI", "Function Hold");
            func_hold_callback();
            PostCallbackCleanUp();
            return;
        }
    }
    Point xy = MatrixOS::KEYPAD::ID2XY(keyID);
    if(xy && uiElementsMap.count(xy)) //Key Found
    {   
        MatrixOS::Logging::LogDebug("UI", "Key Event %d %d", xy.x, xy.y);
        if(keyInfo.state == RELEASED && keyInfo.hold == false) 
        {
            if(uiElementsMap[xy]->callback != nullptr)
            {
                MatrixOS::Logging::LogDebug("UI", "Key Event Callback");
                uiElementsMap[xy]->callback();
                PostCallbackCleanUp();
                return;
            }
        }
        else if(keyInfo.state == HOLD)
        {
            if(uiElementsMap[xy]->hold_callback != nullptr)
            {
                uiElementsMap[xy]->hold_callback();
                PostCallbackCleanUp();
                return;
            }
            else
            {
                MatrixOS::UIComponent::TextScroll(uiElementsMap[xy]->name, uiElementsMap[xy]->color);
            }
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

void UI::PostCallbackCleanUp()
{
    MatrixOS::KEYPAD::Clear();
}
