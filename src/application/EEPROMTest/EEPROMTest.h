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

    std::string WaitForString();
    uint32_t WaitForHex();

    void StripString(std::string& str);

    void WriteToFlash(uint32_t flash_address, uint16_t length, uint16_t* data);
    void bytes2hex (unsigned char *src, char *out, uint8_t len);
};