#pragma once
// #ifdef GRID_8x8

#include "MatrixOS.h"
#include "UI/UI.h"

class Setting: public UI
{
    public:
    // string name = "Setting";
    // Color nameColor = Color(0x00FFFF);

    const Point origin = Point((Device::x_size - 1)/2, (Device::y_size - 1)/2);

    void Setup() override;
    // void End() override;

    static void RotateClockwise(EDirection rotation);
    static void NextBrightness();

    bool KeyEvent(uint16_t KeyID, KeyInfo* keyInfo) override;

    private:
    uint8_t konami = 0;
};