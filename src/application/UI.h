#pragma once

#include "MatrixOS.h"
#include "application/Application.h"
#include <string>
#include <functional>
#include <list>
#include <map>
#include <stdarg.h>

struct UIElement
{
    std::string name;
    Color color;
    std::function<void()> callback;

    UIElement(std::string name, Color color, std::function<void()> callback)
    {
        this->name = name;
        this->color = color;
        this->callback = callback;
    }

    // UIElement(std::string name, Color color, std::function<void()> callback, uint8_t count, ...)
    // {
    //     this->name = name;
    //     this->color = color;
    //     this->callback = callback;

    //     va_list valst;
    //     va_start(valst, count);
    //     for(uint8_t i = 0; i < count; i++)
    //     {
    //         xys.push_back(va_arg(valst, Point));
    //     }
    // }

    void Callback()
    {
        callback();
    }
};

class UI
{
    public:
        char name;
        Color nameColor;
        int8_t status = 0;

        void Start();

        virtual void Setup() {};
        virtual void Loop() {};
        virtual void End() {};

        void Exit();

        void GetKey();
        virtual void KeyEvent(uint16_t KeyID, KeyInfo keyInfo) {};

        void GetMidi();
        virtual void MidiEvent(MidiPacket midiPacket) {};

        std::list<UIElement> uiElements;
        std::map<Point, UIElement*> uiElementsMap;

        void AddUIElement(UIElement uiElement, Point xy);
        void AddUIElement(UIElement uiElement, uint16_t count, ...);

        void ClearUIElements();
        private:
        void Render();
        void UIKeyEvent(uint16_t KeyID, KeyInfo keyInfo);
};