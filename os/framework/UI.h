#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"
#include <string>
#include <functional>
#include <list>
#include <map>
#include <stdarg.h>

struct UIElement
{
    string name;
    Color color;
    std::function<void()> callback;
    std::function<void()> hold_callback;

    UIElement(string name, Color color, std::function<void()> callback = nullptr, std::function<void()> hold_callback = nullptr)
    {
        this->name = name;
        this->color = color;
        this->callback = callback;
        this->hold_callback = hold_callback;
    }

    void Callback()
    {
        if(callback)
            callback();
    }

    void HoldCallback()
    {
        if(hold_callback)
        {
            hold_callback();
        }
        else
        {
            MatrixOS::UIComponent::TextScroll(name, color);
        }
    }
};

class UI
{
    public:
        string name;
        Color nameColor;
        int8_t status = 0;

        
        Timer uiTimer;
        uint16_t uiFps = 60;

        UI() {};
        UI(string name, Color color);

        void Start();

        virtual void Setup() {};
        virtual void Loop() {};
        virtual void Render() {};
        virtual void End() {};

        void Exit();

        void GetKey();
        virtual bool KeyEvent(uint16_t KeyID, KeyInfo keyInfo) {return false;}; //Return true to skip UIKeyEvent

        void GetMidi();
        virtual void MidiEvent(MidiPacket midiPacket) {};

        std::list<UIElement> uiElements;
        std::map<Point, UIElement*> uiElementsMap;

        void AddUIElement(UIElement uiElement, Point xy);
        // void AddUIElement(UIElement uiElement, uint32_t keyID);
        void AddUIElement(UIElement uiElement, uint16_t count, ...);

        std::function<void()> func_hold_callback = nullptr;
        bool fn_released = false;
        void AddFuncKeyHold(std::function<void()> callback);

        void ClearUIElements();
        private:
        void RenderUI();
        void UIKeyEvent(uint16_t KeyID, KeyInfo keyInfo);
};