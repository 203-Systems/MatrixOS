#include <cmath>
#include "UAD.h"

#define TAG "UAD Runtime"

int8_t UADRuntime::IndexInBitmap(uint64_t bitmap, uint8_t index) {
  if (!IsBitSet(bitmap, index))
  {
    return -1;
  }

  // Find nums of bits set before index - TODO: This can probably be optimized by a lot
  uint8_t count = 0;
  for (uint8_t i = 0; i < index; i++)
  {
    count += IsBitSet(bitmap, i);
  }
  return count + 1;
}

void UADRuntime::KeyEvent(uint16_t keyID, KeyInfo* keyInfo) {
  if (!loaded)
  {
    return;
  }

  ActionInfo actionInfo;
  Point xy = MatrixOS::KEYPAD::ID2XY(keyID);
  if (xy)
  {
    if(xy.x >= mapSize.x || xy.y >= mapSize.y || xy.x < 0 || xy.y < 0)
    {
      return;
    }
    actionInfo.indexType = ActionIndexType::COORD;
    actionInfo.coord = xy;
  }
  else
  {
    return;  // Doesn't not support off grid keys yet
  }

  actionInfo.layer = GetTopLayer();
  actionInfo.depth = 0;

  ActionEvent actionEvent = {.type = ActionEventType::KEYEVENT, .keyInfo = keyInfo};
  ExecuteActions(&actionInfo, &actionEvent);
  ExecuteEffects(&actionInfo, &actionEvent);
}

bool UADRuntime::ExecuteActions(ActionInfo* actionInfo, ActionEvent* actionEvent) {
  // Get Offset based on index type
  uint16_t offset;
  ActionInfo newActionInfo = *actionInfo;
  newActionInfo.actionType = ActionType::ACTION;
  
  if (actionInfo->indexType == ActionIndexType::COORD)
  {
    MLOGV(TAG, "Executing actions for key %d,%d", actionInfo->coord.x, actionInfo->coord.y);
    offset = actionLUT[actionInfo->coord.x][actionInfo->coord.y];
  }
  else
  {
    MLOGV(TAG, "Executing actions for key %d", actionInfo->id);
    return false;  // Doesn't not support off grid keys yet
  }

  // If the offset is 0, there are no actions to execute
  if (offset == 0)
  {
    MLOGV(TAG, "No actions to execute");
    return false;
  }

  // Get Action Layer Array
  cb0r_s layer_array;
  if (!cb0r((uint8_t*)uad + offset, (uint8_t*)uad + uadSize, 0, &layer_array) || layer_array.type != CB0R_ARRAY)
  {
    MLOGE(TAG, "Failed to get Action Layer Array");
    return false;
  }

  // Get Action Layer Bitmap
  cb0r_s bitmap;
  if (!cb0r_get_check_type(&layer_array, 0, &bitmap, CB0R_INT))
  {
    MLOGE(TAG, "Failed to get Action Layer Bitmap");
    return false;
  }

  // Execute Actions - Iterate through layers and pass through layers based on configs
  // TODO: Because the we are reading layers from back of the array to front of the array, we need to iterate from back to front. That means we can't do serial reading. 
  // I will get this fixed. 
  cb0r_s actions = bitmap;
  for (int8_t layer = actionInfo->layer; layer >= 0; layer--)
  {
    // If the layer is not enabled, skip it
    if (!IsBitSet(layerEnabled, layer))
    {
      continue;
    }

    // If the layer has no action.
    if (!IsBitSet(bitmap.value, layer))
    {
      if (IsBitSet(layerPassthrough, layer))
      {
        // If the layer is set to passthrough, Load next layer action
        continue;
      }
      else
      {
        // If the layer is not set to passthrough, stop executing actions
        return true;
      }
    }

    // Get the actions array
    if (!cb0r_next_check_type(&layer_array, &actions, &actions, CB0R_ARRAY))
    {
      MLOGE(TAG, "Failed to get Action Array");
      return false;
    }

    // Reassign the actions's layer to the one that is actually triggered
    newActionInfo.layer = layer;

    // Execute the actions
    cb0r_s actionData = actions;
    for (uint8_t action_index = 0; action_index < actions.length; action_index++)
    {
      newActionInfo.index = action_index;
      if (!cb0r_next_check_type(&actions, &actionData, &actionData, CB0R_ARRAY))
      {
        MLOGE(TAG, "Failed to get action %d from action list", action_index);
      }
      ExecuteAction(&newActionInfo, &actionData, actionEvent);
    }
    break; // Action on top layer executed, stop executing following layers
  }
  return true;
}

