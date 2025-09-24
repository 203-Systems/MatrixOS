#include "MatrixOS.h"
#include "KeyInfo.h"

#define DIFFERENCE(a, b) ((a) > (b) ? (a) - (b) : (b) - (a))

inline uint16_t MAX(uint16_t a, uint16_t b) {
  return ((a) > (b) ? a : b);
}

#define BELOW_VALUE_THRESHOLD new_value <=  config.low_threshold
#define ABOVE_THRESHOLD new_value > config.low_threshold

// Constructor is defined in the header file

bool KeyInfo::Active() {
  return state >= ACTIVATED && state <= AFTERTOUCH;
}

KeyInfo::operator bool() { 
  return Active(); 
}

uint32_t KeyInfo::HoldTime(void) {
  if (Active())
  {
    return MatrixOS::SYS::Millis() - lastEventTime;
  }
  else
  {
    return 0;
  }
}

Fract16 KeyInfo::ApplyVelocityCurve(KeyConfig& config, Fract16 value) {
  if (!config.apply_curve)
  { return value; }
  if (value < config.low_threshold)
  { value = 0; }
  else if (value >= config.high_threshold)
  {
    // uint32_t source = value;
    value = FRACT16_MAX;
    // MatrixOS::Logging::LogDebug("Velocity Curve", "%d - %d", source, value);
  }
  else
  {
    // uint32_t source = value;
    uint32_t pre_division_value = ((uint16_t)value - (uint16_t)config.low_threshold) * UINT16_MAX;
    value = (Fract16)(pre_division_value / ((uint16_t)config.high_threshold - (uint16_t)config.low_threshold));
    // MatrixOS::Logging::LogDebug("Velocity Curve", "%d - %d", source, value);
  }
  return value;
}

/*
Action Checklist:
Nothing (All)
To Long Term State(Pressed, Hold, Release)
Active (Idle, Release)
Release(Pressed, Active, Hold, Hold Activated)
Aftertouch (Pressed, Activated, Hold, Hold Activated)
*/

// Update method for single value (primary value - force/pressure)
bool KeyInfo::Update(KeyConfig& config, Fract16 new_value) {
  uint32_t timeNow = MatrixOS::SYS::Millis();

  // Handle debouncing states first
  if (debouncing) {
    if (state == IDLE) { // Press debouncing
      if (new_value <= config.low_threshold + config.activation_offset) {
        // MatrixOS::Logging::LogVerbose("KeyInfo", "DEBOUNCING -> IDLE");
        debouncing = false;
        lastEventTime = timeNow;
        return false;
      }
      else if (timeNow - lastEventTime > config.debounce) {
        state = PRESSED;
        debouncing = false;
        // MatrixOS::Logging::LogVerbose("KeyInfo", "DEBOUNCING -> PRESSED");
        lastEventTime = timeNow;
        values[0] = config.apply_curve ? ApplyVelocityCurve(config, new_value) : new_value;
        return true & !cleared;
      }
      return false;
    }
    else if (state == ACTIVATED) { // Release debouncing
      if (BELOW_VALUE_THRESHOLD) {
        state = RELEASED;
        debouncing = false;
        // MatrixOS::Logging::LogVerbose("KeyInfo", "RELEASE_DEBOUNCING -> RELEASED");
        lastEventTime = timeNow;
        values[0] = config.apply_curve ? ApplyVelocityCurve(config, new_value) : new_value;
        return true & !cleared;
      }
      else if (ABOVE_THRESHOLD && timeNow - lastEventTime > config.debounce) {
        debouncing = false;
        // MatrixOS::Logging::LogVerbose("KeyInfo", "RELEASE_DEBOUNCING -> ACTIVATED");
        lastEventTime = timeNow;
      }
      return false;
    }
  }

  switch (state)
  {
    case INVALID:
    case RELEASED:
      state = IDLE;
      // MatrixOS::Logging::LogVerbose("KeyInfo", "RELEASED -> IDLE");
      lastEventTime = timeNow;
      cleared = false;
      hold = false;
      [[fallthrough]];
    case IDLE:
      if (new_value > config.low_threshold + config.activation_offset)
      {
        if(config.debounce > 0)
        {
          // MatrixOS::Logging::LogVerbose("KeyInfo", "IDLE -> DEBOUNCING");
          debouncing = true;
          lastEventTime = timeNow;
          return false;
        }
        else
        {
          state = PRESSED;
          // MatrixOS::Logging::LogVerbose("KeyInfo", "IDLE -> PRESSED");
          lastEventTime = timeNow;
          values[0] = config.apply_curve ? ApplyVelocityCurve(config, new_value) : new_value;
          return true & !cleared;
        }
      }
      break;
    case PRESSED:
    case HOLD:
    case AFTERTOUCH:
      state = ACTIVATED;
      // MatrixOS::Logging::LogVerbose("KeyInfo", "PRESSED/HOLD/AFTERTOUCH -> ACTIVATED");
      [[fallthrough]];
    case ACTIVATED:
    {
      if (BELOW_VALUE_THRESHOLD)
      {
        if(config.debounce > 0)
        {
          // MatrixOS::Logging::LogVerbose("KeyInfo", "ACTIVATED -> RELEASE_DEBOUNCING");
          debouncing = true;
          lastEventTime = timeNow;
          return false;
        }
        else
        {
          state = RELEASED;
          // MatrixOS::Logging::LogVerbose("KeyInfo", "ACTIVATED -> RELEASED");
          lastEventTime = timeNow;
          values[0] = 0;
          return true & !cleared;
        }
      }
      // Apply value curve
      Fract16 processed_value = config.apply_curve ? ApplyVelocityCurve(config, new_value) : new_value;

      if (timeNow - lastEventTime > hold_threshold && !hold)
      {
        state = HOLD;
        // MatrixOS::Logging::LogVerbose("KeyInfo", "ACTIVATED -> HOLD");
        values[0] = processed_value;
        hold = true;
        return true & !cleared;
      }
      // Check if value has changed significantly
      else if((DIFFERENCE((uint16_t)processed_value, (uint16_t)values[0]) > KEY_INFO_THRESHOLD) ||
              ((processed_value != values[0]) && processed_value == FRACT16_MAX))
      {
        state = AFTERTOUCH;
        // MatrixOS::Logging::LogVerbose("KeyInfo", "ACTIVATED -> AFTERTOUCH");
        values[0] = processed_value;
        return true & !cleared;
      }
      return false;
    }
  }
  return false;
}

void KeyInfo::Clear() {
  if (state == PRESSED || state == ACTIVATED || state == HOLD || state == AFTERTOUCH)
  { cleared = true; }
}

// User access methods
KeyState KeyInfo::State() {
  return state;
}

bool KeyInfo::Hold() {
  return hold;
}

Fract16 KeyInfo::Force() const {
  return values[0];
}

Fract16 KeyInfo::Value(uint8_t index) const {
  return (index < KEY_INFO_VALUE_COUNT) ? values[index] : 0;
}

// UpdateRaw method - directly update a value without state machine processing
bool KeyInfo::UpdateRaw(uint8_t index, Fract16 new_value) {
  if (index >= KEY_INFO_VALUE_COUNT) {
    return false;
  }

  // Check delta for primary value (index 0) and trigger AFTERTOUCH if needed
  if (state == ACTIVATED) {
    Fract16 old_value = values[0];
    if (DIFFERENCE((uint16_t)new_value, (uint16_t)old_value) >= KEY_INFO_THRESHOLD) {
      state = AFTERTOUCH;
      values[index] = new_value;
    }
  }

  return true;
}
