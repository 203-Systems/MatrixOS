#pragma once
// #ifdef GRID_8x8

#include "MatrixOS.h"
#include "UI/UI.h"

class BrightnessControl: public UI
{
    public:
    // string name = "Setting";
    // Color nameColor = Color(0x00FFFF);

    const Point origin = Point((Device::x_size - 1)/2, (Device::y_size - 1)/2);

    void Setup() override;
};