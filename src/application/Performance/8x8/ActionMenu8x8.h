#pragma once
// #ifdef GRID_8x8

#include "MatrixOS.h"
#include "application/UI.h"

class ActionMenu : public UI
{
    public:
    string name = "ActionMenu";
    Color nameColor = Color(0x00FFAA);

    void Setup() override;
    // void End() override;

    static void RotateClockwise(EDirection rotation);
    static void NextBrightness();

    void KeyEvent(uint16_t KeyID, KeyInfo keyInfo) override;
};