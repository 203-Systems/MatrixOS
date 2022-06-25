#include "MatrixOS.h"
#include "applications/Application.h"
#include <string> 

namespace MatrixOS::KEYPAD
{   
    uint16_t read = 0;
    uint16_t changed = 0;
    uint16_t* changelist;

    StaticTimer_t keypad_tmdef;
    TimerHandle_t keypad_tm;

    void KeypadTimerHandler()
    {
        Scan();
    }

    void Init()
    {
        keypad_tm = xTimerCreateStatic(NULL, configTICK_RATE_HZ / Device::keypad_scanrate, true, NULL, reinterpret_cast<TimerCallbackFunction_t>(KeypadTimerHandler), &keypad_tmdef);
        xTimerStart(keypad_tm, 0);
    }

    uint16_t Scan(bool force)
    {   
        if(force)
        {
            ClearList();
        }
        else if(Available()) //Not all cache has been read yet
        {
            return Available();
        }

        // MatrixOS::Logging::LogDebug("Keypad", "Scan");
        if (changelist)
        {
            vPortFree(changelist);
            changelist = NULL;
        }
            
        uint16_t* device_change_list = Device::KeyPad::Scan();
        changed = device_change_list[0];
        if(changed > 0)
        {
            // if(active_app)
            // {
            //     for(uint8_t i = 0; i < changed; i++)
            //     {
            //         uint8_t key_id = device_change_list[1 + i];
            //         active_app->KeyEvent(key_id, GetKey(key_id));
            //     }
            //     changed = 0;
            // }
            // else
            // {
                changelist = (uint16_t*)pvPortMalloc(sizeof(uint16_t) * changed);
                memcpy(changelist, &device_change_list[1], sizeof(uint16_t) * changed);
            // }
        }
        read = 0;

        return changed;
    }

    uint16_t Available()
    {
        // MatrixOS::USB::CDC::Println("KeyPad Available");
        return changelist ? (changed - read) : 0; //incase change list is null
    }

    uint16_t Get()
    {
        // Logging::LogDebug("Keypad", "%d", Available());
        if(Available() == 0)
            return 0xFFFF;
        
        if (changelist)
        {
            uint16_t keyID = (changelist[read]);
            read++;
            return keyID;
        }
        return 0xFFFE;
    }

    KeyInfo* GetKey(Point keyXY)
    {
        return GetKey(XY2ID(keyXY));
    }

    KeyInfo* GetKey(uint16_t keyID)
    {
        return Device::KeyPad::GetKey(keyID);
    }

    void Clear()
    {
        Device::KeyPad::Clear();
        ClearList();
    }

    void ClearList()
    {
        read = 0;
        changed = 0;
    }

    uint16_t XY2ID(Point xy) //Not sure if this is required by Matrix OS, added in for now. return UINT16_MAX if no ID is assigned to given XY //TODO Compensate for rotation
    {
        if(!xy)
            return UINT16_MAX;
        xy = xy.Rotate(UserVar::rotation, Point(Device::x_size, Device::y_size));
        return Device::KeyPad::XY2ID(xy);
    }

    Point ID2XY(uint16_t keyID) //Locate XY for given key ID, return Point(INT16_MIN, INT16_MIN) if no XY found for given ID;
    {
        Point point = Device::KeyPad::ID2XY(keyID);
        if(point)
           return point.Rotate(UserVar::rotation, Point(Device::x_size, Device::y_size), true);
        return point;
    }
}