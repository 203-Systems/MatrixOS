#include "FactoryMenu.h"
void FactoryMenu::KeyPadTester() {
  bool keypad_tested[X_SIZE][Y_SIZE];
  memset(keypad_tested, false, X_SIZE * Y_SIZE);
  MatrixOS::LED::Fill(0);
  while (!MatrixOS::KEYPAD::GetKey(FUNCTION_KEY)->active())  // Break when fn pressed
  {
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      for (uint8_t y = 0; y < Y_SIZE; y++)
      {
        KeyInfo* keyInfo = MatrixOS::KEYPAD::GetKey(Point(x, y));

        if (keyInfo->active())
        {
          keypad_tested[x][y] = true;
          MatrixOS::LED::SetColor(Point(x, y), Color(0x00FF00));
        }
        else
        { MatrixOS::LED::SetColor(Point(x, y), Color(0xFFFFFF * keypad_tested[x][y])); }
      }
    }
    MatrixOS::LED::Update();
  }
  MatrixOS::KEYPAD::Clear();
  MatrixOS::LED::Fill(0);
}