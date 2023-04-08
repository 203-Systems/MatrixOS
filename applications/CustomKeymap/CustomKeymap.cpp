#include "CustomKeymap.h"
#include "sample.h"

void CustomKeymap::Setup() {
  if(!uad.LoadUAD((uint8_t*)sample_uad, sizeof(sample_uad)))
  {
    MLOGE("CustomKeymap", "Failed to load UAD");
  }
}

void CustomKeymap::Loop() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEventHandler(keyEvent.id, &keyEvent.info); }
}

void CustomKeymap::KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo) {
  // Reserve Function Key 
  if(KeyID == FUNCTION_KEY && keyInfo->state == KeyState::HOLD) { Exit(); }
  uad.KeyEvent(KeyID, keyInfo);
}