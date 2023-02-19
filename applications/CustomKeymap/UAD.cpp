#include "UAD.h"

#define TAG "UAD"

UAD::UAD(uint8_t* uad, size_t size)
{
    LoadUAD(uad, size);
}

UAD::~UAD()
{
    // Free LUTs
    if(actionLUT != NULL)
    {
        for(uint16_t x = 0; x < mapSize.x; x++)
        {
            free(actionLUT[x]);
        }
    }
    free(actionLUT);

    if(effectLUT != NULL)
    {
        for(uint16_t x = 0; x < mapSize.x; x++)
        {
            free(effectLUT[x]);
        }
    }
    free(effectLUT);
}

void UAD::UpdateEffects()
{

}

void UAD::KeyEvent(uint16_t keyID, KeyInfo* keyInfo)
{
    Point xy = MatrixOS::KEYPAD::ID2XY(keyID);
    if(!xy) { return; } // Doesn't not support off grid keys yet
    else{
        if(!mapSize.Contains(xy)) { return; }  // out of bound
        ExecuteActionsFromOffset(actionLUT[xy.x][xy.y], currentLayer, keyID, keyInfo);
        UpdateEffectsFromOffset(effectLUT[xy.x][xy.y], currentLayer, keyID, keyInfo);
    }
}

bool UAD::ExecuteActionsFromOffset(uint16_t offset, uint8_t layer, uint16_t keyID, KeyInfo* keyInfo)
{
    cb0r_s actions;
    if(!GetActionFromOffset(offset, layer, &actions))
    {   
        // TODO load an default action from this layer.
        return false; 
    }

    for(uint8_t i = 0; i < actions.length; i++)
    {
        cb0r_s action;
        if(!cb0r_get(&actions, i, &action) || action.type != CB0R_ARRAY)
        {
            MatrixOS::Logging::LogError(TAG, "Failed to get action %d from action list", i);
        }
        ExecuteAction(&action, keyID, keyInfo);
    }
    return true;
}

bool UAD::UpdateEffectsFromOffset(uint16_t offset, uint8_t layer, uint16_t keyID, KeyInfo* keyInfo)
{
    // TODO
    return false;
}

uint8_t UAD::IndexInBitmap(uint64_t bitmap, uint8_t index)
{
    if((bitmap & (1 << index)) == 0)
    {
        return 0;
    }

    // Find nums of bits set before index - TODO: This can probably be opttimized by a lot
    uint8_t count = 0;
    for (uint8_t i = 0; i < index; i++)
    {
        if(bitmap & (1 << i))
        {
            count++;
        }
    }
    return count + 1;
}

bool UAD::CheckVersion(cb0r_t uadMap)
{
    struct cb0r_s uad_section;

    // Get UAD version
    if(!cb0r_find(uadMap, CB0R_UTF8, 11, (uint8_t*)"uad_version", &uad_section) || uad_section.type != CB0R_INT)
    {
        MatrixOS::Logging::LogError(TAG, "UAD version not found");
        return false;
    }
    MatrixOS::Logging::LogVerbose(TAG, "UAD version: %d", uad_section.value);

    if(uad_section.value != UAD_VERSION)
    {
        MatrixOS::Logging::LogError(TAG, "UAD version not supported");
        return false;
    }
    return true;
}

bool UAD::LoadActionList(cb0r_t uadMap)
{
    struct cb0r_s action_list;
    
    if(!cb0r_find(uadMap, CB0R_UTF8, 11, (uint8_t*)"action_list", &action_list)) // || action_list.type != CB0R_ARRAY)
    {
        MatrixOS::Logging::LogError(TAG, "Action List not found");
        return false;
    }
    
    return CreateHashList(&action_list, &actionList);
}

bool UAD::LoadEffectList(cb0r_t uadMap)
{
    struct cb0r_s effect_list;
    
    if(!cb0r_find(uadMap, CB0R_UTF8, 11, (uint8_t*)"effect_list", &effect_list)) // || effect_list.type != CB0R_ARRAY)
    {
        MatrixOS::Logging::LogError(TAG, "Effect List not found");
        return false;
    }
    
    return CreateHashList(&effect_list, &effectList);
}

