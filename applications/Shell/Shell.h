#pragma once

#include "MatrixOS.h"
#include "Framework/UI.h"
#include "applications/Application.h"

#define APPLICATION_NAME "Shell"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0xFFFFFF)
#define APPLICATION_VERSION 0
#define APPLICATION_CLASS Shell
#define APPLICATION_VISIBLITY false

class Shell : public Application
{
    string name = "Matrix OS Shell";
    string author = "203 Electronics";
    uint32_t version = 10000;

    uint8_t current_page = 0;

    void Loop() override;

    void AddCommonBarInUI(UI* ui);
    void ApplicationLauncher();

    // void GetKey();
    // virtual bool KeyEvent(uint16_t KeyID, KeyInfo keyInfo) {return false;}; //Return true to skip UIKeyEvent

    // std::list<UIElement> uiElements;
    // std::map<Point, UIElement*> uiElementsMap;

    // void AddUIElement(UIElement uiElement, Point xy);
    // void ClearUIElements();

    // private:
    // void RenderUI();
    // void UIKeyEvent(uint16_t KeyID, KeyInfo keyInfo);
};

#include "applications/RegisterApplication.h"