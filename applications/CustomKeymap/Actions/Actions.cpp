#include "../UAD.h"

#include "midi/MidiAction.h"
#include "keyboard/KeyboardAction.h"

#define TAG "UAD Actions"

bool UAD::ExecuteAction(ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo)
{
    switch (actionData->value)
    {
        case MidiAction::signature:
            return MidiAction::KeyEvent(this, actionInfo, actionData, keyInfo);
        case KeyboardAction::signature:
            return KeyboardAction::KeyEvent(this, actionInfo, actionData, keyInfo);
    }
    return false;
}