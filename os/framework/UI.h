#pragma once

#include "MatrixOS.h"
#include "applications/Application.h"
#include <string>
#include <functional>
#include <list>
#include <map>
#include <stdarg.h>
#include "UIElement.h"

class UI
{
    public:
        string name;
        Color nameColor;
        int8_t status = 0;
        
        bool newLedLayer = false;
        bool disableExit = false;
        
        Timer uiTimer;
        uint16_t uiFps = 60;

        UI() {};
        UI(string name, Color color,bool newLedLayer = false);

        void Start();

        virtual void Setup() {};
        virtual void Loop() {};
        virtual void Render() {};
        virtual void End() {};

        void Exit();

        void LoopTask();

        void GetKey();
        virtual bool KeyEvent(uint16_t KeyID, KeyInfo keyInfo) {return false;}; //Return true to skip UIKeyEvent

        void GetMidi();
        virtual void MidiEvent(MidiPacket midiPacket) {};

        std::map<Point, UIElement*> uiElementMap;

        void AddUIElement(UIElement* uiElement, Point xy);
        // void AddUIElement(UIElement* uiElement, uint32_t keyID);
        void AddUIElement(UIElement* uiElement, uint16_t count, ...);

        std::function<void()> func_hold_callback = nullptr;
        void AddFuncKeyHold(std::function<void()> callback);

        void AllowExit(bool allow);

        void ClearUIElements();
        private:
        void RenderUI();
        void UIKeyEvent(uint16_t KeyID, KeyInfo keyInfo);
        void PostCallbackCleanUp();
};