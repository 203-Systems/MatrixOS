#include "Lighting.h"

void Lighting::Setup() {
  MatrixOS::LED::Fill(colors[color_index]);
  MatrixOS::LED::Update();
}

// void Lighting::Loop()
// {

// }

void Lighting::KeyEvent(uint16_t keyID, KeyInfo* keyInfo) {
  if (keyID == FUNCTION_KEY)
  {
    if (keyInfo->state == RELEASED)
    {
      color_index++;
      color_index %= 7;
      MatrixOS::LED::Fill(colors[color_index]);
      MatrixOS::LED::Update();
    }
    else if (keyInfo->state == HOLD)
    { Exit(); }
  }
}