#ifndef __TestApp_H
#define __TestApp_H

#include "application/Application.h"
#include "tusb.h"

class TestApp : public Application
{
    using Application::Application;

    char name[64] = "Matrix OS TestApp";
    char author[64] = "203 Electronics";
    uint32_t version = 10000;
    
    void main() override;

    void midi_task(void);
};
#endif