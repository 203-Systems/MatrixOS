#include "MSC.h"
#include "UI/UI.h"
#include "USB/USB.h"

void MSC::Setup(const vector<string>& args) {
  if (!Device::Storage::Available()) {
    MLOGW("MSC", "Storage not available - cannot enter USB storage mode");
    Exit();
    return;
  }

  MatrixOS::USB::SetMode(USB_MODE_MSC);

  // Create MSC Mode UI
  UI mscUI("USB Storage Mode", Color(0xFF8000));

  // Add instructions text or visual indicators
  mscUI.SetPreRenderFunc([]() -> void {
    // Display "U"
    MatrixOS::LED::SetColor(Point(2, 2), Color(0xFF8000));
    MatrixOS::LED::SetColor(Point(2, 3), Color(0xFF8000));
    MatrixOS::LED::SetColor(Point(2, 4), Color(0xFF8000));
    MatrixOS::LED::SetColor(Point(3, 5), Color(0xFF8000));
    MatrixOS::LED::SetColor(Point(4, 5), Color(0xFF8000));
    MatrixOS::LED::SetColor(Point(5, 2), Color(0xFF8000));
    MatrixOS::LED::SetColor(Point(5, 3), Color(0xFF8000));
    MatrixOS::LED::SetColor(Point(5, 4), Color(0xFF8000));
  });

// Start the MSC UI
  mscUI.Start();

  MatrixOS::USB::SetMode(USB_MODE_NORMAL);
  Exit();
}