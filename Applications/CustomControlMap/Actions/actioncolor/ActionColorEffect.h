#include "MatrixOS.h"

namespace ActionColorEffect
{
  const char* TAG = "ActionColorEffect";

  constexpr uint32_t signature = StaticHash("actioncolor");

  static bool KeyEvent(UADRuntime* uadRT, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo) {
    if (keyInfo->state != KeyState::PRESSED && keyInfo->state != KeyState::RELEASED)
    {
      return false;
    }

    cb0r_s cbor_data;
    if (!cb0r_get_check_type(actionData, 1, &cbor_data, CB0R_INT))
    {
      MLOGE(TAG, "Failed to get enabled bitmap");
      return false;
    }
    
    ActionInfo groupActionInfo = *actionInfo;
    groupActionInfo.index = 255;

    uint32_t groupRegister;

    uadRT->GetRegister(&groupActionInfo, &groupRegister);

    groupRegister &= 0x0F;

    int8_t index = UADRuntime::IndexInBitmap(cbor_data.value, groupRegister);

    if (index == -1)
    {
      return false;
    }
    else
    {
      Color color;
      if (!cb0r_get_check_type(actionData, index + 1, &cbor_data, CB0R_INT))
      {
        MLOGE(TAG, "Failed to get color");
        return false;
      }
      color = Color(cbor_data.value);
      if (actionInfo->indexType == ActionIndexType::COORD)
      {
        MatrixOS::LED::SetColor(actionInfo->coord, color, 0);
      }
      else if (actionInfo->indexType == ActionIndexType::ID)
      {
        MatrixOS::LED::SetColor(actionInfo->id, color, 0);
      }
      return true;
    }
  }

  static bool Initialization(UADRuntime* uadRT, ActionInfo* actionInfo, cb0r_t actionData) {
    cb0r_s cbor_data;
    if (!cb0r_get_check_type(actionData, 1, &cbor_data, CB0R_INT))
    {
      MLOGE(TAG, "Failed to get enabled bitmap");
      return false;
    }
    

    if (IsBitSet(cbor_data.value, 0))
    {
      Color color;
      if (!cb0r_get_check_type(actionData, 2, &cbor_data, CB0R_INT))
      {
        MLOGE(TAG, "Failed to get color");
        return false;
      }
      color = Color(cbor_data.value);
      if (actionInfo->indexType == ActionIndexType::COORD)
      {
        MatrixOS::LED::SetColor(actionInfo->coord, color, 0);
      }
      else if (actionInfo->indexType == ActionIndexType::ID)
      {
        MatrixOS::LED::SetColor(actionInfo->id, color, 0);
      }
    }
    return true;
  }
};