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
  while (MatrixOS::KeyPad::Get(&keyEvent))
  { KeyEvent(keyEvent.ID(), &keyEvent.info); }
}

void BootAnimation::KeyEvent(uint16_t KeyID, KeyInfo* keyInfo) {
  if (KeyID == FUNCTION_KEY && keyInfo->State() == PRESSED)
  { Exit(); }
}