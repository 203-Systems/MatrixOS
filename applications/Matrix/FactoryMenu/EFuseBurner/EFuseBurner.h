#pragma once

#include "MatrixOS.h"
#include "Framework/UI.h"
#include "applications/Application.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"

#define APPLICATION_NAME "Matrix EFuse Burner"
#define APPLICATION_AUTHOR "203 Electronics"
#define APPLICATION_COLOR Color(0xFFFFFF)
#define APPLICATION_VERSION 1
#define APPLICATION_CLASS EFuseBurner
#define APPLICATION_VISIBLITY false

class EFuseBurner : public Application
{   
    public:
    void Setup() override;
    // void Loop() override;

    void BurnEFuse();
};

#include "applications/RegisterApplication.h"