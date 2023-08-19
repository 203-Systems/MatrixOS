#include "MatrixOS.h"

namespace WrapAction
{
    constexpr string TAG = "WarpAction";

    constexpr uint32_t signature = StaticHash("wrap");

    static bool KeyEvent(UAD* UAD, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        bool relativeLayer = false;
        int8_t layer = 0;
        bool relativePos = false;
        int8_t x = 0;
        int8_t y = 0;
        
        // for(uint8_t i = 1; i < actionData->length; i++)
        // {
        //     cb0r_s cbor_data;
        //     if(!cb0r_get(actionData, i, &cbor_data) || cbor_data.type != CB0R_INT)
        //     {
        //         MLOGE(TAG, "Failed to get action data %d", i - 1);
        //         return false;
        //     }
        //     data[i - 1] = cbor_data.value;
        // }

        cb0r_s cbor_data;
        if(!cb0r_get(actionData, 1, &cbor_data) || cbor_data.type != CB0R_BOOL)
        {
            MLOGE(TAG, "Failed to get action data %d", 0);
            return false;
        }
        relativeLayer = cbor_data.value;

        if(!cb0r_get(actionData, 2, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data %d", 1);
            return false;
        }
        layer = cbor_data.value;

        if(!cb0r_get(actionData, 3, &cbor_data) || cbor_data.type != CB0R_BOOL)
        {
            MLOGE(TAG, "Failed to get action data %d", 2);
            return false;
        }
        relativePos = cbor_data.value;

        if(!cb0r_get(actionData, 4, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data %d", 3);
            return false;
        }
        x = cbor_data.value;

        if(!cb0r_get(actionData, 5, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data %d", 4);
            return false;
        }
        y = cbor_data.value;

        // TODO Execute in UAD Runtime

        return true;
    }
};