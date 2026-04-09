#include "FactoryMenu.h"
void FactoryMenu::KeyPadTester() {
  bool keypadTested[X_SIZE][Y_SIZE];
  InputSnapshot fnSnap = {};
  memset(keypadTested, false, X_SIZE * Y_SIZE);
  MatrixOS::LED::Fill(0);
  while (!(MatrixOS::Input::GetState(InputId::FunctionKey(), &fnSnap) &&
           fnSnap.inputClass == InputClass::Keypad && fnSnap.keypad.Active()))
  {
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      for (uint8_t y = 0; y < Y_SIZE; y++)
      {
        KeypadInfo keypadState = {};
        {
          const InputCluster* grid = MatrixOS::Input::GetPrimaryGridCluster();
          InputId id; InputSnapshot snap;
          if (grid && MatrixOS::Input::GetInputAt(grid->clusterId, Point(x, y), &id) &&
              MatrixOS::Input::GetState(id, &snap) && snap.inputClass == InputClass::Keypad) {
            keypadState = snap.keypad;
          }
        }

        if (keypadState.Active())
        {
          keypadTested[x][y] = true;
          MatrixOS::LED::SetColor(Point(x, y), Color(0x00FF00));
        }
        else
        {
          MatrixOS::LED::SetColor(Point(x, y), Color(0xFFFFFF * keypadTested[x][y]));
        }
      }
    }
    MatrixOS::LED::Update();
  }
  MatrixOS::Input::ClearState();
  MatrixOS::LED::Fill(0);
}
