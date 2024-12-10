#include "MatrixOS.h"

namespace MatrixOS::HID::Gamepad
{
typedef struct {
	// 32 Buttons, 6 Axis, 2 D-Pads
    int16_t	xAxis;
    int16_t	yAxis;
    int16_t	zAxis;
    int16_t	rzAxis;
    int16_t	rxAxis;
    int16_t	ryAxis;

    uint8_t	dPad;

    uint32_t buttons;
} HID_GamepadReport_Data_t;

  extern HID_GamepadReport_Data_t _report;
}

namespace GamepadAction
{
  const char* TAG = "GamepadAction";

  constexpr uint32_t signature = StaticHash("gamepad");

  enum class GamepadActionKeycode {
    GAMEPAD_1 = 0x00,
    GAMEPAD_2 = 0x01,
    GAMEPAD_3 = 0x02,
    GAMEPAD_4 = 0x03,
    GAMEPAD_5 = 0x04,
    GAMEPAD_6 = 0x05,
    GAMEPAD_7 = 0x06,
    GAMEPAD_8 = 0x07,
    GAMEPAD_9 = 0x08,
    GAMEPAD_10 = 0x09,
    GAMEPAD_11 = 0x0A,
    GAMEPAD_12 = 0x0B,
    GAMEPAD_13 = 0x0C,
    GAMEPAD_14 = 0x0D,
    GAMEPAD_15 = 0x0E,
    GAMEPAD_16 = 0x0F,
    GAMEPAD_17 = 0x10,
    GAMEPAD_18 = 0x11,
    GAMEPAD_19 = 0x12,
    GAMEPAD_20 = 0x13,
    GAMEPAD_21 = 0x14,
    GAMEPAD_22 = 0x15,
    GAMEPAD_23 = 0x16,
    GAMEPAD_24 = 0x17,
    GAMEPAD_25 = 0x18,
    GAMEPAD_26 = 0x19,
    GAMEPAD_27 = 0x1A,
    GAMEPAD_28 = 0x1B,
    GAMEPAD_29 = 0x1C,
    GAMEPAD_30 = 0x1D,
    GAMEPAD_31 = 0x1E,
    GAMEPAD_32 = 0x1F,

    GAMEPAD_DPAD = 0x40,

    GAMEPAD_X_AXIS = 0x61,
    GAMEPAD_Y_AXIS = 0x62,
    GAMEPAD_Z_AXIS = 0x63,
    GAMEPAD_RX_AXIS = 0x64,
    GAMEPAD_RY_AXIS = 0x65,
    GAMEPAD_RZ_AXIS = 0x66,
  };

  enum class AnalogSource : uint8_t { Binary = 0, KeyForce = 1, Invalid = 0xFF };

  struct GamepadAction {
    GamepadActionKeycode key;
    AnalogSource source;
    union {
      uint16_t begin;
      GamepadDPadDirection dPad;
    };
    uint16_t end;
  };

  static bool LoadData(cb0r_t actionData, GamepadAction* action) {
    cb0r_s cbor_data;
    if (!cb0r_get_check_type(actionData, 1, &cbor_data, CB0R_INT))
    {
      MLOGE(TAG, "Failed to get key");
      return false;
    }
    action->key = (GamepadActionKeycode)cbor_data.value;
    action->source = AnalogSource::Invalid;

    if (action->key == GamepadActionKeycode::GAMEPAD_DPAD)
    {
      if (!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_INT))
      {
        MLOGE(TAG, "Failed to get D-Pad direction");
        return false;
      }
      action->dPad = (GamepadDPadDirection)cbor_data.value;
    }
    else if (action->key >= GamepadActionKeycode::GAMEPAD_X_AXIS && action->key <= GamepadActionKeycode::GAMEPAD_RZ_AXIS)
    {
      if (!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_INT))
      {
        MLOGE(TAG, "Failed to get analog source");
        return false;
      }
      action->source = (AnalogSource)cbor_data.value;

