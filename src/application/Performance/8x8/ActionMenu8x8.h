#pragma once
// #ifdef GRID_8x8

#include "MatrixOS.h"
#include "framework/UI.h"

class ActionMenu : public UI
{
    public:
    string name = "ActionMenu";
    Color nameColor = Color(0x00FFAA);

    void Setup() override;
    // void End() override;

    void RotateClockwise(EDirection rotation);
    void NextBrightness();

    // void KeyEvent(uint16_t KeyID, KeyInfo keyInfo) override;
};