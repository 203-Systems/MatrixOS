#include "FactoryMenu.h"

void FactoryMenu::Setup()
{
    UI factoryMenu("Factory Menu", Color(0xFFFFFF));

    factoryMenu.AddUIElement(new UIButton("LED Test", 
                                    Color(0xFFFFFF),
                                    [&]() -> void {LEDTester();}), 
                                    Point(0, 0));
    factoryMenu.AddUIElement(new UIButton("Keypad Test", 
                                    Color(0xFFFFFF),
                                    [&]() -> void {KeyPadTester();}), 
                                    Point(1, 0));
    factoryMenu.AddUIElement(new UIButton("Touch Bar Test", 
                                    Color(0xFFFFFF),
                                    [&]() -> void {TouchBarTester();}), 
                                    Point(2, 0));

    #ifdef EFUSE_BURNER
    factoryMenu.AddUIElement(new UIButton("Burn EFuse", 
                                        esp_efuse_block_is_empty(EFUSE_BLK3) ? Color(0xFF0000) : Color(0x00FF00), 
                                        [&]() -> void {EFuseBurner();}), 
                                        Point(0, 7));
    #endif

    factoryMenu.Start();
    Exit();
}