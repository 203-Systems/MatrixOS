#pragma once

#include "MatrixOS.h"

#define REGISTERED_APPS Application_Info("Performance Mode", "203 Electronics", Color(0xFF0000), true), Application_Info("REDACTED", "203 Electronics", Color(0xFFFFFF), true) 
// #define REGISTER_APPLICATION(APP_NAME, AUTHOR, APP_CLASS_NAME, COLOR, VERSION, VISABLE) 
// #define APPLICTION_REGISTION APPLICTION_REGISTION##Application_Info

struct Application_Info
{
    string name;
    string author;
    Color color;
    uint32_t version;
    bool visible;

    Application_Info(string name, string author, Color color, uint32_t version, bool visible = true)
    {
        this->name = name;
        this->author = author;
        this->color = color;
        this->version = version;
        this->visible = visible;
    }
};



class Application
{
    public: 
        string name;
        string author;
        uint32_t version;

        void* args;

        void Start(void* args = NULL);

        virtual void Setup() {};
        virtual void Loop() {};
        virtual void End() {};

        void Exit();

        void LoopTask();

        void GetKey();
        virtual void KeyEvent(uint16_t KeyID, KeyInfo keyInfo) {};

        void GetMidi();
        virtual void MidiEvent(MidiPacket midiPacket) {};
};