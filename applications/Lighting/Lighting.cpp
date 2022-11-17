#include "Lighting.h"

void Lighting::Setup() {
  MatrixOS::LED::Fill(color);
  MatrixOS::LED::Update();
}

void Lighting::Loop()
{
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEvent(keyEvent.id, &keyEvent.info); }
}

void Lighting::KeyEvent(uint16_t keyID, KeyInfo* keyInfo) {
  if (keyID == FUNCTION_KEY)
  {
    if (keyInfo->state == RELEASED)
    {
      if(MatrixOS::UIInterface::ColorPicker(color.value))
      { color.Set(color.value); }
      MatrixOS::LED::Fill(color.value);
      MatrixOS::LED::Update();
    }
    else if (keyInfo->state == HOLD)
    { Exit(); }
  }
}