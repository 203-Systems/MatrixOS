#include "ForceCalibration.h"

void ForceCalibration::Setup() {
  MatrixOS::SYS::Rotate(EDirection::UP, true);
  // If not force sensitive, then exit
  if (!Device::KeyPad::velocity_sensitivity)
  { return; }

  UI forceCalibrationMenu = UI("Force Calibration", Color(0xFFFFFF));

  UIButtonLargeWithColorFunc LowCalibrationBtn(
      "Low Calibration", [&]() -> Color { return lowCalibrationSaved ? Color(0x00FF00) : Color(0xFF0000); }, Dimension(6, 2),
      [&]() -> void { LowCalibration(); });
  forceCalibrationMenu.AddUIComponent(LowCalibrationBtn, Point(1, 1));

  UIButtonLargeWithColorFunc HighCalibrationBtn(
      "High Calibration", [&]() -> Color { return highCalibrationSaved ? Color(0x00FF00) : Color(0xFF0000); }, Dimension(6, 2),
      [&]() -> void { HighCalibration(); });

  forceCalibrationMenu.AddUIComponent(HighCalibrationBtn, Point(1, 5));

  forceCalibrationMenu.Start();
  Exit();
}
