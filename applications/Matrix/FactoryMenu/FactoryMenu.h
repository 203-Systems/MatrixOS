#pragma once

#include "MatrixOS.h"
#include "Framework/UI.h"
#include "applications/Application.h"

#if defined(FACTORY_CONFIG) && defined(ESP32)
#include "esp_efuse.h"
#define EFUSE_BURNER
#endif


#define APPLICATION_NAME "Matrix Factory Menu"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0xFFFFFF)
#define APPLICATION_VERSION 1
#define APPLICATION_CLASS FactoryMenu
#define APPLICATION_VISIBLITY false

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