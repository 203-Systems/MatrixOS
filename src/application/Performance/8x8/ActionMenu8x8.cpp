#include "ActionMenu8x8.h"

void ActionMenu::Setup()
{
    AddUIElement(UIElement("Rotate right", Color(0x00FF00), []() -> void {RotateClockwise(RIGHT);}), 2, Point(5, 3), Point(5, 4));
}

void ActionMenu::RotateClockwise(EDirection rotation)
{
    
}