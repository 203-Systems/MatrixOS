
// I really don't like how this works atm since it has to go back to System Layer
// Device Keypad Driver -> KeyInfo(Update) -> Apply System configured velocity curve (with user parameter) -> Update
#pragma once

#include <stdint.h>
#include "SavedVariable.h"
#include "Types.h"
#include "system/Parameters.h"

#define KEY_INFO_THRESHOLD 512
// 1/127 - Key Velocity has to move beyond this range in order for after touch to be triggered

// Avoid recuesive include
namespace MatrixOS::SYS
{
  uint32_t Millis(void);
}

namespace MatrixOS::Logging
{
  void LogError(const string &tag, const string &format, ...);
  void LogWarning(const string &tag, const string &format, ...);
  void LogInfo(const string &tag, const string &format, ...);
  void LogDebug(const string &tag, const string &format, ...);
  // void LogVerbose(const string &tag, const string &format, ...);
}


struct KeyConfig {
  bool apply_curve;
  Fract16 low_threshold;
  Fract16 high_threshold;
  Fract16 activation_threshold;
  uint16_t debounce;
};

enum KeyState : uint8_t { /*Status Keys*/ IDLE,
                           ACTIVATED,
                           /*Event Keys*/ PRESSED,
                           RELEASED,
                           HOLD,
                           AFTERTOUCH,
                           /*Special*/ DEBUNCING = 240u, RELEASE_DEBUNCING = 241u,
                           /*Placeholder Keys*/ INVAILD = 255u };
// When adding new state, remember to update active() as well

struct KeyInfo {
  KeyState state = IDLE;
  uint32_t lastEventTime = 0;  // PRESSED and RELEASED event only
  Fract16 velocity = 0;
  bool hold = false;
  bool cleared = false;

  KeyInfo() {}

  uint32_t holdTime(void) {
    if (state == IDLE)
      return 0;

    if (lastEventTime > MatrixOS::SYS::Millis())
      return 0;

    return MatrixOS::SYS::Millis() - lastEventTime;
  }

  bool active() { return (state >= ACTIVATED && state <= AFTERTOUCH) || state == RELEASE_DEBUNCING; }

  operator bool() { return active(); }

  /*
  Action Checklist:
  Nothing (All)
  To Long Term State(Pressed, Hold, Release)
  Active (Idle, Release)
  Release(Pressed, Active, Hold, Hold Actived)
  Aftertouch (Pressed, Actived, Hold, Hold Actived)
  */

#define DIFFERENCE(a, b) ((a) > (b) ? (a) - (b) : (b) - (a))

  inline uint16_t MAX(uint16_t a, uint16_t b) {
    return ((a) > (b) ? a : b);
  }

  Fract16 applyVelocityCurve(KeyConfig& config, Fract16 velocity) {
    if (!config.apply_curve)
    { return velocity; }
    if (velocity < config.low_threshold)
    { velocity = 0; }
    else if (velocity >= config.high_threshold)
    {
      velocity = UINT16_MAX;
      // MLOGD("Velocity Curve", "%d - %d", source, velocity);
    }
    else
    {
      uint32_t pre_division_velocity = ((uint16_t)velocity - (uint16_t)config.low_threshold) * UINT16_MAX;
      velocity = (Fract16)(pre_division_velocity / ((uint16_t)config.high_threshold - (uint16_t)config.low_threshold));
      // MLOGD("Velocity Curve", "%d - %d", source, velocity);
    }
    return velocity;
  }

  #define BELOW_VELOCITY_THRESHOLD new_velocity <=  config.low_threshold
  #define ABOVE_THRESHOLD new_velocity > config.low_threshold
  bool update(KeyConfig& config, Fract16 new_velocity) {
    uint16_t timeNow = MatrixOS::SYS::Millis();

    switch (state)
    {
      case INVAILD:
      case RELEASED:
        state = IDLE;
        // MatrixOS::Logging::LogVerbose("KeyInfo", "RELEASED -> IDLE");
        lastEventTime = timeNow;
        cleared = false;
        hold = false;
        [[fallthrough]];
      case IDLE:
        if (new_velocity > config.activation_threshold)
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
            velocity = config.apply_curve ? applyVelocityCurve(config, new_velocity) : new_velocity;
            return true & !cleared;
          }
        }
        break;
      case DEBUNCING:
        if (new_velocity <= config.activation_threshold)
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
          velocity = config.apply_curve ? applyVelocityCurve(config, new_velocity) : new_velocity;
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
        new_velocity = config.apply_curve ? applyVelocityCurve(config, new_velocity) : new_velocity;

        if (timeNow - lastEventTime > hold_threshold && !hold)
        {
          state = HOLD;
          // MatrixOS::Logging::LogVerbose("KeyInfo", "ACTIVATED -> HOLD");
          velocity = new_velocity;
          hold = true;
          return true & !cleared;
        }
        else if(DIFFERENCE((uint16_t)new_velocity, (uint16_t)velocity) > KEY_INFO_THRESHOLD || ((new_velocity != velocity) && (uint16_t)new_velocity == UINT16_MAX))
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

  void Clear() {
    if (state == PRESSED || state == ACTIVATED || state == HOLD || state == AFTERTOUCH)
    { cleared = true; }
  }
};

struct KeyEvent {
  uint16_t id;
  KeyInfo info;
};