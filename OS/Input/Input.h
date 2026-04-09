#pragma once

namespace MatrixOS::Input
{
void Init();
bool NewEvent(const InputEvent& event);
void RegisterKeypadCapabilities(uint8_t clusterId, const KeypadCapabilities& caps);
bool GetKeypadCapabilities(uint8_t clusterId, KeypadCapabilities* caps);
} // namespace MatrixOS::Input
