#include ",/UAD.h"

class Action
{
    public:
    static string name;
    static uint32_t hash;

    virtual bool KeyEvent(cb0r_t action, uint16_t keyID, KeyInfo* keyInfo);
}