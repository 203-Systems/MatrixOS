#include "MatrixOS.h"
#include "KeypadInfo.h"

#define DIFFERENCE(a, b) ((a) > (b) ? (a) - (b) : (b) - (a))

inline uint16_t MAX(uint16_t a, uint16_t b) {
  return ((a) > (b) ? a : b);
}

#define BELOW_VALUE_THRESHOLD newValue <= config.lowThreshold
#define ABOVE_THRESHOLD newValue > config.lowThreshold

IRAM_ATTR Fract16 KeypadInfo::ApplyForceCurve(KeypadConfig& config, Fract16 value) {
  if (!config.applyCurve)
  {
    return value;
  }
  if (value < config.lowThreshold)
  {
    value = 0;
  }
  else if (value >= config.highThreshold)
  {
    value = FRACT16_MAX;
  }
  else
  {
    uint32_t preDivisionValue = ((uint16_t)value - (uint16_t)config.lowThreshold) * UINT16_MAX;
    value = (Fract16)(preDivisionValue / ((uint16_t)config.highThreshold - (uint16_t)config.lowThreshold));
  }
  return value;
}

IRAM_ATTR bool KeypadInfo::Update(KeypadConfig& config, Fract16 newValue) {
  uint32_t timeNow = (uint32_t)MatrixOS::SYS::Millis();

  switch (state)
  {
  case KeypadState::Invalid:
  case KeypadState::Released:
    state = KeypadState::Idle;
    lastEventTime = timeNow;
    suppressed = false;
    hold = false;
    [[fallthrough]];
  case KeypadState::Idle:
    if (newValue > config.lowThreshold + config.activationOffset)
    {
      if (config.debounce > 0)
      {
        state = KeypadState::Debouncing;
        lastEventTime = timeNow;
        return false;
      }
      else
      {
        state = KeypadState::Pressed;
        lastEventTime = timeNow;
        pressure = config.applyCurve ? ApplyForceCurve(config, newValue) : newValue;
        return true & !suppressed;
      }
    }
    break;
  case KeypadState::Debouncing:
    if (newValue <= config.lowThreshold + config.activationOffset)
    {
      state = KeypadState::Idle;
      lastEventTime = timeNow;
      return false;
    }
    else if (timeNow - lastEventTime > config.debounce)
    {
      state = KeypadState::Pressed;
      lastEventTime = timeNow;
      pressure = config.applyCurve ? ApplyForceCurve(config, newValue) : newValue;
      return true & !suppressed;
    }
    return false;
  case KeypadState::ReleaseDebouncing:
    if (BELOW_VALUE_THRESHOLD)
    {
      state = KeypadState::Released;
      lastEventTime = timeNow;
      return true & !suppressed;
    }
    else if (ABOVE_THRESHOLD && timeNow - lastEventTime > config.debounce)
    {
      state = KeypadState::Activated;
      lastEventTime = timeNow;
    }
    [[fallthrough]];
  case KeypadState::Pressed:
  case KeypadState::Hold:
  case KeypadState::Aftertouch:
    state = KeypadState::Activated;
    [[fallthrough]];
  case KeypadState::Activated: {
    if (BELOW_VALUE_THRESHOLD)
    {
      if (config.debounce > 0)
      {
        state = KeypadState::ReleaseDebouncing;
        lastEventTime = timeNow;
        return false;
      }
      else
      {
        state = KeypadState::Released;
        lastEventTime = timeNow;
        return true & !suppressed;
      }
    }
    // Apply force curve
    newValue = config.applyCurve ? ApplyForceCurve(config, newValue) : newValue;

    if (timeNow - lastEventTime > kHoldThreshold && !hold)
    {
      state = KeypadState::Hold;
      pressure = newValue;
      hold = true;
      return true & !suppressed;
    }
    else if ((DIFFERENCE((uint16_t)newValue, (uint16_t)pressure) > KEY_SCAN_THRESHOLD) ||
             ((newValue != pressure) && newValue == FRACT16_MAX))
    {
      state = KeypadState::Aftertouch;
      pressure = newValue;
      return true & !suppressed;
    }
    return false;
  }
  }
  return false;
}

void KeypadInfo::Suppress() {
  if (state == KeypadState::Pressed || state == KeypadState::Activated ||
      state == KeypadState::Hold || state == KeypadState::Aftertouch)
  {
    suppressed = true;
  }
}

uint32_t KeypadInfo::HoldTime() {
  if (Active())
  {
    return (uint32_t)MatrixOS::SYS::Millis() - lastEventTime;
  }
  return 0;
}
