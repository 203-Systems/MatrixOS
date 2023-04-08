#include <cmath>
#include "UAD.h"

#define TAG "UAD Runtime"

void UAD::KeyEvent(uint16_t keyID, KeyInfo* keyInfo) {
  if (!loaded) { return; }

  ActionInfo actionInfo;
  Point xy = MatrixOS::KEYPAD::ID2XY(keyID);
  if (xy)
  {
    actionInfo.indexType = ActionIndexType::COORD;
    actionInfo.coord = xy;
  }
  else
  {
    return;  // Doesn't not support off grid keys yet
  }

  actionInfo.layer = GetTopLayer();

  ExecuteActions(&actionInfo, keyInfo);
}

bool UAD::ExecuteActions(ActionInfo* actionInfo, KeyInfo* keyInfo) {
  // Get Offset based on index type
  uint16_t offset;
  if (actionInfo->indexType == ActionIndexType::COORD)
  { 
    MLOGV(TAG, "Executing actions for key %d,%d", actionInfo->coord.x, actionInfo->coord.y);
    offset = actionLUT[actionInfo->coord.x][actionInfo->coord.y]; 
  }
  else
  {
    MLOGV(TAG, "Executing actions for key %d", actionInfo->ID);
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
  if (!cb0r_get(&layer_array, 0, &bitmap) || bitmap.type != CB0R_INT)
  {
    MLOGE(TAG, "Failed to get Action Layer Bitmap");
    return false;
  }

  // Execute Actions - Iterate through layers and pass through layers based on configs
  for (int8_t layer = (int8_t)GetTopLayer(); layer >= 0; layer--)
  {
    // If the layer is not enabled, skip it
    if (!IsBitSet(layerEnabled, layer))
    { continue; }

    // Get the index of the layer in the bitmap
    int8_t layer_index = IndexInBitmap(bitmap.value, layer);

    // If the layer has no action.
    if (layer_index == -1)
    {
      if (IsBitSet(layerPassthough, layer))
      {
        // If the layer is set to passthough, Load next layer action
        continue;
      }
      else
      {
        // If the layer is not set to passthough, stop executing actions
        return false;
      }
    }

    // Get the actions array
    cb0r_s actions;
    if(!cb0r_get(&layer_array, layer_index, &actions) || actions.type != CB0R_ARRAY)
    {
        MLOGE(TAG, "Failed to get Action Array");
        return false;
    }

    // Assign the actions's layer
    actionInfo->layer = layer;

    // Execute the actions
    for(uint8_t action_index = 0; action_index < actions.length; action_index++)
    {
        cb0r_s action;
        actionInfo->index = action_index;
        if(!cb0r_get(&actions, action_index, &action) || action.type != CB0R_ARRAY)
        {
            MLOGE(TAG, "Failed to get action %d from action list", action_index);
        }
        ExecuteAction(actionInfo, &action, keyInfo);
    }
  }
  return true;
}

uint8_t UAD::GetTopLayer() {
  return std::log2(layerEnabled);
}