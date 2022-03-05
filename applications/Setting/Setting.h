#pragma once
// #ifdef GRID_8x8

#include "MatrixOS.h"
#include "framework/UI.h"

class Setting: public UI
{
    public:
    char name[11] = "Setting";
    Color nameColor = Color(0x00FFFF);

    void Setup() override;
    // void End() override;

    static void RotateClockwise(EDirection rotation);
    static void NextBrightness();

    // bool KeyEvent(uint16_t KeyID, KeyInfo keyInfo) override;
};