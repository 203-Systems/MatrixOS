#include "../UAD.h"

#include "midi/MidiAction.h"
#include "keyboard/KeyboardAction.h"

#define TAG "UAD Actions"

bool UAD::ExecuteAction(ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
{
    MLOGV(TAG, "Executing action");
    cb0r_s action_index;

    if(!cb0r_get(actionData, 0, &action_index) || action_index.type != CB0R_INT)
    {
        MLOGE(TAG, "Failed to get action index");
        return false;
    }

    uint32_t action_signature = actionList[action_index.value];
    switch (action_signature)
    {
        case MidiAction::signature:
            return MidiAction::KeyEvent(this, actionInfo, actionData, keyInfo);
        case KeyboardAction::signature:
            return KeyboardAction::KeyEvent(this, actionInfo, actionData, keyInfo);
    }
    return false;
}