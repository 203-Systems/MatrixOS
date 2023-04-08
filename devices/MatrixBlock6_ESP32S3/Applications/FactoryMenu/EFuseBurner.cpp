#include "FactoryMenu.h"

void BurnEFuse();  // This is in device folder, a custom BurnEFuse will be provided

void FactoryMenu::EFuseBurner() {
#ifdef EFUSE_BURNER
  if (true)
  {
    UI efuseConfirm("eFuse Burn Confirmation", Color(0xFFFFFF));

    UIButtonLarge confirmBtn("Confirm", Color(0x00FF00), Dimension(2, 2), [&]() -> void {
      BurnEFuse();
      efuseConfirm.Exit();
    });
    efuseConfirm.AddUIComponent(confirmBtn, Point(1, 5));

    UIButtonLarge cancelBtn("Cancel", Color(0xFF0000), Dimension(2, 2), [&]() -> void { efuseConfirm.Exit(); });
    efuseConfirm.AddUIComponent(cancelBtn, Point(5, 5));

    efuseConfirm.Start();
  }
  else
  {
    MatrixOS::LED::Fill(0);
    MatrixOS::LED::SetColor(Point(2, 2), Color(0x00FF00));
    MatrixOS::LED::SetColor(Point(3, 2), Color(0x00FF00));
    MatrixOS::LED::SetColor(Point(4, 2), Color(0x00FF00));
    MatrixOS::LED::SetColor(Point(5, 2), Color(0x00FF00));
    MatrixOS::LED::SetColor(Point(2, 3), Color(0x00FF00));
    MatrixOS::LED::SetColor(Point(5, 3), Color(0x00FF00));
    MatrixOS::LED::SetColor(Point(2, 4), Color(0x00FF00));
    MatrixOS::LED::SetColor(Point(5, 4), Color(0x00FF00));
    MatrixOS::LED::SetColor(Point(2, 5), Color(0x00FF00));
    MatrixOS::LED::SetColor(Point(3, 5), Color(0x00FF00));
    MatrixOS::LED::SetColor(Point(4, 5), Color(0x00FF00));
    MatrixOS::LED::SetColor(Point(5, 5), Color(0x00FF00));
    MatrixOS::LED::Update();
    MatrixOS::SYS::DelayMs(2000);
    MatrixOS::LED::Fill(0);
  }
#else  // Not in factory mode or not ESP32
  MatrixOS::LED::Fill(0);
  MatrixOS::LED::SetColor(Point(2, 2), Color(0xFF00FF));
  MatrixOS::LED::SetColor(Point(3, 2), Color(0xFF00FF));
  MatrixOS::LED::SetColor(Point(4, 2), Color(0xFF00FF));
  MatrixOS::LED::SetColor(Point(5, 2), Color(0xFF00FF));
  MatrixOS::LED::SetColor(Point(2, 3), Color(0xFF00FF));
  MatrixOS::LED::SetColor(Point(5, 3), Color(0xFF00FF));
  MatrixOS::LED::SetColor(Point(2, 4), Color(0xFF00FF));
  MatrixOS::LED::SetColor(Point(5, 4), Color(0xFF00FF));
  MatrixOS::LED::SetColor(Point(2, 5), Color(0xFF00FF));
  MatrixOS::LED::SetColor(Point(3, 5), Color(0xFF00FF));
  MatrixOS::LED::SetColor(Point(4, 5), Color(0xFF00FF));
  MatrixOS::LED::SetColor(Point(5, 5), Color(0xFF00FF));
  MatrixOS::LED::Update();
  MatrixOS::SYS::DelayMs(2000);
  MatrixOS::LED::Fill(0);
#endif
}