bool UAD::CreateHashList(cb0r_t cborArray, vector<uint32_t>* list)
{
    list->clear();
    list->reserve(cborArray->length);

    for (size_t i = 0; i < cborArray->length; i++)
    {
        struct cb0r_s item;
        if(!cb0r_get(cborArray, i, &item) || item.type != CB0R_UTF8)
        {
            MatrixOS::Logging::LogError(TAG, "Failed to create hash list\n");
            return false;
        }
        list->push_back(Hash(string((const char*)(item.start + item.header), item.length))); //Beecause array was not null terimated.
    }
    return true;
}

bool UAD::CreateLUT(cb0r_t actionMatrix, uint16_t*** lut, Dimension lutSize)
{
    *lut = (uint16_t**)pvPortMalloc(lutSize.x * sizeof(uint16_t*));
    struct cb0r_s bitmap;

    for(uint8_t x = 0; x < lutSize.x; x++)
    {   
        (*lut)[x] = (uint16_t*)pvPortMalloc(lutSize.y * sizeof(uint16_t*));
        
        // Layer 1
        if(!cb0r_get(actionMatrix, 0, &bitmap) || bitmap.type != CB0R_INT)
        {
            MatrixOS::Logging::LogError(TAG, "Failed to get Action X Bitmap\n");
            return false;
        }
        
        uint8_t x_index = IndexInBitmap(bitmap.value, x);
        struct cb0r_s y_array;

        if(!cb0r_get(actionMatrix, x_index, &y_array) || y_array.type != CB0R_ARRAY)
        {
            MatrixOS::Logging::LogError(TAG, "Failed to get Action Y Array\n");
            return false;
        }

        for(uint8_t y = 0; y < lutSize.y; y++)
        {   
            if(!cb0r_get(&y_array, 0, &bitmap) || bitmap.type != CB0R_INT)
            {
                MatrixOS::Logging::LogError(TAG, "Failed to get Action Y Bitmap\n");
                return false;
            }

            uint8_t layer_index = IndexInBitmap(bitmap.value, x);
            struct cb0r_s layer_array;

            if(!cb0r_get(&y_array, layer_index, &layer_array) || layer_array.type != CB0R_ARRAY)
            {
                MatrixOS::Logging::LogError(TAG, "Failed to get Action Layer Array\n");
                return false;
            }

            uint32_t offset = layer_array.start - uad;
            if(offset > UINT16_MAX)
            {
                MatrixOS::Logging::LogError(TAG, "Action Offset is too large\n");
                return false;
            }

            (*lut)[x][y] = (uint16_t)offset;
        }
    }
    return true;
}

