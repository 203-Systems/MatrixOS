#pragma once

#include "MatrixOS.h"
#include "framework/Timer.h"
#include "applications/Application.h"

class NVSTest : public Application
{   
    // using Application::Application;

    public:
    string name = "NVS Tester";
    string author = "203 Electronics";
    uint32_t version = 0;
    
    void Setup() override;
    void Loop() override;

    void PrintMenu();

    string WaitForString();
    void StripString(std::string& str);
};