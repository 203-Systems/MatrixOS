#include "system/MatrixOS.h"
#include "framework/Timer.h"

class Application
{
    public: 
        char name[64];
        char author[64];
        uint32_t version;

        int8_t status = 0;
        
        inline Application()
        {
            // setup();
            Timer ledTimer;
            Timer keypadTimer;
            while(status != 0)
            {
                // if(ledTimer.Tick(1000 / LED_FPS))
                // {
                // MatrixOS::LED::Render();
                // }
                // if(keypadTimer.Tick(1000 / KEYPAD_POLLRATE))
                // {
                //     MatrixOS::KEYPAD::Scan();
                // }
                // // TODO: USB Scan and call MidiEvent
                main();
            }
        }

        virtual void setup();
        virtual void main();

        virtual void KeyEvent(MatrixOS::KEYPAD::KeyInfo keyInfo);
        virtual void MidiEvent();

        inline void exit()
        {
            status = -1;
        }
};