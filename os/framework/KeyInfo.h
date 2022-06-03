
// I really don't like how this works atm since it has to go back to System Layer
// Device Keypad Driver -> KeyInfo(Update) -> Apply System configured velocity curve (with user parameter) -> Update
#pragma once

#include <stdint.h>
#include "system/Parameters.h"

#define KEY_INFO_THRESHOLD 512 // 1/127

//Avoid recuesive include
namespace MatrixOS::SYS
{
    uint32_t Millis(void);
    uint32_t GetVariable(string variable, EVarClass varClass);
}

namespace Device::KeyPad
{
    extern bool FSR;
    extern uint16_t low_threshold;
    extern uint16_t high_threshold;
}

enum KeyStates : uint8_t {/*Status Keys*/ IDLE, ACTIVATED, 
                /*Event Keys*/ PRESSED, RELEASED, HOLD, AFTERTOUCH,
                CLEARED = 254u,
                /*Placeholder Keys*/ INVAILD = 255u};

struct KeyInfo {
    KeyInfo() {}
    KeyInfo(Fract16 velocity) {this->velocity = velocity;}

    KeyStates state = IDLE;
    uint32_t lastEventTime = 0; //PRESSED and RELEASED event only
    Fract16 velocity = 0;
    bool hold = false;
    
    // bool hold(uint32_t threshold = hold_threshold)
    // {   
    //     if
    //     return holdTime() > threshold; 
    // }
    
    uint32_t holdTime(void)
    {
        if(state == IDLE)
        return 0;

        if(lastEventTime > MatrixOS::SYS::Millis())
        return 0;

        return MatrixOS::SYS::Millis() - lastEventTime;
    }

    operator bool() { return velocity > 0; }

    /*
    Action Checklist:
    Nothing (All)
    To Long Term State(Pressed, Hold, Release)
    Active (Idle, Release)
    Release(Pressed, Active, Hold, Hold Actived)
    Aftertouch (Pressed, Actived, Hold, Hold Actived)
    */

   #define DIFFERENCE(a,b) ((a)>(b)?(a)-(b):(b)-(a))

    Fract16 applyVelocityCurve(Fract16 velocity)
    {
        uint32_t velocity_sensitive = MatrixOS::SYS::GetVariable("velocity_sensitive", EVarClass::UserVar);
        if(velocity_sensitive)
        {
            if((uint16_t)velocity < velocity_sensitive)
            {
                velocity = 0;
            }
            else if((uint16_t)velocity >= velocity_sensitive)
            {
                velocity = UINT16_MAX;
            }
        }
        else
        {
            if((uint16_t)velocity < Device::KeyPad::low_threshold)
            {
                velocity = 0;
            }
            else if((uint16_t)velocity >= Device::KeyPad::high_threshold)
            {
                velocity = UINT16_MAX;
            }
            else
            {
                velocity = (float)((uint16_t)velocity - Device::KeyPad::low_threshold) / (Device::KeyPad::high_threshold - Device::KeyPad::low_threshold) * UINT16_MAX;
            }
        }
        return velocity;
    }

    bool update(Fract16 velocity, bool applyCurve = true)
    {   
        if(applyCurve && Device::KeyPad::FSR)
        {
            velocity = applyVelocityCurve(velocity);
        }
        //Reset back to normal keys
        if(state == PRESSED)
        {
            state = ACTIVATED;
        }

        if(state == RELEASED)
        {
            hold = false;
            state = IDLE;
        }

        if(state == HOLD || state == AFTERTOUCH)
        {
            state = ACTIVATED;
        }


        if(state == IDLE && velocity > KEY_INFO_THRESHOLD * 3 && MatrixOS::SYS::Millis() - lastEventTime > debounce_threshold)
        {
            state = PRESSED;
            this->velocity = velocity;
            lastEventTime = MatrixOS::SYS::Millis();
            return true;
        }

        if( state == CLEARED && velocity < KEY_INFO_THRESHOLD * 3 && MatrixOS::SYS::Millis() - lastEventTime > debounce_threshold) //May result in key released early
        {
            state = RELEASED;
            this->velocity = 0;
            lastEventTime = MatrixOS::SYS::Millis();
            return false;
        }

        if(state == ACTIVATED&& velocity < KEY_INFO_THRESHOLD * 3 && MatrixOS::SYS::Millis() - lastEventTime > debounce_threshold) //May result in key released early
        {
            state = RELEASED;
            this->velocity = 0;
            lastEventTime = MatrixOS::SYS::Millis();
            return true;
        }

        if(state == ACTIVATED && DIFFERENCE((uint16_t)velocity, (uint16_t)this->velocity) > KEY_INFO_THRESHOLD)
        {
            state = AFTERTOUCH;
            this->velocity = velocity;
            return true;
        }

        if(state == ACTIVATED && !hold)
        {
            if(MatrixOS::SYS::Millis() - lastEventTime > hold_threshold)
            {
                state = HOLD;
                hold = true;
                return true;
            }
        }
        return false;
    }

    void Clear()
    {
        if(state == PRESSED || state == ACTIVATED || state == HOLD || state == AFTERTOUCH)
        {
            state = CLEARED;
        }
    }
};