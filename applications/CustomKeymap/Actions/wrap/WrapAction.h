#include "MatrixOS.h"

namespace WrapAction
{
    const char* TAG = "WarpAction";

    constexpr uint32_t signature = StaticHash("wrap");

    struct WrapAction
    {
        bool relativeLayer;
        int8_t layer;
        bool relativePos;
        int8_t x;
        int8_t y;
    };

    static bool LoadAction(cb0r_t actionData, WrapAction* action)
    {
        cb0r_s cbor_data;
        if(!cb0r_get(actionData, 1, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data %d", 0);
            return false;
        }
        action->relativeLayer = cbor_data.value & 0x01;
        action->relativePos = (cbor_data.value >> 1) & 0x01;

        if(!cb0r_get(actionData, 2, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data %d", 1);
            return false;
        }
        action->layer = cbor_data.value;

        if(!cb0r_get(actionData, 3, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data %d", 2);
            return false;
        }
        action->x = cbor_data.value;

        if(!cb0r_get(actionData, 4, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data %d", 3);
            return false;
        }
        action->y = cbor_data.value;
        return true;
    }

    static bool KeyEvent(UAD* UAD, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        WrapAction action;
        if(!LoadAction(actionData, &action))
        {
            MLOGE(TAG, "Failed to load action");
            return false;
        }
        

        // If index type is via ID. Only different layer of same ID is supported! No relative position!
        if(!(actionInfo->indexType == ActionIndexType::ID && action.relativePos == true && action.x == 0 && action.y == 0))
        {
            MLOGE(TAG, "Invalid action");
            return false;
        }

        ActionInfo newAction = *actionInfo;

        if(action.relativeLayer == true)
        {
            int8_t newLayer = newAction.layer + action.layer;
            if(newLayer < 0 || newLayer >= UAD->GetLayerCount())
            {
                MLOGE(TAG, "Relative Layer out of range");
                return false;
            }
            newAction.layer += action.layer;
        }
        else if(action.relativeLayer == false)
        {
            newAction.layer = action.layer;
        }

        
        if (action.relativePos == true && action.x == 0 && action.y == 0 && actionInfo->indexType == ActionIndexType::ID)
        {
           // Nothing
        }
        else if(action.relativePos == true)
        {
            newAction.coord = newAction.coord + Point(action.x, action.y);
        }
        else if(!action.relativePos == false)
        {
            newAction.coord = Point(action.x, action.y);
        }

        return UAD->ExecuteActions(&newAction, keyInfo);
    }
};