#include "MatrixOS.h"

namespace LayerAction
{
    const char* TAG = "LayerAction";

    constexpr uint32_t signature = StaticHash("layer");

    enum LayerActionMode : uint8_t { PERSISTANCE = 0, MOMENTARY = 1 };

    enum LayerActionType : uint8_t { ACTIVE = 0, PASSTHROUGH = 1 };

    enum LayerActionOption : uint8_t { TOGGLE = 0, ENABLE = 1, DISABLE = 2 };

    struct LayerAction
    {
      LayerActionMode mode;
      LayerActionType type;
      LayerActionOption option;
      bool relative;
      uint8_t layer;
    };

    static bool LoadData(cb0r_t actionData, LayerAction* action)
    {
        cb0r_s cbor_data;
        if(!cb0r_get(actionData, 1, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data 0");
            return false;
        }

        action->mode = (LayerActionMode)(cbor_data.value & 0x0F);
        action->type = (LayerActionType)((cbor_data.value >> 4) & 0x0F);
        action->option = (LayerActionOption)((cbor_data.value >> 8) & 0x0F);
        action->relative = (bool)((cbor_data.value >> 15) & 0x01);

        if(!cb0r_get(actionData, 2, &cbor_data) || cbor_data.type != CB0R_INT)
        {
            MLOGE(TAG, "Failed to get action data 1");
            return false;
        }
        action->layer = cbor_data.value;
        return true;
    }
    

    static bool KeyEvent(UAD* UAD, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        if(keyInfo->state != KeyState::PRESSED && keyInfo->state != KeyState::RELEASED) return false;

        struct LayerAction data;
        if(!LoadData(actionData, &data))
        {
            MLOGE(TAG, "Failed to load action");
            return false;
        }

        // Process Layer Action
        int8_t targetLayer = data.layer;
        if(data.relative)
        {
          targetLayer = actionInfo->layer + data.layer;
        }

        if(targetLayer < 0 || targetLayer >= UAD->layerCount)
        {
          MLOGE(TAG, "Invalid target layer");
          return false;
        }

        UAD::LayerInfoType targetLayerInfo;
        if(data.type == LayerActionType::ACTIVE)
        {
          targetLayerInfo = UAD::LayerInfoType::ACTIVE;
        }
        else if(data.type == LayerActionType::PASSTHROUGH)
        {
          targetLayerInfo = UAD::LayerInfoType::PASSTHROUGH;
        }
        else
        {
          MLOGE(TAG, "Invalid type");
          return false;
        }

        bool targetLayerState;
        if(data.option == LayerActionOption::ENABLE)
        {
          targetLayerState = true;
        }
        else if(data.option == LayerActionOption::DISABLE)
        {
          targetLayerState = false;
        }
        else if(data.option == LayerActionOption::TOGGLE)
        {
          targetLayerState = !UAD->GetLayerState(targetLayer, targetLayerInfo);
          // Save togged state to register
          UAD->SetRegister(actionInfo, targetLayerState);
        }
        else
        {
            MLOGE(TAG, "Invalid option");
            return false;
        }

        // Process Key Event
        if(keyInfo->state == KeyState::PRESSED)
        {
            UAD->SetLayerState(targetLayer, targetLayerInfo, targetLayerState);
            return true;
        }
        else if(data.mode == LayerActionMode::MOMENTARY && keyInfo->state == KeyState::RELEASED)
        {

            // Flip Back!
            if(data.option == LayerActionOption::ENABLE)
            {
                targetLayerState = true;
            }
            else if(data.option == LayerActionOption::DISABLE)
            {
                targetLayerState = false;
            }
            else if(data.option == LayerActionOption::TOGGLE)
            {   
                // Load Toggle State from register
                uint32_t registerValue;
                UAD->GetRegister(actionInfo, &registerValue);
                targetLayerState = !(bool)registerValue;
            }

            UAD->SetLayerState(targetLayer, targetLayerInfo, targetLayerState);
            return true;
        }

        return false;
    }
};