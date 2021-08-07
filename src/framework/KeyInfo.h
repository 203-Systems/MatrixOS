
#pragma once

#include <stdint.h>
#include "Parameters.h"
#include "system/Variables.h"

//Avoid recuesive include
namespace MatrixOS::SYS
{
    uint32_t Millis(void);
}

enum KeyStates {/*Status Key*/ IDLE, ACTIVED, 
                 /*Event Keys*/ PRESSED, RELEASED, HOLD, AFTERTOUCH};

struct KeyInfo {
    KeyStates state = IDLE;
    uint32_t lastEventTime = 0; //PRESSED and RELEASED event only
    fract16 velocity = 0;
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
    bool update(fract16 velocity)
    {
        //Reset back to normal keys
        if(state == PRESSED)
        {
            state = ACTIVED;
        }

        if(state == RELEASED)
        {
            state = IDLE;
        }

        if(state == HOLD)
        {
            state = ACTIVED;
        }

        if(state == IDLE && velocity && MatrixOS::SYS::Millis() - lastEventTime > debounce_threshold)
        {
            state = PRESSED;
            velocity = velocity;
            lastEventTime = MatrixOS::SYS::Millis() ;
            return true;
        }

        if(state == ACTIVED && velocity == 0 && MatrixOS::SYS::Millis() - lastEventTime > debounce_threshold) //May result in key released early
        {
            state = RELEASED;
            velocity = 0;
            lastEventTime = MatrixOS::SYS::Millis();
            return true;
        }

        if(state == ACTIVED && !hold)
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