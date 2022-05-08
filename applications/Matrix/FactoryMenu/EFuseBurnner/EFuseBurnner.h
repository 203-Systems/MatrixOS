#pragma once

#include "MatrixOS.h"
#include "Framework/UI.h"
#include "applications/Application.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"

#define APPLICATION_NAME "Matrix EFuse Burnner"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0xFFFFFF)
#define APPLICATION_VERSION 1
#define APPLICATION_CLASS EFuseBurnner

//I know this is very wasteful to memory but it is just a factory test so who gives a fuck
class EFuseBurnner : public Application
{   
    public:
    void Setup() override;
    // void Loop() override;

    void BurnEFuse();
};

#include "applications/RegisterApplication.h"