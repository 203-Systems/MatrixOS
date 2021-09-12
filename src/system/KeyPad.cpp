#include "MatrixOS.h"
#include <string> 

namespace MatrixOS::KEYPAD
{   
    uint16_t read = 0;
    uint16_t changed = 0;
    uint16_t* changelist;

    void Init()
    {
        SysVar::keypad_millis = 1000/UserVar::keypad_scanrate;
    }

    // void (*handler)(uint16_t) = nullptr;

    // void SetHandler(void (*handler)(uint16_t))
    // {
    //     KEYPAD::handler = handler;
    // }

    uint16_t Scan(void)
    {   
        // USB::CDC::Println("KeyPad Scan");
        free(changelist);
        uint16_t* device_change_list = Device::KeyPad::Scan();
        changed = device_change_list[0];
        if(changed > 0)
        {
            changelist = (uint16_t*)malloc(sizeof(uint16_t) * changed);
            memcpy(changelist, &device_change_list[1], sizeof(uint16_t) * changed);
        }
        read = 0;

        return changed;
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

    uint16_t Available()
    {
        // MatrixOS::USB::CDC::Println("KeyPad Available");
        return changed - read;
    }

    KeyInfo Get()
    {
        if(Available == 0)
            return KeyInfo();
        KeyInfo keyInfo = GetKey(changelist[read]);
        read++;
        return keyInfo;
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