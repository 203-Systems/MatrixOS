#pragma once

#include <cstring>
#include <type_traits>

#include "InputTypes.h"

struct InputEvent {
  InputId id;
  InputClass inputClass;
  union {
    KeypadInfo keypad;
    FaderInfo fader;
    EncoderInfo encoder;
    TouchPointInfo touchPoint;
    GyroInfo gyro;
    AccelerometerInfo accelerometer;
    TemperatureInfo temperature;
    BatteryInfo battery;
    GenericInfo generic;
  };

  InputEvent() { memset(this, 0, sizeof(*this)); }
};

struct InputSnapshot {
  InputId id;
  InputClass inputClass;
  union {
    KeypadInfo keypad;
    FaderInfo fader;
    EncoderInfo encoder;
    TouchPointInfo touchPoint;
    GyroInfo gyro;
    AccelerometerInfo accelerometer;
    TemperatureInfo temperature;
    BatteryInfo battery;
    GenericInfo generic;
  };

  InputSnapshot() { memset(this, 0, sizeof(*this)); }
};

static_assert(std::is_trivially_copyable_v<InputEvent>, "InputEvent must be trivially copyable");
static_assert(std::is_trivially_copyable_v<InputSnapshot>, "InputSnapshot must be trivially copyable");
