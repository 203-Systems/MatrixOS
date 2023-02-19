#include "CustomKeymap.h"

void CustomKeymap::Setup() {

}

void CustomKeymap::Loop() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEventHandler(keyEvent.id, &keyEvent.info); }
}

void CustomKeymap::KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo) {

}