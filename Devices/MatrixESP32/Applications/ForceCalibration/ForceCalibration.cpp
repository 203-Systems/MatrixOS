#include "ForceCalibration.h"

void ForceCalibration::Setup(const vector<string>& args) {
  MatrixOS::SYS::Rotate(Direction::UP, true);
  // If not force sensitive, then exit
  if (!Device::KeyPad::velocity_sensitivity)
  { return; }

  UI forceCalibrationMenu = UI("Force Calibration", Color(0xFFFFFF));

  UIButton highCalibrationBtn;
  highCalibrationBtn.SetName("Force Max Calibration");
  highCalibrationBtn.SetColorFunc([&]() -> Color { return highCalibrationSaved ? Color(0x00FF00) : Color(0xFF0000); });
  highCalibrationBtn.SetSize(Dimension(4, 2));
  highCalibrationBtn.OnPress([&]() -> void { HighCalibration(); });
  forceCalibrationMenu.AddUIComponent(highCalibrationBtn, Point(2, 0));

  UIButton highOffsetBtn;
  highOffsetBtn.SetName("Force Max Offset");
  highOffsetBtn.SetColor(Color(0x00FFFF));
  highOffsetBtn.SetSize(Dimension(1, 2));
  highOffsetBtn.OnPress([&]() -> void { SetOffset(High); });
  forceCalibrationMenu.AddUIComponent(highOffsetBtn, Point(0, 0));

  UIButton clearHighCalibrationBtn;
  clearHighCalibrationBtn.SetName("Clear Force Max Calibration");
  clearHighCalibrationBtn.SetColor(Color(0xFF00FF));
  clearHighCalibrationBtn.SetSize(Dimension(1, 2));
  clearHighCalibrationBtn.OnPress([&]() -> void { ClearHighCalibration(); });
  forceCalibrationMenu.AddUIComponent(clearHighCalibrationBtn, Point(7, 0));

  UIButton lowCalibrationBtn;
  lowCalibrationBtn.SetName("Activation Force Calibration");
  lowCalibrationBtn.SetColorFunc([&]() -> Color { return lowCalibrationSaved ? Color(0x00FF00) : Color(0xFF0000); });
  lowCalibrationBtn.SetSize(Dimension(4, 2));
  lowCalibrationBtn.OnPress([&]() -> void { LowCalibration(); });
  forceCalibrationMenu.AddUIComponent(lowCalibrationBtn, Point(2, 6));

  UIButton lowOffsetBtn;
  lowOffsetBtn.SetName("Activation Force Offset");
  lowOffsetBtn.SetColor(Color(0x00FFFF));
  lowOffsetBtn.SetSize(Dimension(1, 2));
  lowOffsetBtn.OnPress([&]() -> void { SetOffset(Low); });
  forceCalibrationMenu.AddUIComponent(lowOffsetBtn, Point(0, 6));

  UIButton clearLowCalibrationBtn;
  clearLowCalibrationBtn.SetName("Clear Activation Force Calibration");
  clearLowCalibrationBtn.SetColor(Color(0xFF00FF));
  clearLowCalibrationBtn.SetSize(Dimension(1, 2));
  clearLowCalibrationBtn.OnPress([&]() -> void { ClearLowCalibration(); });
  forceCalibrationMenu.AddUIComponent(clearLowCalibrationBtn, Point(7, 6));

  UIButton keypadVisualizerBtn;
  keypadVisualizerBtn.SetName("Keypad Visualizer");
  keypadVisualizerBtn.SetColor(Color(0xFFFFFF));
  keypadVisualizerBtn.SetSize(Dimension(6, 2));
  keypadVisualizerBtn.OnPress([&]() -> void { ForceGridVisualizer(); });
  forceCalibrationMenu.AddUIComponent(keypadVisualizerBtn, Point(1, 3));

  forceCalibrationMenu.Start();
  Exit();
}

void ForceCalibration::ClearLowCalibration()
{
  lowCalibrationSaved = false;
  Device::KeyPad::FSR::ClearLowCalibration();
}

void ForceCalibration::ClearHighCalibration()
{
  highCalibrationSaved = false;
  Device::KeyPad::FSR::ClearHighCalibration();
}

