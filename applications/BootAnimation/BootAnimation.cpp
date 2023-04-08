#include "BootAnimation.h"

void BootAnimation::Loop() {
  switch(stage)
  {
    case 0:
      if(Idle(MatrixOS::USB::Connected())) {stage++;}
      break;
    case 1:
      Boot();
      break;
    default:
      Exit();
  }

  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEvent(keyEvent.id, &keyEvent.info); }
}

void BootAnimation::KeyEvent(uint16_t KeyID, KeyInfo* keyInfo) {
  if (KeyID == FUNCTION_KEY && keyInfo->state == PRESSED)
  { Exit(); }
}