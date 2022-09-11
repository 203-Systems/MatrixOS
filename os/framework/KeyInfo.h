
// I really don't like how this works atm since it has to go back to System Layer
// Device Keypad Driver -> KeyInfo(Update) -> Apply System configured velocity curve (with user parameter) -> Update
#pragma once

#include <stdint.h>
#include "system/Parameters.h"
#include "SavedVariable.h"

#define KEY_INFO_THRESHOLD 512 // 1/127 - Key Velocity has to move beyond this range in order for after touch to be triggered

//Avoid recuesive include
namespace MatrixOS::SYS
{
    uint32_t Millis(void);
}

namespace MatrixOS::UserVar
{
    extern SavedVariable<bool> velocity_sensitive;
}

namespace MatrixOS::Logging
{
    void LogError (string tag, string format, ...);
    void LogWarning (string tag, string format, ...);
    void LogInfo (string tag, string format, ...);
    void LogDebug (string tag, string format, ...);
    void LogVerbose (string tag, string format, ...);
}

namespace Device::KeyPad
{
    extern bool FSR;
    extern Fract16 low_threshold;
    extern Fract16 high_threshold;
}

struct KeyConfig {
    bool FSR;
    Fract16 low_threshold;
    Fract16 high_threshold;
};

enum KeyStates : uint8_t {/*Status Keys*/ IDLE, ACTIVATED, 
                /*Event Keys*/ PRESSED, RELEASED, HOLD, AFTERTOUCH,
                /*Special*/ DEBUNCING = 253u, CLEARED = 254u,
                /*Placeholder Keys*/ INVAILD = 255u};
// When adding new state, remember to update active() as well


struct KeyInfo {
    KeyConfig* config = nullptr;
    KeyStates state = IDLE;
    uint32_t lastEventTime = 0; //PRESSED and RELEASED event only
    Fract16 velocity = 0;
    bool hold = false;

    KeyInfo(){}

    KeyInfo(KeyConfig* config) {
        this->config = config;
    }

    void setConfig(KeyConfig* config)
    {
        this->config = config;
    }

    uint32_t holdTime(void)
    {
        if(state == IDLE)
        return 0;

        if(lastEventTime > MatrixOS::SYS::Millis())
        return 0;

        return MatrixOS::SYS::Millis() - lastEventTime;
    }

    bool active() {return state >= ACTIVATED && state <= AFTERTOUCH;} 

    operator bool() {return active();}

    /*
    Action Checklist:
    Nothing (All)
    To Long Term State(Pressed, Hold, Release)
    Active (Idle, Release)
    Release(Pressed, Active, Hold, Hold Actived)
    Aftertouch (Pressed, Actived, Hold, Hold Actived)
    */

    #define DIFFERENCE(a,b) ((a)>(b)?(a)-(b):(b)-(a))

    inline uint16_t MAX(uint16_t a, uint16_t b) { return((a) > (b) ? a : b); }

    Fract16 applyVelocityCurve(Fract16 velocity)
    {
        // Fract16 source = velocity;
        if(MatrixOS::UserVar::velocity_sensitive.Get()) //FSR disabled
        {   
            uint16_t threshold = (uint16_t)config->low_threshold;
            if(velocity < threshold)
            {
                velocity = 0;
            }
            else if(velocity >= threshold)
            {
                velocity = UINT16_MAX;
            }
        }
        else
        {
            if(velocity < config->low_threshold)
            {
                velocity = 0;
            }
            else if(velocity >= config->high_threshold)
            {
                velocity = UINT16_MAX;
                // MatrixOS::Logging::LogDebug("Velocity Curve", "%d - %d", source, velocity);
            }
            else
            {
                velocity = ((uint16_t)velocity - (uint16_t)config->low_threshold) * UINT16_MAX / ((uint16_t)config->high_threshold - (uint16_t)config->low_threshold);
                // MatrixOS::Logging::LogDebug("Velocity Curve", "%d - %d", source, velocity);
            }
        }
        return velocity;
    }

    bool update(Fract16 velocity, bool applyCurve = true)
    {   
        if(config == nullptr)
        if(applyCurve && config->FSR)
        {
            velocity = applyVelocityCurve(velocity);
        }

        //Reset back to normal keys
        if(state == PRESSED)
        {
            state = ACTIVATED;
        }

        //Check aftertouch before previous velocity get overwritten
        if(state == ACTIVATED && DIFFERENCE((uint16_t)velocity, (uint16_t)this->velocity) > KEY_INFO_THRESHOLD)
        {
            state = AFTERTOUCH;
            return true;
        }

        //Update current velocity
        this->velocity = velocity;

        if(state == RELEASED)
        {
            hold = false;
            state = IDLE;
        }

        if(state == HOLD || state == AFTERTOUCH)
        {
            state = ACTIVATED;
        }

        if(state == IDLE && velocity)
        {
            state = DEBUNCING;
            lastEventTime = MatrixOS::SYS::Millis();
            return false;
        }
        
        if(state == DEBUNCING)
        {
            if(velocity == 0)
            {
                state = IDLE;
                lastEventTime = MatrixOS::SYS::Millis();
                return false;
            }
            else if(MatrixOS::SYS::Millis() - lastEventTime > debounce_threshold)
            {
                state = PRESSED;
                lastEventTime = MatrixOS::SYS::Millis();
                return true;
            }
            return false;
        }

        if( state == CLEARED && !velocity && MatrixOS::SYS::Millis() - lastEventTime > debounce_threshold) //May result in key released early
        {
            state = RELEASED;
            lastEventTime = MatrixOS::SYS::Millis();
            return false;
        }

        if(state == ACTIVATED && !velocity && MatrixOS::SYS::Millis() - lastEventTime > debounce_threshold) //May result in key released early
        {
            state = RELEASED;
            lastEventTime = MatrixOS::SYS::Millis();
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