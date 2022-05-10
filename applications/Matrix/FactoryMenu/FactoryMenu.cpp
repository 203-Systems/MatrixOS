#include "FactoryMenu.h"

void FactoryMenu::Setup()
{
    UI factoryMenu("Factory Menu", Color(0xFFFFFF));
    factoryMenu.AddUIElement(UIElement("Burn EFuse", 
                                        esp_efuse_block_is_empty(EFUSE_BLK3) ? Color(0xFF0000) : Color(0x00FF00), 
                                        [&]() -> void {EFuseBurner eFuseBurner; eFuseBurner.Start();}), 
                                        Point(0, 7));

    factoryMenu.Start();
}