bool UADRuntime::ExecuteEffects(ActionInfo* effectInfo, ActionEvent* effectEvent) {
  // Get Offset based on index type
  uint16_t offset = effectLUT[effectInfo->layer];
  ActionInfo newEffectInfo = *effectInfo;
  newEffectInfo.actionType = ActionType::EFFECT;

  if (effectInfo->indexType == ActionIndexType::COORD)
  {
    MLOGV(TAG, "Executing effects for key %d,%d", effectInfo->coord.x, effectInfo->coord.y);
  }
  else
  {
    MLOGV(TAG, "Executing effects for key %d", effectInfo->id);
    return false;  // Doesn't not support off grid keys yet
  }

  // If the offset is 0, there are no effects to execute
  if (offset == 0)
  {
    MLOGV(TAG, "No effects to execute");
    return false;
  }

  // Get X Array
  cb0r_s x_array;
  if (!cb0r((uint8_t*)uad + offset, (uint8_t*)uad + uadSize, 0, &x_array) || x_array.type != CB0R_ARRAY)
  {
    MLOGE(TAG, "Failed to get Effect X Array");
    return false;
  }

  // Get x Layer Bitmap
  cb0r_s x_bitmap;
  if (!cb0r_get_check_type(&x_array, 0, &x_bitmap, CB0R_INT))
  {
    MLOGE(TAG, "Failed to get Effect X Bitmap");
    return false;
  }

  int8_t x_index = IndexInBitmap(x_bitmap.value, effectInfo->coord.x);

  if(x_index == -1)
  {
    MLOGV(TAG, "No effects to execute");
    return false;
  }

  // Get Y Array
  cb0r_s y_array;
  if (!cb0r_get_check_type(&x_array, x_index, &y_array, CB0R_ARRAY))
  {
    MLOGE(TAG, "Failed to get Effect Y Array");
    return false;
  }

  // Get y Layer Bitmap
  cb0r_s y_bitmap;
  if (!cb0r_get_check_type(&y_array, 0, &y_bitmap, CB0R_INT))
  {
    MLOGE(TAG, "Failed to get Effect Y Bitmap");
    return false;
  }

  int8_t y_index = IndexInBitmap(y_bitmap.value, effectInfo->coord.y);

  if(y_index == -1)
  {
    MLOGV(TAG, "No effects to execute");
    return false;
  }

  // Get the effects array
  cb0r_s effects;
  if (!cb0r_get_check_type(&y_array, y_index, &effects, CB0R_ARRAY))
  {
    MLOGE(TAG, "Failed to get Effect Array");
    return false;
  }

  // Execute the effects
  cb0r_s effectData = effects;
  for (uint8_t effect_index = 0; effect_index < effects.length; effect_index++)
  {
    newEffectInfo.index = effect_index;
    if(!cb0r_next_check_type(&effects, &effectData, &effectData, CB0R_ARRAY))
    {
      MLOGE(TAG, "Failed to get effect %d from effect list", effect_index);
    }
    ExecuteAction(&newEffectInfo, &effectData, effectEvent);
  }
  return true;
}

uint8_t UADRuntime::GetTopLayer() {
  return std::log2(layerEnabled);
}

