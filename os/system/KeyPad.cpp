#include "MatrixOS.h"
#include <string> 

namespace MatrixOS::KEYPAD
{   
    uint16_t read = 0;
    uint16_t changed = 0;
    uint16_t* changelist;
    // uint16_t changelist[MULTIPRESS]; //Multipress limits how many press can happen at very moment. Doesn't affect how many keys can be on at the same time. Even if it excided, it will be reported in the next tick.

    // static timer
    StaticTimer_t keypad_tmdef;
    TimerHandle_t keypad_tm;

    void Init()
    {
        keypad_tm = xTimerCreateStatic(NULL, configTICK_RATE_HZ / Device::keypad_scanrate, true, NULL, reinterpret_cast<TimerCallbackFunction_t>(Scan), &keypad_tmdef);
        xTimerStart(keypad_tm, 0);
    }

    // void (*handler)(uint16_t) = nullptr;

    // void SetHandler(void (*handler)(uint16_t))
    // {
    //     KEYPAD::handler = handler;
    // }

    uint16_t Scan(void)
    {   
        if(Available()) //Not all cache has been read yet
            return Available();
        // USB::CDC::Println("KeyPad Scan");
        if (changelist)
        {
            vPortFree(changelist);
            changelist = NULL;
        }
            
        uint16_t* device_change_list = Device::KeyPad::Scan();
        changed = device_change_list[0];
        if(changed > 0)
        {
            changelist = (uint16_t*)pvPortMalloc(sizeof(uint16_t) * changed);
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

    uint16_t Get()
    {
        Logging::LogDebug("Keypad", "%d", Available());
        if(Available() == 0)
            return 0xFFFF;
        
        if (changelist)
        {
            uint16_t keyID = (changelist[read]);
            read++;
            return keyID;
        }
        return 0xFFFF;
    }

    KeyInfo GetKey(Point keyXY)
    {
        return Device::KeyPad::GetKey(keyXY);
    }

    KeyInfo GetKey(uint16_t keyID)
    {
        return Device::KeyPad::GetKey(keyID);
    }

    void Clear()
    {
        read = 0;
        changed = 0;
    }

    uint16_t XY2ID(Point xy) //Not sure if this is required by Matrix OS, added in for now. return UINT16_MAX if no ID is assigned to given XY //TODO Compensate for rotation
    {
        return Device::KeyPad::XY2ID(xy);
    }

    Point ID2XY(uint16_t keyID) //Locate XY for given key ID, return Point(INT16_MIN, INT16_MIN) if no XY found for given ID;
    {
        Point point = Device::KeyPad::ID2XY(keyID);
        if(point)
           return point.Rotate((EDirection)SYS::GetVariable("rotation"), Point(Device::x_size, Device::y_size), true);
        return point;
        // return Device::KeyPad::ID2XY(keyID).Rotate((EDirection)SYS::GetVariable("rotation"), Point(Device::x_size, Device::y_size), true);
    }
}