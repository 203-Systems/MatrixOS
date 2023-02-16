#include "CustomKeymap.h"
#include "cb0r.h"

void CustomKeymap::Setup() {

}

void CustomKeymap::Loop() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEventHandler(keyEvent.id, &keyEvent.info); }
}