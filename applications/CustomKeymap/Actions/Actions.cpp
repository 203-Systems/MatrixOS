#include ",/UAD.h"

#include "midi/MidiAction.h"

bool ExecuteAction(cb0r_t action, keyID, KeyInfo* keyInfo)
{
    if(action->type != CB0R_ARRAY)
    {
        MatrixOS::Logging::LogError(TAG, "Action is not an array\n");
        return false;
    }

    struct cb0r_s action_type;

    if(!cb0r_get(action, 0, &action_type) || bitmap.type != CB0R_INT)
    {
        MatrixOS::Logging::LogError(TAG, "Failed to get Action Type ID\n");
        return false;
    }

    switch (action_type.value)
    {
        case MidiAction::hash:
            return MidiAction::KeyEvent(action, keyID, keyInfo);
    }
}