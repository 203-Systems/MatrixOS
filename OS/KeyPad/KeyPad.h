#pragma once

namespace MatrixOS::KeyPad
{
bool NewEvent(KeyEvent* keyevent); // Bridge: forward to MatrixOS::Input, will be removed when drivers emit InputEvent directly
} // namespace MatrixOS::KeyPad