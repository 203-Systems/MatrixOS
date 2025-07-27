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

    static bool LoadData(cb0r_t actionData, WrapAction* action)
    {
        cb0r_s cbor_data;
        MLOGV(TAG, "Loading data with length %d", actionData->length);
        if (!cb0r_get_check_type(actionData, 1, &cbor_data, CB0R_INT))
        {
            MLOGE(TAG, "Failed to get action data %d", 0);
            return false;
        }
        MLOGV(TAG, "Got data %d", cbor_data.value);
        action->relativeLayer = cbor_data.value & 0x01;
        action->relativePos = (cbor_data.value >> 1) & 0x01;

        if(!cb0r_get(actionData, 2, &cbor_data) || (cbor_data.type != CB0R_INT && cbor_data.type != CB0R_NEG))
        {
            MLOGE(TAG, "Failed to get action data %d", 1);
            return false;
        }

        if (cbor_data.type == CB0R_INT)
        {
            action->layer = cbor_data.value;
        }
        else if(cbor_data.type == CB0R_NEG)
        {
            action->layer = -1 - cbor_data.value;
        }

        if(!cb0r_get(actionData, 3, &cbor_data) || (cbor_data.type != CB0R_INT && cbor_data.type != CB0R_NEG))
        {
            MLOGE(TAG, "Failed to get action data %d", 2);
            return false;
        }
        
        if(cbor_data.type == CB0R_INT)
        {
            action->x = cbor_data.value;
        }
        else if(cbor_data.type == CB0R_NEG)
        {
            action->x = -1 - cbor_data.value;
        }

        if(!cb0r_get(actionData, 4, &cbor_data) || (cbor_data.type != CB0R_INT && cbor_data.type != CB0R_NEG))
        {
            MLOGE(TAG, "Failed to get action data %d", 3);
            return false;
        }
   
        if(cbor_data.type == CB0R_INT)
        {
            action->y = cbor_data.value;
        }
        else if(cbor_data.type == CB0R_NEG)
        {
            action->y = -1 - cbor_data.value;
        }

        return true;
    }

    static bool KeyEvent(UADRuntime* uadRT, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        WrapAction data;
        if(!LoadData(actionData, &data))
        {
            MLOGE(TAG, "Failed to load action");
            return false;
        }
        

        // If index type is via ID. Only different layer of same ID is supported! No relative position!
        if(actionInfo->indexType == ActionIndexType::ID && data.relativePos == true && data.x == 0 && data.y == 0)
        {
            MLOGE(TAG, "Invalid action");
            return false;
        }
        

        ActionInfo newAction = *actionInfo;

        if(data.relativeLayer == true)
        {
            int8_t newLayer = newAction.layer + data.layer;
            if(newLayer < 0 || newLayer >= uadRT->layerCount)
            {
                MLOGE(TAG, "Relative Layer out of range");
                return false;
            }
            newAction.layer += data.layer;
        }
        else if(data.relativeLayer == false)
        {
            newAction.layer = data.layer;
        }

        
        if (data.relativePos == true && data.x == 0 && data.y == 0 && actionInfo->indexType == ActionIndexType::ID)
        {
           // Nothing
        }
        else if(data.relativePos == true)
        {
            newAction.coord = newAction.coord + Point(data.x, data.y);
        }
        else if(!data.relativePos == false)
        {
            newAction.coord = Point(data.x, data.y);
        }

        newAction.depth++;

        UADRuntime::ActionEvent actionEvent = {.type = UADRuntime::ActionEventType::KEYEVENT, .keyInfo = keyInfo};
        uadRT->ExecuteActions(&newAction, &actionEvent);
        uadRT->ExecuteEffects(&newAction, &actionEvent);
        return true;
    }
};