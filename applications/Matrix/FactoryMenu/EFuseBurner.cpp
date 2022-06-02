#include "FactoryMenu.h"

void BurnEFuse(); //This is in device folder, a custom BurnEFuse will be provided

void FactoryMenu::EFuseBurner()
{
    #ifdef EFUSE_BURNER
    if(esp_efuse_block_is_empty(EFUSE_BLK3))
    {
        UI efuseConfirm("eFuse Burn Confirmation", Color(0xFFFFFF));

        efuseConfirm.AddUIElement(UIElement("Confirm", 
                                        Color(0x00FF00), 
                                        [&]() -> void {BurnEFuse(); Exit();}), 
                                        4, Point(1, 5), Point(2, 5), Point(1, 6), Point(2, 6));

        efuseConfirm.AddUIElement(UIElement("Cancel", 
                                    Color(0xFF0000), 
                                    [&]() -> void {Exit();}), 
                                    4, Point(5, 5), Point(6, 5), Point(5, 6), Point(6, 6));

        efuseConfirm.Start();
    }
    else
    {
        MatrixOS::LED::Fill(0);
        MatrixOS::LED::SetColor(Point(2,2), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(3,2), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(4,2), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(5,2), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(2,3), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(5,3), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(2,4), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(5,4), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(2,5), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(3,5), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(4,5), Color(0x00FF00));
        MatrixOS::LED::SetColor(Point(5,5), Color(0x00FF00));
        MatrixOS::LED::Update();
        MatrixOS::SYS::DelayMs(2000);
        MatrixOS::LED::Fill(0);
    }
    #else //Not in factory mode or not ESP32
        MatrixOS::LED::Fill(0);
        MatrixOS::LED::SetColor(Point(2,2), Color(0xFF00FF));
        MatrixOS::LED::SetColor(Point(3,2), Color(0xFF00FF));
        MatrixOS::LED::SetColor(Point(4,2), Color(0xFF00FF));
        MatrixOS::LED::SetColor(Point(5,2), Color(0xFF00FF));
        MatrixOS::LED::SetColor(Point(2,3), Color(0xFF00FF));
        MatrixOS::LED::SetColor(Point(5,3), Color(0xFF00FF));
        MatrixOS::LED::SetColor(Point(2,4), Color(0xFF00FF));
        MatrixOS::LED::SetColor(Point(5,4), Color(0xFF00FF));
        MatrixOS::LED::SetColor(Point(2,5), Color(0xFF00FF));
        MatrixOS::LED::SetColor(Point(3,5), Color(0xFF00FF));
        MatrixOS::LED::SetColor(Point(4,5), Color(0xFF00FF));
        MatrixOS::LED::SetColor(Point(5,5), Color(0xFF00FF));
        MatrixOS::LED::Update();
        MatrixOS::SYS::DelayMs(2000);
        MatrixOS::LED::Fill(0);
    #endif
}