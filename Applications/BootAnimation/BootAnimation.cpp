#include "BootAnimation.h"

void BootAnimation::Loop() {
  switch (stage)
  {
  case 0:
    if (Idle(MatrixOS::USB::Connected()))
    {
      stage++;
    }
    break;
  case 1:
    Boot();
    break;
  default:
    Exit();
  }

  InputEvent inputEvent;
  while (MatrixOS::Input::Get(&inputEvent))
  {
    KeyEvent(inputEvent.id, &inputEvent.keypad);
  }
}

void BootAnimation::KeyEvent(InputId inputId, KeypadInfo* keypadInfo) {
  if (inputId == InputId::FunctionKey() && keypadInfo->state == KeypadState::Pressed)
  {
    Exit();
  }
}
