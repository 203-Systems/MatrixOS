#include "ForceGridVisualizer.h"

void ForceGridVisualizer::Setup() {
  // If not force sensitive, then exit
  if (!Device::KeyPad::velocity_sensitivity)
  { return; }
}

void ForceGridVisualizer::Render()
{
  for (uint8_t x = 0; x < 8; x++)
  { 
    for (uint8_t y = 0; y < 8; y++)
    {
      KeyInfo* keyInfo = MatrixOS::KEYPAD::GetKey(Point(x, y));
      Color color = Color(0xFFFFFF);
      // uint16_t value = (uint16_t)(keyInfo->raw_velocity) >> 8;
      // uint8_t value8 = value > 0xFF ? 0xFF : value & 0xFF;
      if (keyInfo->velocity > 0 && keyInfo->active()) { color = Color(0x00FFFF).Scale(keyInfo->velocity.to8bits());}
      // else if (keyInfo->raw_velocity > 0 && keyInfo->velocity == 0) {
      //     color = Color(0xFFFFFF);
      // }

      if (keyInfo->velocity == FRACT16_MAX) { color = Color(0x00FF00); }
      MatrixOS::LED::SetColor(Point(x, y), color);

      if (keyInfo->velocity.to8bits() > 127) { activeKey = Point(x, y); }

      // if (activeKey.x == x && activeKey.y == y)
      // { MLOGD("ForceGridVisualizer", "%d %d\tRaw Read: %d\t16bit: %d\tThreshold: %d\tActive %d", x, y, keyInfo->raw_velocity, keyInfo->velocity, keyInfo->threshold, keyInfo->active()); }
    } 
  }
  MatrixOS::LED::Update();
}

void ForceGridVisualizer::Loop() {  
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { 
    if (keyEvent.id == FUNCTION_KEY && keyEvent.info.state == KeyState::HOLD)
    { Exit(); }
  }

  if (renderTimer.Tick(10))
  {
    Render();
  }
}