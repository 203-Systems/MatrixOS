#include "../UAD.h"

#include "midi/MidiAction.h"

#define TAG "UAD Actions"

bool UAD::ExecuteAction(cb0r_t action, uint16_t keyID, KeyInfo* keyInfo)
{
    switch (action->value)
    {
        case MidiAction::signature:
            return MidiAction::KeyEvent(action, keyID, keyInfo);
    }
    return false;
}