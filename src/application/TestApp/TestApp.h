#ifndef __TestApp_H
#define __TestApp_H

#include "application/Application.h"
// #include "tusb.h"

class TestApp// : public Application
{
    // using Application::Application;
    public:
    // inline TestApp()
    // {
    //     main();
    // }

    char name[64] = "Matrix OS TestApp";
    char author[64] = "203 Electronics";
    uint32_t version = 10000;
    
    void main();

    void midi_task(void);
};
#endif