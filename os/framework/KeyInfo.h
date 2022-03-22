
#pragma once

#include <stdint.h>
#include "system/Parameters.h"

#define KEY_INFO_THRESHOLD 512 // 1/127

//Avoid recuesive include
namespace MatrixOS::SYS
{
    uint32_t Millis(void);
}

enum KeyStates : uint8_t {/*Status Keys*/ IDLE, ACTIVATED, 
                /*Event Keys*/ PRESSED, RELEASED, HOLD, AFTERTOUCH,
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

    bool update(Fract16 velocity)
    {
        //Reset back to normal keys
        if(state == PRESSED)
        {
            state = ACTIVATED;
        }

        if(state == RELEASED)
        {
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

        if(state == ACTIVATED && velocity < KEY_INFO_THRESHOLD * 3 && MatrixOS::SYS::Millis() - lastEventTime > debounce_threshold) //May result in key released early
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
};