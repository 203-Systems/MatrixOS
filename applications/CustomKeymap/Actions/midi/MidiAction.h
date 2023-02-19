#include "./Action.h"

class MidiAction: public Action
{
    public:
     static string name = "midi";
     static uint32_t hash = StaticHash("midi");

     virtual bool KeyEvent(cb0r_t action, uint16_t keyID, KeyInfo* keyInfo)
     {
        return true;
     }
}