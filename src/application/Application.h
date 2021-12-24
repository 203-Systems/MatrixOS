#pragma once

#include "MatrixOS.h"

class Application
{
    public: 
        string name;
        string author;
        uint32_t version;

        int8_t status = 0;

        void Start();

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