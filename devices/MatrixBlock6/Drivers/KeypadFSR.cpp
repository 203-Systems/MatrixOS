#include "Family.h"

namespace Device::KeyPad::FSR
{
  Fract16 (*low_thresholds)[8][8] = nullptr;
  Fract16 (*high_thresholds)[8][8] = nullptr;

  void Init() {
    // Dummy implementation
  }

  void SaveLowCalibration()
  {
    // Dummy implementation
  }

  void SaveHighCalibration()
  {
    // Dummy implementation
  }

  void ClearLowCalibration()
  {
    // Dummy implementation
  }

  void ClearHighCalibration()
  {
    // Dummy implementation
  }

  int16_t GetLowOffset()
  {
    return 0; // Dummy return
  }

  int16_t GetHighOffset()
  {
    return 0; // Dummy return
  }

  void SetLowOffset(int16_t offset)
  {
    // Dummy implementation
  }

  void SetHighOffset(int16_t offset)
  {
    // Dummy implementation
  }

  uint32_t GetScanCount()
  {
    return 0; // Dummy return
  }

  uint16_t GetRawReading(uint8_t x, uint8_t y)
  {
    return 0; // Dummy return
  }

  void Start() {
    // Dummy implementation
  }
  
  bool Scan() {
    return false; // Dummy return
  }
}