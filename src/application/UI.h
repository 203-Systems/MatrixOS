#pragma once

#include "MatrixOS.h"
#include "application/Application.h"

class UI : public Application
{
    public:
        char name;
        int8_t status = 0;
}