bool UAD::LoadDevice(cb0r_t uadMap)
{
    bool device_found = false;
    // for (size_t i = 0; i < uad_section.length; i++) // We don't support multiple devices yet
    size_t i = 0; // Device 1
    {
        struct cb0r_s device;
        
        if(!cb0r_get(uadMap, i, &device) || device.type != CB0R_MAP)
        {
            MatrixOS::Logging::LogError(TAG, "Failed to get Device");
            return false;
        }
        
        struct cb0r_s device_data;

        // // Get Device Name
        // if(!cb0r_find(&device, CB0R_UTF8, 4, (uint8_t*)"name", &device_data) || device_data.type != CB0R_UTF8)
        // {
        //     MatrixOS::Logging::LogError(TAG, "Failed to get Device");
        //     return false;
        // }
        
        // // Get Device ID
        // if(!cb0r_find(&device, CB0R_UTF8, 2, (uint8_t*)"id", &device_data) || device_data.type != CB0R_ARRAY || device_data.length != 2)
        // {
        //     MatrixOS::Logging::LogError(TAG, "Failed to get Device ID Array");
        //     return false;
        // }
        // printf("\tID: ");

        // for (size_t i = 0; i < device_data.length; i++)
        // {
        //     struct cb0r_s device_id;
        //     if(!cb0r_get(&device_data, i, &device_id) || device_id.type != CB0R_INT)
        //     {
        //         MatrixOS::Logging::LogError(TAG, "Failed to get Device ID");
        //         return false;
        //     }
        //     printf("%.4X ", device_id.value);
        // }
        // printf("");

        // TODO check if this device is self? - The following code is for example
        // if(loadedDevice != thisDevice) {continue;}

        device_found = true;

        // Get Device Size
        if(!cb0r_find(&device, CB0R_UTF8, 4, (uint8_t*)"size", &device_data) || device_data.type != CB0R_ARRAY || device_data.length != 2)
        {
            MatrixOS::Logging::LogError(TAG, "Failed to get Device Size Array");
            return false;
        }

        for (size_t i = 0; i < device_data.length; i++)
        {
            struct cb0r_s device_size;
            if(!cb0r_get(&device_data, i, &device_size) || device_size.type != CB0R_INT)
            {
                MatrixOS::Logging::LogError(TAG, "Failed to get Device Size");
                return false;
            }

            if(i == 0)
            {
                mapSize.x = device_size.value;
            }
            else if(i == 1)
            {
                mapSize.y = device_size.value;
            }
        }

        // Get Layer Count
        if(!cb0r_find(&device, CB0R_UTF8, 6, (uint8_t*)"layers", &device_data) || device_data.type != CB0R_INT)
        {
            MatrixOS::Logging::LogError(TAG, "Failed to get Device Layer Count");
            return false;
        }
        layerCount = device_data.value;

        // Get Device Actions
        if(!cb0r_find(&device, CB0R_UTF8, 7, (uint8_t*)"actions", &device_data) || device_data.type != CB0R_ARRAY)
        {
            MatrixOS::Logging::LogError(TAG, "Failed to get Device Actions");
            return false;
        }
        CreateLUT(&device_data, &actionLUT, mapSize);

        // Get Device Effects
        if(!cb0r_find(&device, CB0R_UTF8, 7, (uint8_t*)"effects", &device_data) || device_data.type != CB0R_ARRAY)
        {
            MatrixOS::Logging::LogError(TAG, "Failed to get Device Effects");
            return false;
        }
        CreateLUT(&device_data, &effectLUT, mapSize);
       
    }
    return device_found;
}

bool UAD::LoadUAD(uint8_t* uad, size_t size)
{  
    this->uad = uad;
    this->uadSize = size;
    MatrixOS::Logging::LogVerbose(TAG, "Loading UAD");
    struct cb0r_s uad_map;
    
    // TODO: Checking without CB0R - CB0R need to return size which will cause it to iterate the entire data. That is not needed and SLOW
    cb0r(uad, uad + size, 0, &uad_map);
    if(uad_map.type != CB0R_MAP)
    {
        MatrixOS::Logging::LogError(TAG, "UAD is not a map");
        return false;
    }

    if(!CheckVersion(&uad_map)) { return false;}
    if(!LoadActionList(&uad_map)) { return false;}
    if(!LoadEffectList(&uad_map)) { return false;}
    if(!LoadDevice(&uad_map)) { return false;}

    printf("Done parsing UAD");
    return true;
}

bool UAD::GetActionFromOffset(uint16_t offset, uint16_t layer, cb0r_t result)
{
    if(offset == 0) { return false; }

    struct cb0r_s layer_array;
    cb0r(uad + offset, uad + uadSize, 0, &layer_array);

    if(layer_array.type != CB0R_ARRAY)
    {
        MatrixOS::Logging::LogDebug(TAG, "Failed to get Action Layer Array");
        return false;
    }

    struct cb0r_s bitmap;
    if(!cb0r_get(&layer_array, 0, &bitmap) || bitmap.type != CB0R_INT)
    {
        MatrixOS::Logging::LogDebug(TAG, "Failed to get Action Layer Bitmap");
        return false;
    }
    uint8_t layer_index = IndexInBitmap(bitmap.value, layer);
    if(!cb0r_get(&layer_array, layer_index, result) || layer_array.type != CB0R_ARRAY)
    {
        MatrixOS::Logging::LogDebug(TAG, "Failed to get Action Array");
        return false;
    }

    return true;
}