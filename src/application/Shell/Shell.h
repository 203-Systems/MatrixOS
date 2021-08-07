#pragma once


#include "application/Application.h"

class Shell : public Application
{
    char name[64] = "Matrix OS Shell";
    char author[64] = "203 Electronics";
    uint32_t version = 10000;
    
    void main();
};