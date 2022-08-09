#include "UI.h"

UI::UI(string name, Color color, bool newLedLayer)
{
    this->name = name;
    this->nameColor = color;
    this->newLedLayer = newLedLayer;
}

// TODO, make new led layer
void UI::Start()
{
    if (newLedLayer)
        MatrixOS::LED::CreateLayer();
    MatrixOS::KEYPAD::Clear();
    Setup();
    while (status != -1)
    {
        LoopTask();
        Loop();
        RenderUI();
    }
    End();
    UIEnd();
}

void UI::Exit()
{
    status = -1;
}

void UI::LoopTask()
{
    GetKey();
    // GetMidi();
}

void UI::RenderUI()
{
    if (uiTimer.Tick(uiFps))
    {
        MatrixOS::LED::Fill(0);
        for (auto const &uiComponentPair : uiComponentMap)
        {
            Point xy = uiComponentPair.first;
        UIComponent *uiComponent = uiComponentPair.second;
        uiComponent->Render(xy);
        }
        Render();
        MatrixOS::LED::Update();
    }
}

void UI::GetKey()
{
    while (MatrixOS::KEYPAD::Available())
    {
        uint16_t keyID = MatrixOS::KEYPAD::Get();
        KeyInfo* keyInfo = MatrixOS::KEYPAD::GetKey(keyID);
        // MatrixOS::Logging::LogDebug("UI", "Key Event %d %d", keyID, keyInfo.state);
        bool action = KeyEvent(keyID, keyInfo);
        if (!action)
            UIKeyEvent(keyID, keyInfo);
        else
            MatrixOS::Logging::LogDebug("UI", "KeyEvent Skip: %d", keyID);
    }
}

void UI::UIKeyEvent(uint16_t keyID, KeyInfo* keyInfo)
{
    MatrixOS::Logging::LogDebug("UI Key Event", "%d - %d", keyID, keyInfo->state);
    if (keyID == FUNCTION_KEY)
    {
        if (!disableExit && keyInfo->state == ((func_hold_callback == nullptr) ? PRESSED : RELEASED))
        {
            MatrixOS::Logging::LogDebug("UI", "Function Key Exit");
            Exit();
            return;
        }

        if (keyInfo->state == HOLD && func_hold_callback)
        {
            MatrixOS::Logging::LogDebug("UI", "Function Hold");
            (*func_hold_callback)();
            MatrixOS::KEYPAD::Clear();
            return;
        }
    }
    Point xy = MatrixOS::KEYPAD::ID2XY(keyID);
    if (xy)
    {
        MatrixOS::Logging::LogDebug("UI", "UI Key Event X:%d Y:%d", xy.x, xy.y);
        bool hasAction = false;
        for (auto const &uiComponentPair : uiComponentMap)
        {
            Point relative_xy = xy - uiComponentPair.first;
            UIComponent *uiComponent = uiComponentPair.second;
            if (uiComponent->GetSize().Inside(relative_xy)) // Key Found
            {
                hasAction |= uiComponent->KeyEvent(relative_xy, keyInfo);
            }
        }
        if(hasAction == false && keyInfo->state == HOLD && Dimension(Device::x_size, Device::y_size).Inside(xy))
        {
            MatrixOS::UIInterface::TextScroll(this->name, this->nameColor);
        }
    }

}

void UI::AddUIComponent(UIComponent *uiComponent, Point xy)
{
    // ESP_LOGI("Add UI Component", "%d %d %s", xy.x, xy.y, uiComponent->GetName().c_str());
    // uiComponents.push_back(uiComponent);
    uiComponentMap[xy] = uiComponent;
}

void UI::AddUIComponent(UIComponent *uiComponent, uint16_t count, ...)
{
    // uiComponents.push_back(uiComponent);
    va_list valst;
    va_start(valst, count);
    for (uint8_t i = 0; i < count; i++)
    {
        uiComponentMap[(Point)va_arg(valst, Point)] = uiComponent;
    }
}

void UI::AllowExit(bool allow)
{
    disableExit = !allow;
}

void UI::SetSetupFunc(std::function<void()> setup_func)
{
    UI::setup_func = &setup_func;
}

void UI::SetLoopFunc(std::function<void()> loop_func)
{
    UI::loop_func = &loop_func;
}

void UI::SetEndFunc(std::function<void()> end_func)
{
    UI::end_func = &end_func;
}

void UI::AddFuncKeyHold(std::function<void()> callback)
{
    UI::func_hold_callback = &callback;
}

void UI::ClearUIComponents()
{
    uiComponentMap.clear();
}

void UI::UIEnd()
{
    if (newLedLayer)
    {
        MatrixOS::LED::DestoryLayer();
    }
    else
    {
        MatrixOS::LED::Fill(0);
    }

    MatrixOS::LED::Update();

    // Free up heap
    vector<UIComponent*> deletedComponents; //Pervent pointer is deleted twice
    deletedComponents.reserve(uiComponentMap.size());
    for (auto const &uiComponentPair : uiComponentMap)
    {
        if(std::find(deletedComponents.begin(), deletedComponents.end(), uiComponentPair.second) == deletedComponents.end())
        {
            deletedComponents.push_back(uiComponentPair.second);
            delete uiComponentPair.second;
        }
    }
}