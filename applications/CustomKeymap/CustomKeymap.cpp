#include "CustomKeymap.h"
#include "sample.h"

void CustomKeymap::Setup() {
  uad = UAD((uint8_t*)sample_uad, sizeof(sample_uad));
}

void CustomKeymap::Loop() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEventHandler(keyEvent.id, &keyEvent.info); }
}

void CustomKeymap::KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo) {
  // Reserve Function Key 
  if(KeyID == FUNCTION_KEY)
  {
    return;
  }
  uad.KeyEvent(KeyID, keyInfo);
}