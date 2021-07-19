#ifndef __SHELL_H
#define __SHELL_H

#include "application/Application.h"

class Shell : public Application
{
    char name[64] = "Matrix OS Shell";
    char author[64] = "203 Electronics";
    uint32_t version = 10000;
    
    int main();
};
#endif