#include "MatrixOS.h"

namespace LayerAction
{

    enum LayerMode
    {
        PERSISTANCE = 0,
        MONENTARY = 1,
    };

    enum LayerType
    {
        ACTIVE = 0,
        PASSTHOUGH = 1,
    };

    enum LayerOption
    {
        TOGGLE = 0,
        ENABLE = 1,
        DISABLE = 2,
    };

    constexpr uint32_t signature = StaticHash("layer");

    static bool KeyEvent(UAD* UAD, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
    {
        if(keyInfo->state != KeyState::PRESSED) return false;

        uint16_t data[2];
        for(uint8_t i = 1; i < actionData->length; i++)
        {
            cb0r_s cbor_data;
            if(!cb0r_get(actionData, 1, &cbor_data) || cbor_data.type != CB0R_INT)
            {
                MLOGE(TAG, "Failed to get action data %d", i - 1);
                return false;
            }
            data[i - 1] = cbor_data.value;
        }

        LayerMode mode = data[0] & 0x0F;
        LayerType type = (data[0] & 0xF0) >> 4;
        LayerOption option = data[0] & 0xF00 >> 8;
        bool relativeLayer = data[0] >> 15;
        int16_t layer = (int16_t)data[1];

        // TODO

        return true;
    }
};