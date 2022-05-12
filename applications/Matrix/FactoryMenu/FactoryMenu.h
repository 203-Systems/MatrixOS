#pragma once

#include "MatrixOS.h"
#include "Framework/UI.h"
#include "applications/Application.h"

#define APPLICATION_NAME "Matrix Factory Menu"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0xFFFFFF)
#define APPLICATION_VERSION 1
#define APPLICATION_CLASS FactoryMenu

class FactoryMenu : public Application
{   
    void Setup() override;
    
    // void LEDTest();
    // void TouchBarTest();
    // void KeyPadTest();

    // void BurnEFuse();
    void LEDTester();
    void KeyPadTester();
    void TouchBarTester();

    void EFuseBurner();
};

#include "applications/RegisterApplication.h"