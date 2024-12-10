#include "UAD.h"

#define TAG "UAD Loader"

UADRuntime::UADRuntime() {}

UADRuntime::UADRuntime(uint8_t* uad, size_t size) {
  LoadUAD(uad, size);
}

UADRuntime::~UADRuntime() {
  // Free LUTs
  if (actionLUT != NULL)
  {
    for (uint16_t x = 0; x < mapSize.x; x++)
    {
      free(actionLUT[x]);
    }
    free(actionLUT);
  }

  if (effectLUT != NULL)
  {
    free(effectLUT);
  }
}

bool UADRuntime::CheckVersion(cb0r_t uadMap) {
  cb0r_s uad_section;

  // Get UAD version
  if (!cb0r_find(uadMap, CB0R_UTF8, 11, (uint8_t*)"uad_version", &uad_section) || uad_section.type != CB0R_INT)
  {
    MLOGE(TAG, "UAD version not found");
    return false;
  }
  MLOGV(TAG, "UAD version: %d", uad_section.value);

  if (uad_section.value != UAD_VERSION)
  {
    MLOGE(TAG, "UAD version not supported");
    return false;
  }
  return true;
}

bool UADRuntime::LoadActionList(cb0r_t uadMap) {
  cb0r_s action_list;

  if (!cb0r_find(uadMap, CB0R_UTF8, 11, (uint8_t*)"action_list", &action_list))  // || action_list.type != CB0R_ARRAY)
  {
    MLOGE(TAG, "Action List not found");
    return false;
  }

  return CreateHashList(&action_list, &actionList);
}

bool UADRuntime::LoadEffectList(cb0r_t uadMap) {
  cb0r_s effect_list;

  if (!cb0r_find(uadMap, CB0R_UTF8, 11, (uint8_t*)"effect_list", &effect_list))  // || effect_list.type != CB0R_ARRAY)
  {
    MLOGE(TAG, "Effect List not found");
    return false;
  }

  return CreateHashList(&effect_list, &effectList);
}

bool UADRuntime::CreateHashList(cb0r_t cborArray, vector<uint32_t>* list) {
  list->clear();
  list->reserve(cborArray->length);

  cb0r_s item = *cborArray;
  for (size_t i = 0; i < cborArray->length; i++)
  {
    if (!cb0r_next_check_type(cborArray, &item, &item, CB0R_UTF8))
    {
      MLOGE(TAG, "Failed to create hash list\n");
      return false;
    }
    list->push_back(Hash(string((const char*)(item.start + item.header), item.length)));  // Because array was not null terminated.
  }
  return true;
}

bool UADRuntime::CreateActionLUT(cb0r_t actionMatrix, uint16_t*** lut, Dimension lutSize) {
  cb0r_s x_bitmap;
  if(!cb0r_get_check_type(actionMatrix, 0, &x_bitmap, CB0R_INT))
  {
    MLOGE(TAG, "Failed to get Action X Bitmap\n");
    return false;
  }

  *lut = (uint16_t**)pvPortMalloc(lutSize.x * sizeof(uint16_t*));

  // Layer 1
  cb0r_s y_array = x_bitmap;
  for (uint8_t x = 0; x < lutSize.x; x++)
  {
    (*lut)[x] = (uint16_t*)pvPortMalloc(lutSize.y * sizeof(uint16_t*));

    for (uint8_t y = 0; y < lutSize.y; y++)
    {
      (*lut)[x][y] = 0;
    }

    if(!IsBitSet(x_bitmap.value, x))
    {
      continue;
    }

    if(!cb0r_next_check_type(actionMatrix, &y_array, &y_array, CB0R_ARRAY))
    {
      MLOGE(TAG, "Failed to get Action X Array\n");
      return false;
    }

    cb0r_s y_bitmap;
    if (!cb0r_get_check_type(&y_array, 0, &y_bitmap, CB0R_INT))
    {
      MLOGE(TAG, "Failed to get Action Y Bitmap\n");
      return false;
    }

    // Layer 2
    cb0r_s layer_array = y_bitmap;
    for (uint8_t y = 0; y < lutSize.y; y++)
    {
      if(!IsBitSet(y_bitmap.value, y))
      {
        continue;
      }

      if (!cb0r_next_check_type(&y_array, &layer_array, &layer_array, CB0R_ARRAY))
      {
        MLOGE(TAG, "Failed to get Action Layer Array\n");
        return false;
      }

      uint32_t offset = layer_array.start - uad;
      if (offset > UINT16_MAX)
      {
        MLOGE(TAG, "Action Y Offset is too large\n");
        return false;
      }

      (*lut)[x][y] = (uint16_t)offset;
    }
  }
  return true;
}

bool UADRuntime::CreateEffectLUT(cb0r_t effectMatrix, uint16_t** lut, uint8_t lutSize) {
  cb0r_s layer_bitmap;
  if (!cb0r_get_check_type(effectMatrix, 0, &layer_bitmap, CB0R_INT))
  {
    MLOGE(TAG, "Failed to get Effect Layer Bitmap\n");
    return false;
  }

  *lut = (uint16_t*)pvPortMalloc(lutSize * sizeof(uint16_t));

  // Layer List
  cb0r_s x_array = layer_bitmap;
  for (uint8_t layer = 0; layer < lutSize; layer++)
  {
    if (!IsBitSet(layer_bitmap.value, layer))
    {
      (*lut)[layer] = 0;
      continue;
    }

    if (!cb0r_next_check_type(effectMatrix, &x_array, &x_array, CB0R_ARRAY))
    {
      MLOGE(TAG, "Failed to get Effect X Array\n");
      return false;
    }

    uint32_t offset = x_array.start - uad;
    if (offset > UINT16_MAX)
    {
      MLOGE(TAG, "Effect X Offset is too large\n");
      return false;
    }

    (*lut)[layer] = (uint16_t)offset;
  }
  return true;
}