void UADRuntime::InitializeLayer(uint8_t layer) {
  if(layer == 255)
  {
    layer = GetTopLayer();
  }

  MLOGI(TAG, "Initializing layer %d", layer);

  uint16_t offset = effectLUT[layer];

  // Nothing in this layer
  if (offset == 0)
  {
    MLOGI(TAG, "Nothing in the effect layer");
    return;
  }

  ActionEvent actionEvent = {.type = ActionEventType::INITIALIZATION, .data = NULL};

  ActionInfo effectInfo;
  effectInfo.actionType = ActionType::EFFECT;
  effectInfo.indexType = ActionIndexType::COORD;

  // Iterating
  cb0r_s x_array;
  if (!cb0r((uint8_t*)uad + offset, (uint8_t*)uad + uadSize, 0, &x_array) || x_array.type != CB0R_ARRAY)
  {
    MLOGE(TAG, "Failed to get Effect X Array");
    return;
  }

  // Get X Bitmap
  cb0r_s x_bitmap;
  if (!cb0r_get_check_type(&x_array, 0, &x_bitmap, CB0R_INT))
  {
    MLOGE(TAG, "Failed to get Effect X Bitmap");
    return;
  }

  // Get Y Array
  cb0r_s y_array = x_bitmap;
  for (uint8_t x = 0; x < mapSize.x; x++)
  {
    if (!IsBitSet(x_bitmap.value, x))
    {
      continue;
    }

    if (!cb0r_next_check_type(&x_array, &y_array, &y_array, CB0R_ARRAY))
    {
      MLOGE(TAG, "Failed to get Effect Y Array");
      return;
    }

    // Get Y Bitmap
    cb0r_s y_bitmap;
    if (!cb0r_get_check_type(&y_array, 0, &y_bitmap, CB0R_INT))
    {
      MLOGE(TAG, "Failed to get Effect Y Bitmap");
      return;
    }

    // Get Y Array
    cb0r_s effects = y_bitmap;
    for (uint8_t y = 0; y < mapSize.y; y++)
    {
      if (!IsBitSet(y_bitmap.value, y))
      {
        continue;
      }

      if (!cb0r_next_check_type(&y_array, &effects, &effects, CB0R_ARRAY))
      {
        MLOGE(TAG, "Failed to get Effect Effect Array\n");
        return;
      }

      // Execute the effects
      cb0r_s effectData = effects;
      for (uint8_t effect_index = 0; effect_index < effects.length; effect_index++)
      {
        effectInfo.coord = Point(x, y);
        effectInfo.layer = layer;
        effectInfo.index = effect_index;

        if (!cb0r_next_check_type(&effects, &effectData, &effectData, CB0R_ARRAY))
        {
          MLOGE(TAG, "Failed to get effect %d from effect list", effect_index);
          continue;
        }
        ExecuteAction(&effectInfo, &effectData, &actionEvent);
      }
    }
  }
  MLOGI(TAG, "Layer %d initialized", layer);
}

void UADRuntime::DeinitializeLayer(uint8_t layer) {
  if(layer == 255)
  {
    layer = GetTopLayer();
  }

  MLOGI(TAG, "Deinitializing layer %d", layer);

  uint16_t offset = effectLUT[layer];

  // Nothing in this layer
  if (offset == 0)
  {
    MLOGI(TAG, "Nothing in the effect layer");
    return;
  }

  ActionEvent actionEvent = {.type = ActionEventType::DEINITIALIZATION, .data = NULL};

  
  ActionInfo effectInfo;
  effectInfo.actionType = ActionType::EFFECT;
  effectInfo.indexType = ActionIndexType::COORD;

  // Iterating
  cb0r_s x_array;
  if (!cb0r((uint8_t*)uad + offset, (uint8_t*)uad + uadSize, 0, &x_array) || x_array.type != CB0R_ARRAY)
  {
    MLOGE(TAG, "Failed to get Effect X Array");
    return;
  }

  // Get X Bitmap
  cb0r_s x_bitmap;
  if (!cb0r_get_check_type(&x_array, 0, &x_bitmap, CB0R_INT))
  {
    MLOGE(TAG, "Failed to get Effect X Bitmap");
    return;
  }

  // Get Y Array
  cb0r_s y_array = x_bitmap;
  for (uint8_t x = 0; x < mapSize.x; x++)
  {
    if (!IsBitSet(x_bitmap.value, x))
    {
      continue;
    }

    if (!cb0r_next_check_type(&x_array, &y_array, &y_array, CB0R_ARRAY))
    {
      MLOGE(TAG, "Failed to get Effect Y Array");
      return;
    }

    // Get Y Bitmap
    cb0r_s y_bitmap;
    if (!cb0r_get_check_type(&y_array, 0, &y_bitmap, CB0R_INT))
    {
      MLOGE(TAG, "Failed to get Effect Y Bitmap");
      return;
    }

    // Get Y Array
    cb0r_s effects = y_bitmap;
    for (uint8_t y = 0; y < mapSize.y; y++)
    {
      if (!IsBitSet(y_bitmap.value, y))
      {
        continue;
      }

      if (!cb0r_next_check_type(&y_array, &effects, &effects, CB0R_ARRAY))
      {
        MLOGE(TAG, "Failed to get Effect Effect Array\n");
        return;
      }

      // Execute the effects
      cb0r_s effectData = effects;
      for (uint8_t effect_index = 0; effect_index < effects.length; effect_index++)
      {
        effectInfo.coord = Point(x, y);
        effectInfo.layer = layer;
        effectInfo.index = effect_index;

        if (!cb0r_next_check_type(&effects, &effectData, &effectData, CB0R_ARRAY))
        {
          MLOGE(TAG, "Failed to get effect %d from effect list", effect_index);
          continue;
        }
        ExecuteAction(&effectInfo, &effectData, &actionEvent);
      }
    }
  }
  MLOGI(TAG, "Layer %d deinitialized", layer);
}
