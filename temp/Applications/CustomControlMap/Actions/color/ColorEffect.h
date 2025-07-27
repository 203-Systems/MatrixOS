#include "MatrixOS.h"

namespace ColorEffect
{
  const char* TAG = "ColorEffect";

  constexpr uint32_t signature = StaticHash("color");

  struct ColorEffectData {
    bool hasDefault;
    bool hasActivated;
    uint32_t defaultColor;
    uint32_t activatedColor;
  };

  static bool LoadData(cb0r_t actionData, ColorEffectData* data) {
    cb0r_s cbor_data;
    if (!cb0r_get_check_type(actionData, 1, &cbor_data, CB0R_INT))
    {
      return false;
    }
    data->hasDefault = cbor_data.value & 0x01;
    data->hasActivated = (cbor_data.value >> 1) & 0x01;

    if (data->hasDefault == false)
    {
      data->defaultColor = 0x000000;
    }
    else
    {
      if (!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_INT))
      {
        return false;
      }

      data->defaultColor = cbor_data.value;
    }

    if (data->hasActivated == false)
    {
      data->activatedColor = 0xFFFFFF;
      return true;
    }
    else
    {
      if (!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_INT))
      {
        return false;
      }

      data->activatedColor = cbor_data.value;
    }
    return true;
  }

  static bool KeyEvent(UADRuntime* uadRT, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo) {
    if (keyInfo->state != KeyState::PRESSED && keyInfo->state != KeyState::RELEASED)
      return false;

    struct ColorEffectData data;

    if (!LoadData(actionData, &data))
    {
      MLOGE(TAG, "Failed to load data");
      return false;
    }

    // if(data.hasActivated == false)
    // {
    //     return true;
    // }

    if (keyInfo->state == KeyState::PRESSED)
    {
      if (actionInfo->indexType == ActionIndexType::COORD)
      {
        MatrixOS::LED::SetColor(actionInfo->coord, Color(data.activatedColor), 0);
      }
      else if (actionInfo->indexType == ActionIndexType::ID)
      {
        MatrixOS::LED::SetColor(actionInfo->id, Color(data.activatedColor), 0);
      }
      return true;
    }
    else if (keyInfo->state == KeyState::RELEASED)
    {
      if (actionInfo->indexType == ActionIndexType::COORD)
      {
        MatrixOS::LED::SetColor(actionInfo->coord, Color(data.defaultColor), 0);
      }
      else if (actionInfo->indexType == ActionIndexType::ID)
      {
        MatrixOS::LED::SetColor(actionInfo->id, Color(data.defaultColor), 0);
      }
      return true;
    }
    return false;
  }

  static bool Initialization(UADRuntime* uadRT, ActionInfo* actionInfo, cb0r_t actionData) {
    struct ColorEffectData data;

    if (!LoadData(actionData, &data))
    {
      MLOGE(TAG, "Failed to load data");
      return false;
    }

    // if(data.hasDefault)
    // {
    if (actionInfo->indexType == ActionIndexType::COORD)
    {
      MatrixOS::LED::SetColor(actionInfo->coord, Color(data.defaultColor), 0);
    }
    else if (actionInfo->indexType == ActionIndexType::ID)
    {
      MatrixOS::LED::SetColor(actionInfo->id, Color(data.defaultColor), 0);
    }
    // }

    return true;
  }
};