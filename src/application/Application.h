#pragma once

#include "MatrixOS.h"

class Application
{
    public: 
        char name;
        char author;
        uint32_t version;

        int8_t status = 0;
        
        // Application();

        void Start();

        virtual void Setup() {};
        virtual void Loop() {};
        virtual void End() {};

        void Exit();

        void GetKey();
        virtual void KeyEvent(uint16_t KeyID, KeyInfo keyInfo) {};

        void GetMidi();
        virtual void MidiEvent(MidiPacket midiPacket) {};
};