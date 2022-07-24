#include "FactoryMenu.h"

void FactoryMenu::Setup()
{
    MatrixOS::SYS::Rotate(EDirection::UP, true);
    
    UI factoryMenu("Factory Menu", Color(0xFFFFFF));

    factoryMenu.AddUIComponent(new UIButton("LED Test", 
                                    Color(0xFFFFFF),
                                    [&]() -> void {LEDTester();}), 
                                    Point(0, 0));
    factoryMenu.AddUIComponent(new UIButton("Keypad Test", 
                                    Color(0xFFFFFF),
                                    [&]() -> void {KeyPadTester();}), 
                                    Point(1, 0));
    factoryMenu.AddUIComponent(new UIButton("Touch Bar Test", 
                                    Color(0xFFFFFF),
                                    [&]() -> void {TouchBarTester();}), 
                                    Point(2, 0));

    factoryMenu.AddUIComponent(new UIButtonWithColorFunc("Burn EFuse", 
                                        [&]() -> Color{return esp_efuse_block_is_empty(EFUSE_BLK3) ? Color(0xFF0000) : Color(0x00FF00);}, 
                                        [&]() -> void {EFuseBurner();}), 
                                        Point(0, 7));

    factoryMenu.AddUIComponent(new UIButtonWithColorFunc("USB Connection", 
                                    [&]() -> Color{return MatrixOS::USB::Connected() ? Color(0x00FF00): Color(0xFF0000);}, 
                                    [&]() -> void {}), 
                                    Point(7, 7));

    factoryMenu.Start();
    Exit();
}