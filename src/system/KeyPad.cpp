#include "MatrixOS.h"
#include <string> 

namespace MatrixOS::KEYPAD
{
    KeyInfo* changelist[MULTIPRESS];

    void Init()
    {
        
    }

    // void (*handler)(uint16_t) = nullptr;

    // void SetHandler(void (*handler)(uint16_t))
    // {
    //     KEYPAD::handler = handler;
    // }

    uint16_t* Scan(void)
    {   
        // USB::CDC::Println("KeyPad Scan");
        return Device::KeyPad::Scan();
        // uint16_t* changelist = Device::KeyPad::Scan();
        // USB::CDC::Println(std::to_string(changelist[0]).c_str());
        // if(handler)
        // {
        //     for(uint16_t i = 0; i < changelist[0]; i++)
        //     {
        //         uint16_t keyID = changelist[i+1];
        //         handler(keyID);
                // KeyInfo info = Device::KeyPad::GetKey(keyID);
                // if(info.state == PRESSED || info.state == RELEASED)
                // {
                //     USB::CDC::Print("Key Press Detected [");
                //     USB::CDC::Print(std::to_string(keyID).c_str());
                //     USB::CDC::Print("] [");
                //     Point keyXY = Device::KeyPad::ID2XY(keyID);
                //     USB::CDC::Print(std::to_string(keyXY.x).c_str());
                //     USB::CDC::Print(",");
                //     USB::CDC::Print(std::to_string(keyXY.y).c_str());
                //     USB::CDC::Print("] [");
                //     USB::CDC::Print(std::to_string((uint16_t)info.velocity).c_str());
                //     USB::CDC::Print("] [");
                //     switch(info.state)
                //     {
                //         case PRESSED:
                //             USB::CDC::Print("PRESSED"); 
                //             break;
                //         case RELEASED:
                //             USB::CDC::Print("RELEASED"); 
                //             break;
                //     }
                //     USB::CDC::Print("] [");
                //     USB::CDC::Print(std::to_string(info.lastEventTime).c_str());
                //     USB::CDC::Println("]");

                // }
        //     }
        // }
    }

    KeyInfo GetKey(Point keyXY)
    {
        return Device::KeyPad::GetKey(keyXY);
    }

    KeyInfo GetKey(uint16_t keyID)
    {
        return Device::KeyPad::GetKey(keyID);
    }

    uint16_t XY2ID(Point xy) //Not sure if this is required by Matrix OS, added in for now. return UINT16_MAX if no ID is assigned to given XY
    {
        return Device::KeyPad::XY2ID(xy);
    }

    Point ID2XY(uint16_t keyID) //Locate XY for given key ID, return Point(INT16_MIN, INT16_MIN) if no XY found for given ID;
    {
        return Device::KeyPad::ID2XY(keyID);
    }
}