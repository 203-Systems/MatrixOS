#import "../system/MatrixOS.h"
#import "../framework/Timer.h"

class Application
{
    char name[64];
    char author[64];
    uint32_t version;

    int8_t status = 0;

    inline Application()
    {
        setup();
        Timer ledTimer();
        Timer keypadTimer();
        Timer keypadTimer();
        while(status != 0)
        {
            if(ledTimer.tick(1000 / LED_FPS))
            {
               MatrixOS::LED::Render();
            }
            if(keypadTimer.tick(1000 / KEYPAD_POOLRATE))
            {
                MatrixOS::KEYPAD::Scan(KeyEvent);
            }
            // TODO: USB Scan and call MidiEvent
            loop();
        }
    }

    virtual void setup();
    virtual void loop();

    virtual void KeyEvent(KeyInfo keyInfo);
    virtual void MidiEvent();

    inline void exit()
    {
        status = -1;
    }
}