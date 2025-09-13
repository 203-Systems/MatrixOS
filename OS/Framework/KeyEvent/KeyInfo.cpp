#include "MatrixOS.h"
#include "KeyInfo.h"

#define DIFFERENCE(a, b) ((a) > (b) ? (a) - (b) : (b) - (a))

inline uint16_t MAX(uint16_t a, uint16_t b) {
  return ((a) > (b) ? a : b);
}

#define BELOW_VELOCITY_THRESHOLD new_velocity <=  config.low_threshold
#define ABOVE_THRESHOLD new_velocity > config.low_threshold

KeyInfo::KeyInfo() {}

bool KeyInfo::Active() { 
  return (state >= ACTIVATED && state <= AFTERTOUCH) || state == RELEASE_DEBUNCING; 
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

Fract16 KeyInfo::ApplyVelocityCurve(KeyConfig& config, Fract16 velocity) {
  if (!config.apply_curve)
  { return velocity; }
  if (velocity < config.low_threshold)
  { velocity = 0; }
  else if (velocity >= config.high_threshold)
  {
    // uint32_t source = velocity;
    velocity = FRACT16_MAX;
    // MatrixOS::Logging::LogDebug("Velocity Curve", "%d - %d", source, velocity);
  }
  else
  {
    // uint32_t source = velocity;
    uint32_t pre_division_velocity = ((uint16_t)velocity - (uint16_t)config.low_threshold) * UINT16_MAX;
    velocity = (Fract16)(pre_division_velocity / ((uint16_t)config.high_threshold - (uint16_t)config.low_threshold));
    // MatrixOS::Logging::LogDebug("Velocity Curve", "%d - %d", source, velocity);
  }
  return velocity;
}

/*
Action Checklist:
Nothing (All)
To Long Term State(Pressed, Hold, Release)
Active (Idle, Release)
Release(Pressed, Active, Hold, Hold Activated)
Aftertouch (Pressed, Activated, Hold, Hold Activated)
*/

bool KeyInfo::Update(KeyConfig& config, Fract16 new_velocity) {
  uint32_t timeNow = MatrixOS::SYS::Millis();

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
      if (new_velocity > config.low_threshold + config.activation_offset)
      {
        if(config.debounce > 0)
        {
        // MatrixOS::Logging::LogVerbose("KeyInfo", "IDLE -> DEBUNCING");
        state = DEBUNCING;
        lastEventTime = timeNow;
        return false;
        }
        else
        {
          state = PRESSED;
          // MatrixOS::Logging::LogVerbose("KeyInfo", "IDLE -> PRESSED");
          lastEventTime = timeNow;
          velocity = config.apply_curve ? ApplyVelocityCurve(config, new_velocity) : new_velocity;
          return true & !cleared;
        }
      }
      break;
    case DEBUNCING:
      if (new_velocity <= config.low_threshold + config.activation_offset)
      {
        // MatrixOS::Logging::LogVerbose("KeyInfo", "DEBUNCING -> IDLE");
        state = IDLE;
        lastEventTime = timeNow;
        return false;
      }
      else if (timeNow - lastEventTime > config.debounce)
      {
        state = PRESSED;
        // MatrixOS::Logging::LogVerbose("KeyInfo", "DEBUNCING -> PRESSED");
        lastEventTime = timeNow;
        velocity = config.apply_curve ? ApplyVelocityCurve(config, new_velocity) : new_velocity;
        return true & !cleared; // I know just return "!cleared" works but I want to make it clear this is suppose to return true
      }
      return false;
    case RELEASE_DEBUNCING:
      if (BELOW_VELOCITY_THRESHOLD)
      {
        state = RELEASED;
        // MatrixOS::Logging::LogVerbose("KeyInfo", "RELEASE_DEBUNCING -> RELEASED");
        lastEventTime = timeNow;
        return true & !cleared;
      }
      else if (ABOVE_THRESHOLD && timeNow - lastEventTime > config.debounce)
      {
        state = ACTIVATED;
        // MatrixOS::Logging::LogVerbose("KeyInfo", "RELEASE_DEBUNCING -> ACTIVATED");
        lastEventTime = timeNow;
      }
      [[fallthrough]];
    case PRESSED:
    case HOLD:
    case AFTERTOUCH:
      state = ACTIVATED;
      // MatrixOS::Logging::LogVerbose("KeyInfo", "PRESSED/HOLD/AFTERTOUCH -> ACTIVATED");
      [[fallthrough]];
    case ACTIVATED:
    {
      if (BELOW_VELOCITY_THRESHOLD)
      {
        if(config.debounce > 0)
        {
          state = RELEASE_DEBUNCING;
          // MatrixOS::Logging::LogVerbose("KeyInfo", "ACTIVATED -> RELEASE_DEBUNCING");
          lastEventTime = timeNow;
          return false;
        }
        else
        {
          state = RELEASED;
          // MatrixOS::Logging::LogVerbose("KeyInfo", "ACTIVATED -> RELEASED");
          lastEventTime = timeNow;
          return true & !cleared;
        }
      }
      // Apply velocity Curve
      new_velocity = config.apply_curve ? ApplyVelocityCurve(config, new_velocity) : new_velocity;

      if (timeNow - lastEventTime > hold_threshold && !hold)
      {
        state = HOLD;
        // MatrixOS::Logging::LogVerbose("KeyInfo", "ACTIVATED -> HOLD");
        velocity = new_velocity;
        hold = true;
        return true & !cleared;
      }
      else if((DIFFERENCE((uint16_t)new_velocity, (uint16_t)velocity) > KEY_INFO_THRESHOLD) || ((new_velocity != velocity) && new_velocity == FRACT16_MAX))
      {
        state = AFTERTOUCH;
        // MatrixOS::Logging::LogVerbose("KeyInfo", "ACTIVATED -> AFTERTOUCH");
        velocity = new_velocity;
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