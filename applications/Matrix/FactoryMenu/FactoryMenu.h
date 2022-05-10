#pragma once

#include "MatrixOS.h"
#include "Framework/UI.h"
#include "applications/Application.h"

#include "FactoryTest/FactoryTest.h"
#include "EFuseBurner/EFuseBurner.h"

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
};

#include "applications/RegisterApplication.h"