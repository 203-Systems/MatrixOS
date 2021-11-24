#pragma once


#include "application/Application.h"

class Shell : public Application
{
    string name = "Matrix OS Shell";
    string author = "203 Electronics";
    uint32_t version = 10000;
    
    void main();
};