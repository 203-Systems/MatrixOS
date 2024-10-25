#include "USBtest.h"
#include "ui/UI.h"

void USBtest::Setup() {
    UI usbUI("", Color(0xFFFFFF));
    
    // UIButtonWithColorFunc usbToggle("USB Connected", [&]() { return MatrixOS::USB::Connected() ? Color(0x00FF00) : Color(0xFF0000); }, [&]() { if (MatrixOS::USB::Connected()) { MatrixOS::USB::Disconnect(); } else { MatrixOS::USB::Connect(); } });
    // usbUI.AddUIComponent(usbToggle, Point(0, 0));

    // UIButtonWithColorFunc usbMounted("USB Mounted", [&]() { return tud_mounted() ? Color(0x00FF00) : Color(0xFF0000); }, [&]() { if (MatrixOS::USB::Connected()) { MatrixOS::USB::Disconnect(); } else { MatrixOS::USB::Connect(); } });
    // usbUI.AddUIComponent(usbMounted, Point(1, 0));

    // UIButtonWithColorFunc usbSuspended("USB Suspended", [&]() { return tud_suspended() ? Color(0x00FF00) : Color(0xFF0000); }, [&]() { if (MatrixOS::USB::Connected()) { MatrixOS::USB::Disconnect(); } else { MatrixOS::USB::Connect(); } });
    // usbUI.AddUIComponent(usbSuspended, Point(2, 0));

    // UIButton usbConnect("USB", Color(0xFFFFFF), [&]() { MatrixOS::USB::Connect(); });
    // usbUI.AddUIComponent(usbConnect, Point(0, 1));

    usbUI.Start();
    Exit();
}   

void USBtest::Loop() {
    // Do nothing
}

// void USBtest::KeyEvent(uint16_t keyID, KeyInfo* keyInfo) {
// //   if (keyID == FUNCTION_KEY)
// //   {
// //     else if (keyInfo->state == HOLD)
// //     { Exit(); }
// //   }
// }