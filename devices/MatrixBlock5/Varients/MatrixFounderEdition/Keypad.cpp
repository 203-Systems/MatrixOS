// Define Device Keypad Function
#include "Device.h"

namespace Device::KeyPad
{
  uint16_t* Scan() {
    clearList();

    if (!isListFull())
      FNScan();  // Prob not need to check if list is full but it makes the code looks nicer
    if (!isListFull())
      KeyPadScan();
    if (!isListFull())
      TouchBarScan();

    return changeList;
  }

  void Clear() {
    fnState.Clear();

    for (uint8_t x = 0; x < x_size; x++)
    {
      for (uint8_t y = 0; y < y_size; y++)
      { keypadState[x][y].Clear(); }
    }

    for (uint8_t i = 0; i < touchbar_size; i++)
    { touchbarState[i].Clear(); }
  }

  KeyInfo GetKey(Point keyXY) {
    uint16_t keyID = XY2ID(keyXY);
    return GetKey(keyID);
  }

  KeyInfo GetKey(uint16_t keyID) {
    uint8_t keyClass = keyID >> 12;
    switch (keyClass)
    {
      case 0:  // System
      {
        uint16_t index = keyID & (0b0000111111111111);
        switch (index)
        {
          case 0:
            return fnState;
        }
        break;
      }
      case 1:  // Main Grid
      {
        int16_t x = (keyID & (0b0000111111000000)) >> 6;
        int16_t y = keyID & (0b0000000000111111);
        if (x < x_size && y < y_size)
          return keypadState[x][y];
        break;
      }
      case 2:  // Touch Bar
      {
        uint16_t index = keyID & (0b0000111111111111);
        if (index < touchbar_size)
          return touchbarState[index];
        break;
      }
    }
    return KeyInfo();  // Return an empty KeyInfo
  }

  void FNScan() {
    Fract16 read = HAL_GPIO_ReadPin(FN_GPIO_Port, FN_Pin) * UINT16_MAX;
    if (fnState.update(read))
    { addToList(0); }
  }

  void KeyPadScan() {
    for (uint8_t x = 0; x < Device::x_size; x++)
    {
      HAL_GPIO_WritePin(keypad_write_pins[x].port, keypad_write_pins[x].pin, GPIO_PIN_SET);
      for (uint8_t y = 0; y < Device::y_size; y++)
      {
        Fract16 read = HAL_GPIO_ReadPin(keypad_read_pins[y].port, keypad_read_pins[y].pin) * UINT16_MAX;
        bool updated = keypadState[x][y].update(read);
        if (updated)
        {
          uint16_t keyID = (1 << 12) + (x << 6) + y;
          if (addToList(keyID))
          {
            HAL_GPIO_WritePin(keypad_write_pins[x].port, keypad_write_pins[x].pin, GPIO_PIN_RESET);  // Set pin back to
                                                                                                     // low
            return;  // List is full
          }
        }
      }
      HAL_GPIO_WritePin(keypad_write_pins[x].port, keypad_write_pins[x].pin, GPIO_PIN_RESET);
      volatile int i;
      for (i = 0; i < 5; ++i)
      {}  // Add small delay
    }
  }

  void TouchBarScan() {}

  bool addToList(uint16_t keyID) {
    if (isListFull())
      return true;  // Prevent overwrite

    changeList[0]++;
    changeList[changeList[0]] = keyID;
    return isListFull();
  }

  void clearList() {
    changeList[0] = 0;
  }

  bool isListFull() {
    return changeList[0] == MULTIPRESS;
  }

  uint16_t XY2ID(Point xy) {
    if (xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)  // Main grid
    { return (1 << 12) + (xy.x << 7) + xy.y; }
    // else if(xy.x >= 0 && xy.x < 8 && xy.y == 8) //Touch Bar
    // {
    //     return (2 << 12) + xy.x;
    // }
    return UINT16_MAX;
  }

  // Matrix use the following ID Struct
  // CCCC IIIIIIIIIIII
  // C as class (4 bits), I as index (12 bits). I could be splited by the class defination, for example, class 0 (grid),
  // it's splited to XXXXXXX YYYYYYY. Class List: Class 0 - System - IIIIIIIIIIII Class 1 - Grid - XXXXXX YYYYYY Class 2
  // - TouchBar - IIIIIIIIIIII Class 3 - Underglow - IIIIIIIIIIII

  Point ID2XY(uint16_t keyID) {
    uint8_t keyClass = keyID >> 12;
    switch (keyClass)
    {
      case 1:  // Main Grid
      {
        int16_t x = (keyID & 0b0000111111000000) >> 6;
        int16_t y = keyID & (0b0000000000111111);
        if (x < Device::x_size && y < Device::y_size)
          return Point(x, y);
        break;
      }
      case 2:  // TouchBar
      {
        uint16_t index = keyID & (0b0000111111111111);
        if (index < Device::touchbar_size)
          return Point(Device::y_size, index);
        break;
      }
    }
    return Point(INT16_MIN, INT16_MIN);
  }
}