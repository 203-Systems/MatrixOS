#include "FactoryMenu.h"
#include "esp_efuse.h"

void FactoryMenu::Setup()
{
    UI factoryMenu("Factory Menu", Color(0xFFFFFF));

    factoryMenu.AddUIElement(UIElement("LED Test", 
                                    Color(0xFFFFFF),
                                    [&]() -> void {LEDTester();}), 
                                    Point(0, 0));
    factoryMenu.AddUIElement(UIElement("Keypad Test", 
                                    Color(0xFFFFFF),
                                    [&]() -> void {KeyPadTester();}), 
                                    Point(1, 0));
    factoryMenu.AddUIElement(UIElement("Touch Bar Test", 
                                    Color(0xFFFFFF),
                                    [&]() -> void {TouchBarTester();}), 
                                    Point(2, 0));

    factoryMenu.AddUIElement(UIElement("Burn EFuse", 
                                        esp_efuse_block_is_empty(EFUSE_BLK3) ? Color(0xFF0000) : Color(0x00FF00), 
                                        [&]() -> void {EFuseBurner();}), 
                                        Point(0, 7));

    factoryMenu.Start();
    Exit();
}