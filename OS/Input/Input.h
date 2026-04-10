#pragma once

namespace MatrixOS::Input
{
void Init();
bool NewEvent(const InputEvent& event);
void RegisterKeypadCapabilities(uint8_t clusterId, const KeypadCapabilities& caps);
bool GetKeypadCapabilities(uint8_t clusterId, KeypadCapabilities* caps);

// Internal: invalidate the snapshot cache so GetState() won't return
// stale pressed/hold entries after a context transition.
// Not part of the public API (not in MatrixOS.h).
void InvalidateStateCache();
} // namespace MatrixOS::Input