      if (!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_INT))
      {
        MLOGE(TAG, "Failed to get analog begin");
        return false;
      }
      action->begin = cbor_data.value;

      if (!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_INT))
      {
        MLOGE(TAG, "Failed to get analog end");
        return false;
      }
      action->end = cbor_data.value;
    }
    return true;
  }

  static bool KeyEvent(UADRuntime* uadRT, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo) {
    MLOGV(TAG, "KeyEvent");
    if (keyInfo->state != KeyState::PRESSED && keyInfo->state != KeyState::RELEASED && keyInfo->state != KeyState::AFTERTOUCH)
      return false;

    struct GamepadAction data;

    if (!LoadData(actionData, &data))
    {
      MLOGE(TAG, "Failed to load action");
      return false;
    }

    if(data.source != AnalogSource::KeyForce && keyInfo->state == KeyState::AFTERTOUCH)
    {
      return false;
    }

    if (data.key == GamepadActionKeycode::GAMEPAD_DPAD)
    {
      MLOGE(TAG, "DPad %d", data.dPad);
      if (keyInfo->state == KeyState::PRESSED)
      {
        MatrixOS::HID::Gamepad::DPad(data.dPad);
      }
      else if (keyInfo->state == KeyState::RELEASED && MatrixOS::HID::Gamepad::_report.dPad == data.dPad)  // Only release if it was the last direction
      {
        MatrixOS::HID::Gamepad::DPad(GAMEPAD_DPAD_CENTERED);
      }
      return true;
    }
    else if (data.key >= GamepadActionKeycode::GAMEPAD_X_AXIS && data.key <= GamepadActionKeycode::GAMEPAD_RZ_AXIS)
    {
      int32_t value = 0;
      if (data.source == AnalogSource::Binary)
      {
        if (keyInfo->state == KeyState::PRESSED)
        {
          value = data.end;
        }
        else if (keyInfo->state == KeyState::RELEASED)
        {
          value = data.begin;
        }
      }
      else if (data.source == AnalogSource::KeyForce)
      {
        if (keyInfo->state == RELEASED)
        {
          value = data.begin;
        }
        else if (keyInfo->velocity == FRACT16_MAX)
        {
          value = data.end;
        }
        else
        {
          int32_t range = data.end - data.begin;
          value = data.begin + (((uint16_t)keyInfo->velocity * range) >> 16); 
        }
      }

      value -= 32767;
      MLOGD(TAG, "Analog mapping from %d to %d (%d to %d)", keyInfo->velocity, value, data.begin, data.end);

      switch (data.key)
      {
        case GamepadActionKeycode::GAMEPAD_X_AXIS:
          MatrixOS::HID::Gamepad::XAxis(value);
          break;
        case GamepadActionKeycode::GAMEPAD_Y_AXIS:
          MatrixOS::HID::Gamepad::YAxis(value);
          break;
        case GamepadActionKeycode::GAMEPAD_Z_AXIS:
          MatrixOS::HID::Gamepad::RXAxis(value);
          break;
        case GamepadActionKeycode::GAMEPAD_RX_AXIS:
          MatrixOS::HID::Gamepad::RYAxis(value);
          break;
        case GamepadActionKeycode::GAMEPAD_RY_AXIS:
          MatrixOS::HID::Gamepad::ZAxis(value);
          break;
        case GamepadActionKeycode::GAMEPAD_RZ_AXIS:
          MatrixOS::HID::Gamepad::RZAxis(value);
          break;
        default:
          return false;
        return true;
      }
    }
    else
    {
      uint8_t keycode = (uint8_t)data.key & 0x1F; // We have keys that are not in the range of 0-31 because when we send back to the host we need to be able to tell if it's was as A or as 1
      MLOGD(TAG, "Regular key %d", keycode);
      // if (keyInfo->state == KeyState::PRESSED)
      // {
      //   MatrixOS::HID::Gamepad::Press(keycode);
      // }
      // else if (keyInfo->state == KeyState::RELEASED)
      // {
      //   MatrixOS::HID::Gamepad::Release(keycode);
      // }
      return true;
    }
    return false;
  }
};