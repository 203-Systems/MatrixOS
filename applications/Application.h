#pragma once

#include "MatrixOS.h"

struct Application_Info
{
    string name;
    string author;
    Color color;
    uint32_t version;
    bool visibility;

    Application_Info(string name, string author, Color color, uint32_t version, bool visible = true)
    {
        this->name = name;
        this->author = author;
        this->color = color;
        this->version = version;
        this->visibility = visibility;
    }
    Application_Info(){}
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