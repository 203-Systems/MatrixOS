#pragma once

#include "MatrixOS.h"
#include "framework/Timer.h"
#include "application/Application.h"

class EEPROMTest : public Application
{   
    // using Application::Application;

    public:
    char name[18] = "Matrix OS TestApp";
    char author[16] = "203 Electronics";
    uint32_t version = 0;
    
    void Setup() override;
    void Loop() override;

    void PrintMenu();
};