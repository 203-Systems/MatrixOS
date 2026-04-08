#pragma once

namespace MatrixOS::Input
{
void Init();
bool NewEvent(const InputEvent& event);
void RegisterKeypadCapabilities(uint8_t clusterId, const KeypadCapabilities& caps);
bool GetKeypadCapabilities(uint8_t clusterId, KeypadCapabilities* caps);
InputId GetFunctionKeyId();
bool IsFunctionKey(InputId id);
} // namespace MatrixOS::Input
