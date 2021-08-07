#include "MatrixOS.h"
#include <string> 

namespace MatrixOS::KEYPAD
{
    KeyInfo* changelist[MULTIPRESS];

    void Init()
    {
        
    }

    void (*handler)(KeyInfo) = nullptr;

    void SetHandler(void (*handler)(KeyInfo))
    {
        KEYPAD::handler = handler;
    }

    void Scan(void (*handler)(KeyInfo))
    {
        uint16_t* changelist = Device::KeyPad::Scan();
        // USB::CDC::Print("KeyPad Scan: ");
        // USB::CDC::Println(std::to_string(changelist[0]).c_str());
        for(uint16_t i = 0; i < changelist[0]; i++)
        {
            uint16_t keyID = changelist[i+1];
            KeyInfo info = Device::KeyPad::GetKey(keyID);
            if(info.state == PRESSED || info.state == RELEASED)
            {
                USB::CDC::Print("Key Press Detected [");
                USB::CDC::Print(std::to_string(keyID).c_str());
                USB::CDC::Print("] [");
                Point keyXY = Device::KeyPad::ID2XY(keyID);
                USB::CDC::Print(std::to_string(keyXY.x).c_str());
                USB::CDC::Print(",");
                USB::CDC::Print(std::to_string(keyXY.y).c_str());
                USB::CDC::Print("] [");
                USB::CDC::Print(std::to_string((uint16_t)info.velocity).c_str());
                USB::CDC::Print("] [");
                switch(info.state)
                {
                    case PRESSED:
                        USB::CDC::Print("PRESSED"); 
                        break;
                    case RELEASED:
                        USB::CDC::Print("RELEASED"); 
                        break;
                }
                USB::CDC::Print("] [");
                USB::CDC::Print(std::to_string(info.lastEventTime).c_str());
                USB::CDC::Println("]");

            }
        }
    }
}