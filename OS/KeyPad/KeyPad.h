#pragma once

namespace MatrixOS::KeyPad
{
    void Init(void);
    bool NewEvent(KeyEvent* keyevent);  // Adding keyevent, return true when queue is full
}