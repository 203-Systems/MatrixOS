#include "ActionMenu8x8.h"

void ActionMenu::Setup()
{
    AddUIElement(UIElement("Rotate right", Color(0x00FF00), []() -> void {RotateClockwise(RIGHT);}), Point(5, 3));
}

void ActionMenu::RotateClockwise(EDirection rotation)
{
    
}