#pragma once

#include "InputEvent.h"
#include "KeyEvent.h"

// Compatibility helpers for converting between old KeyEvent/KeyInfo and new InputEvent/KeypadInfo.
// These enable gradual migration from the old MatrixOS::KeyPad model to MatrixOS::Input.

// Convert KeypadState → old KeyState
inline KeyState KeypadStateToKeyState(KeypadState state) {
  switch (state)
  {
  case KeypadState::Idle: return IDLE;
  case KeypadState::Activated: return ACTIVATED;
  case KeypadState::Pressed: return PRESSED;
  case KeypadState::Hold: return HOLD;
  case KeypadState::Aftertouch: return AFTERTOUCH;
  case KeypadState::Released: return RELEASED;
  case KeypadState::Debouncing: return DEBOUNCING;
  case KeypadState::ReleaseDebouncing: return RELEASE_DEBOUNCING;
  default: return IDLE;
  }
}

// Convert old KeyState → KeypadState
inline KeypadState KeyStateToKeypadState(KeyState state) {
  switch (state)
  {
  case IDLE: return KeypadState::Idle;
  case ACTIVATED: return KeypadState::Activated;
  case PRESSED: return KeypadState::Pressed;
  case HOLD: return KeypadState::Hold;
  case AFTERTOUCH: return KeypadState::Aftertouch;
  case RELEASED: return KeypadState::Released;
  case DEBOUNCING: return KeypadState::Debouncing;
  case RELEASE_DEBOUNCING: return KeypadState::ReleaseDebouncing;
  default: return KeypadState::Idle;
  }
}

// Synthesize a KeyInfo from KeypadInfo (for passing to legacy UIComponent::KeyEvent)
inline KeyInfo KeypadInfoToKeyInfo(const KeypadInfo& kpi) {
  KeyInfo ki;
  ki.lastEventTime = kpi.lastEventTime;
  ki.state = KeypadStateToKeyState(kpi.state);
  ki.hold = kpi.hold;
  ki.cleared = kpi.cleared;
  ki.values[0] = kpi.pressure;
  ki.values[1] = kpi.velocity;
  ki.values[2] = 0;
  return ki;
}

// Extract KeypadInfo from a KeyInfo (for building InputEvent from legacy data)
inline KeypadInfo KeyInfoToKeypadInfo(const KeyInfo& ki) {
  KeypadInfo kpi;
  kpi.lastEventTime = ki.lastEventTime;
  kpi.pressure = ki.Force();
  kpi.velocity = ki.Value(1);
  kpi.state = KeyStateToKeypadState(ki.state);
  kpi.hold = ki.hold ? 1 : 0;
  kpi.cleared = ki.cleared ? 1 : 0;
  kpi.reserved = 0;
  return kpi;
}
