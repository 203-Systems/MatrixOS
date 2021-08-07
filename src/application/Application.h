#pragma once

#include "MatrixOS.h"

class Application
{
    public: 
        char name[64];
        char author[64];
        uint32_t version;

        int8_t status = 0;
        
        inline Application()
        {
            main();
        }

        virtual void main() {};

        // virtual void KeyEvent(MatrixOS::KEYPAD::KeyInfo keyInfo) {};
        // virtual void MidiEvent() {};
};