bool UADRuntime::LoadDevice(cb0r_t uadMap) {

  cb0r_s devices;
  if (!cb0r_find(uadMap, CB0R_UTF8, 7, (uint8_t*)"devices", &devices) || devices.type != CB0R_ARRAY)
  {
    MLOGE(TAG, "Devices not found");
    return false;
  }

  bool device_found = false;
  cb0r_s device;
  cb0r_s device_data;

  // for (size_t i = 0; i < uad_section.length; i++) // We don't support multiple devices yet
  size_t i = 0;  // Device 1
  {

    if (!cb0r_get_check_type(&devices, i, &device, CB0R_MAP))
    {
      MLOGE(TAG, "Failed to get Device");
      return false;
    }

    // // Get Device Name
    // if(!cb0r_find(&device, CB0R_UTF8, 4, (uint8_t*)"name", &device_data) || device_data.type != CB0R_UTF8)
    // {
    //     MLOGE(TAG, "Failed to get Device");
    //     return false;
    // }

    // // Get Device ID
    // if(!cb0r_find(&device, CB0R_UTF8, 2, (uint8_t*)"id", &device_data) || device_data.type != CB0R_ARRAY || device_data.length != 2)
    // {
    //     MLOGE(TAG, "Failed to get Device ID Array");
    //     return false;
    // }
    // printf("\tID: ");

    // for (size_t i = 0; i < device_data.length; i++)
    // {
    //     cb0r_s device_id;
    //     if(!cb0r_get(&device_data, i, &device_id) || device_id.type != CB0R_INT)
    //     {
    //         MLOGE(TAG, "Failed to get Device ID");
    //         return false;
    //     }
    //     printf("%.4X ", device_id.value);
    // }
    // printf("");

    // TODO check if this device is self? - The following code is for example
    // if(loadedDevice != thisDevice) {continue;}

    device_found = true;
  }

  // Get Device Size
  if (!cb0r_find(&device, CB0R_UTF8, 4, (uint8_t*)"size", &device_data) || device_data.type != CB0R_ARRAY || device_data.length != 2)
  {
    MLOGE(TAG, "Failed to get Device Size Array");
    return false;
  }

  cb0r_s device_size = device_data;
  for (size_t i = 0; i < device_data.length; i++)
  {
    if (!cb0r_next_check_type(&device_data, &device_size, &device_size, CB0R_INT))
    {
      MLOGE(TAG, "Failed to get Device Size");
      return false;
    }

    if (i == 0)
    {
      mapSize.x = device_size.value;
    }
    else if (i == 1)
    {
      mapSize.y = device_size.value;
    }
  }

  // Get Layer Count
  if (!cb0r_find(&device, CB0R_UTF8, 6, (uint8_t*)"layers", &device_data) || device_data.type != CB0R_INT)
  {
    MLOGE(TAG, "Failed to get Device Layer Count");
    return false;
  }
  layerCount = device_data.value;

  // Get Device Actions
  if (!cb0r_find(&device, CB0R_UTF8, 7, (uint8_t*)"actions", &device_data) || device_data.type != CB0R_ARRAY)
  {
    MLOGE(TAG, "Failed to get Device Actions");
    return false;
  }
  CreateActionLUT(&device_data, &actionLUT, mapSize);

  // Get Device Effects
  if (!cb0r_find(&device, CB0R_UTF8, 7, (uint8_t*)"effects", &device_data) || device_data.type != CB0R_ARRAY)
  {
    MLOGE(TAG, "Failed to get Device Effects");
    return false;
  }
  CreateEffectLUT(&device_data, &effectLUT, layerCount);
  return device_found;
}

bool UADRuntime::LoadUAD(uint8_t* uad, size_t size) {
  this->uad = uad;
  this->uadSize = size;
  MLOGI(TAG, "Loading UAD");
  cb0r_s uad_map;

  // TODO: Checking without CB0R - CB0R need to return size which will cause it to iterate the entire data. That is not needed and SLOW
  cb0r(uad, uad + size, 0, &uad_map);
  if (uad_map.type != CB0R_MAP)
  {
    MLOGE(TAG, "UAD is not a map");
    return false;
  }

  if (!CheckVersion(&uad_map)) {  // Check if version is supported
    MLOGE(TAG, "UAD version not supported");
    return false;
  }
  
  if (!LoadActionList(&uad_map)) {  // Create a hash list for action names
    MLOGE(TAG, "Failed to load action");
    return false;
  }
  
  if (!LoadEffectList(&uad_map)) { // Create a hash list for effect names
    MLOGE(TAG, "Failed to load effect");
    return false;
  }

  if (!LoadDevice(&uad_map)) { // Load map of current device, including action and effect.
    MLOGE(TAG, "Failed to load device");
    return false;
  }

  loaded = true;

  MLOGI(TAG, "Done parsing UAD");

  InitializeLayer();
  return true;
}

void UADRuntime::UnloadUAD() {
  loaded = false;
}