void ForceCalibration::SetOffset(CalibrationType type)
{
  int16_t offset;
  int8_t upper_bound;
  int8_t lower_bound;

  if (type == Low)
  { 
    offset = Device::KeyPad::FSR::GetLowOffset() / 32;
    upper_bound = 32;
    lower_bound = -32;
  }
  else if (type == High)
  { 
    offset = Device::KeyPad::FSR::GetHighOffset() / 256;
    upper_bound = 64;
    lower_bound = -64;
  }
  else
  { return; }

  UI offsetUI = UI("Set Offset", Color(0x00FFFF));

  int32_t display_value = abs(offset);

  UIButton minusBtn;
  minusBtn.SetName("Subtract one");
  minusBtn.SetColorFunc([&]() -> Color { return Color(0xFF00FF).DimIfNot(offset > lower_bound); });
  minusBtn.SetSize(Dimension(3, 1));
  minusBtn.OnPress([&]() -> void {
    if(offset > lower_bound) {offset--;}
    display_value = abs(offset);
  });
  offsetUI.AddUIComponent(minusBtn, Point(0, 7));

  UIButton plusBtn;
  plusBtn.SetName("Add one");
  plusBtn.SetColorFunc([&]() -> Color { return Color(0x00FFFF).DimIfNot(offset < upper_bound); });
  plusBtn.SetSize(Dimension(3, 1));
  plusBtn.OnPress([&]() -> void {
    if(offset < upper_bound) {offset++;}
    display_value = abs(offset);
  });
  offsetUI.AddUIComponent(plusBtn, Point(5, 7));

  UIButton minusSign;
  minusSign.SetColorFunc([&]() -> Color { return (offset < 0 ? Color(0xFF00FF) : Color(0x00FFFF)); });
  minusSign.SetSize(Dimension(2, 1));
  minusSign.SetEnableFunc([&]() -> bool { return offset < 0; });
  
  offsetUI.AddUIComponent(minusSign, Point(0, 2));


  UI4pxNumber offsetDisplay;
  offsetDisplay.SetColorFunc([&](uint16_t digit) -> Color { return digit % 2 ? Color(0xFFFFFF) : (offset < 0 ? Color(0xFF00FF) : Color(0x00FFFF)); });
  offsetDisplay.SetDigits(2);
  offsetDisplay.SetValuePointer((int32_t*)&display_value);
  offsetUI.AddUIComponent(offsetDisplay, Point(2, 0));

  offsetUI.Start();

  if (type == Low)
  { Device::KeyPad::FSR::SetLowOffset(offset * 32); }
  else if (type == High)
  { Device::KeyPad::FSR::SetHighOffset(offset * 256); }

}

void ForceCalibration::ForceGridVisualizer()
{
  UI forceGridVisualizer = UI("", Color(0xFFFFFF));

  Point activeKey = Point::Invalid();
  forceGridVisualizer.SetPreRenderFunc([&]() -> void {
    for (uint8_t x = 0; x < 8; x++)
    { 
      for (uint8_t y = 0; y < 8; y++)
      {
        KeyInfo* keyInfo = MatrixOS::KeyPad::GetKey(Point(x, y));
        Color color = Color(0xFFFFFF);
        // uint16_t value = (uint16_t)(keyInfo->raw_velocity) >> 8;
        // uint8_t value8 = value > 0xFF ? 0xFF : value & 0xFF;
        if (keyInfo->Force() > 0 && keyInfo->Active()) { color = Color(0x00FFFF).Scale(keyInfo->Force().to8bits());}
        // else if (keyInfo->raw_velocity > 0 && keyInfo->Force() == 0) {
        //     color = Color(0xFFFFFF);
        // }

        if (keyInfo->Force() == FRACT16_MAX) { color = Color(0x00FF00); }
        MatrixOS::LED::SetColor(Point(x, y), color);

        if (keyInfo->Force().to8bits() > 127) { activeKey = Point(x, y); }

        // if (activeKey.x == x && activeKey.y == y)
        // { MLOGD("ForceGridVisualizer", "%d %d\tRaw Read: %d\t16bit: %d\tThreshold: %d\tActive %d", x, y, keyInfo->raw_velocity, keyInfo->Force(), keyInfo->threshold, keyInfo->Active()); }
      } 
    }
  });

  forceGridVisualizer.SetKeyEventHandler([&](KeyEvent* keyEvent) -> bool {
    return keyEvent->id != FUNCTION_KEY;  // Skip all keys except function key
  });

  forceGridVisualizer.Start();
}
