#pragma once

#include "MatrixOS.h"

class BootAnimation
{
    public: 
        string name;
        string author;
        uint32_t version;

        int8_t status = 0;

        void Start();

        virtual void Setup() {};

        virtual bool Idle(bool ready) {return false;}; //return true will keep render idle animation even when device is ready
        virtual void Boot() {};
        
        virtual void End() {};

        void LoopTask();

        void Exit();
};