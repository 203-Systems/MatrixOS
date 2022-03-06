#pragma once

#include "MatrixOS.h"
#include "application/Applications.h"

class BootAnimation : public Application
{
    public: 
        string name;
        string author;
        uint32_t version;

        int8_t status = 0;

        void Start();

        virtual void Setup() {};

        virtual void Idle() {};
        virtual void Boot() {};
        
        virtual void End() {};

        void Exit();
};