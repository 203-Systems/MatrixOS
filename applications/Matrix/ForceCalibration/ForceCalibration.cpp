#include "ForceCalibration.h"

void ForceCalibration::Setup() {
  MatrixOS::SYS::Rotate(EDirection::UP, true);
  // If not force sensitive, then exit
  if (!Device::KeyPad::velocity_sensitivity)
  { return; }

  UI forceCalibrationMenu = UI("Force Calibration", Color(0xFFFFFF));

  UIButton LowCalibrationBtn;
  LowCalibrationBtn.SetName("Low Calibration");
  LowCalibrationBtn.SetColorFunc([&]() -> Color { return lowCalibrationSaved ? Color(0x00FF00) : Color(0xFF0000); });
  LowCalibrationBtn.SetSize(Dimension(6, 2));
  LowCalibrationBtn.OnPress([&]() -> void { LowCalibration(); });
  forceCalibrationMenu.AddUIComponent(LowCalibrationBtn, Point(1, 1));

  UIButton HighCalibrationBtn;
  HighCalibrationBtn.SetName("High Calibration");
  HighCalibrationBtn.SetColorFunc([&]() -> Color { return highCalibrationSaved ? Color(0x00FF00) : Color(0xFF0000); });
  HighCalibrationBtn.SetSize(Dimension(6, 2));
  HighCalibrationBtn.OnPress([&]() -> void { HighCalibration(); });
  forceCalibrationMenu.AddUIComponent(HighCalibrationBtn, Point(1, 5));

  forceCalibrationMenu.AddUIComponent(HighCalibrationBtn, Point(1, 5));

  forceCalibrationMenu.Start();
  Exit();
}
