#include "Performance8x8.h"

void Performance::Setup()
{
    //Load variable
    // MatrixOS
}

void Performance::Loop()
{
    if(stfuTimer.Tick(10))
    {
        stfuScan();
    }
}


void Performance::MidiEvent(MidiPacket midiPacket) 
{
    // MatrixOS::Logging::LogDebug("Performance", "Midi Recived %d %d %d", midiPacket.channel(), midiPacket.note(), midiPacket.velocity());
    switch(midiPacket.status)
    {
        case NoteOn:
            NoteHandler(midiPacket.channel(), midiPacket.note(), midiPacket.velocity());
            break;
        case NoteOff:
            NoteHandler(midiPacket.channel(), midiPacket.note(), 0);
            break;
        default:
            break;
    }
}
Point Performance::NoteToXY(uint8_t note)
{
    switch(currentKeymap)
    {
        case 0:
        {
            if (note > 35 && note < 100)
            {   
                uint8_t xy_raw = user1_keymap_optimized[note - 36];
                return Point(xy_raw >> 4, xy_raw & 0x0f);
            }
            else if (note > 99 && note < 108) //Side Light Right Column
            {
                return Point(8, note - 100);
            }
            else if (note > 115 && note < 124) //Side Light Bottom Row
            {
                return Point(note - 116, 8);
            }
            else if (note > 107 && note < 116) //Side Light Left Column
            {
                return Point(-1, note - 108);
            }
            else if (note > 27 && note < 36) //Side Light Top Row
            {   
                return Point(note - 28, -1);
            }
            break;
        }
        case 1:
        {

        }
    }
    return Point::Invalid();
}

void Performance::NoteHandler(uint8_t channel, uint8_t note, uint8_t velocity)
{
    // MatrixOS::Logging::LogDebug("Performance", "Midi Recivied %#02X %#02X %#02X", channel, note, velocity);
    Point xy = NoteToXY(note);

    if(compatibilityMode && channel == 6)
    {
        channel = 1; //So it will use legacy palette
    }

    if(xy && !(velocity == 0 && stfu))
    {
        // MatrixOS::Logging::LogDebug("Performance", "Set LED");
        MatrixOS::LED::SetColor(xy, palette[channel % 2][velocity]);
        MatrixOS::LED::Update();
    }
    // else if(!xy)
    // {
    //     MatrixOS::Logging::LogDebug("Performance", "XY incorrect");
    // }
    // else if((velocity == 0 && stfu))
    // {
    //     MatrixOS::Logging::LogDebug("Performance", "STFU");
    // }
    if(stfu)
    {
        if(velocity == 0)
        {
            stfuMap[note] = stfu;
        }
        else
        {
            stfuMap[note] = -1;
        }
    }
}

void Performance::KeyEvent(uint16_t KeyID, KeyInfo keyInfo)
{
    Point xy = MatrixOS::KEYPAD::ID2XY(KeyID);
    if(xy) //IF XY is vaild, means it's on the main grid
    {
        GridKeyEvent(xy, keyInfo);
    }
    else //XY Not vaild, 
    {
        IDKeyEvent(KeyID, keyInfo);
    }
}

void Performance::GridKeyEvent(Point xy, KeyInfo keyInfo)
{   
    // MatrixOS::Logging::LogDebug("Performance Mode", "KeyEvent %d %d", xy.x, xy.y);
    uint8_t note = 0;
    if(xy.x >= 0 && xy.x < 8 && xy.y >= 0 && xy.y < 8)
    {
        note = keymap[currentKeymap][xy.y][xy.x];
    }
    else if(xy.y == -1 && xy.x >= 0 && xy.x < 8) //TouchBar Top Row
    {
        note = touch_keymap[currentKeymap][0][xy.x];
    }
    else if(xy.x == 8 && xy.y >= 0 && xy.y < 8) //TouchBar Right Column
    {
        note = touch_keymap[currentKeymap][1][xy.y];
    }
    else if(xy.y == 8 && xy.x >= 0 && xy.x < 8) //TouchBar Bottom Row
    {
        note = touch_keymap[currentKeymap][2][xy.x];
    }
    else if(xy.x == -1 && xy.y >= 0 && xy.y < 8) //TouchBar Left Column
    {
        note = touch_keymap[currentKeymap][3][xy.y];
    }
    else
    {
        return; //No suitable keymap
    }

    if(keyInfo.state == PRESSED)
    {
        MatrixOS::MIDI::SendPacket(MidiPacket(0, NoteOn, 0, note, keyInfo.velocity.to7bits()));
    }
    else if(keyInfo.state == AFTERTOUCH)
    {
        MatrixOS::MIDI::SendPacket(MidiPacket(0, AfterTouch, 0, note, keyInfo.velocity.to7bits()));
    } 
    else if(keyInfo.state == RELEASED)
    {
        MatrixOS::MIDI::SendPacket(MidiPacket(0, compatibilityMode ? NoteOn : NoteOff, 0, note, keyInfo.velocity.to7bits()));
    } 
}

void Performance::IDKeyEvent(uint16_t keyID, KeyInfo keyInfo)
{
    // MatrixOS::Logging::LogDebug(name, "Key Event");
    if(keyID == 0 && keyInfo.state == PRESSED)
    {
        ActionMenu();
    }
}

void Performance::stfuScan()
{
    for(uint8_t note = 0; note < 128; note++)
    {
        if(stfuMap[note] > 0)
        {   
            stfuMap[note]--;
        }
        else if(stfuMap[note] == 0)
        {   
            Point xy = NoteToXY(note);
            if(xy)
            {
                MatrixOS::LED::SetColor(xy, 0);
                MatrixOS::LED::Update();
            }
            stfuMap[note] = -1;
        }
    }
}

void Performance::ActionMenu()
{
    MatrixOS::Logging::LogDebug(name, "Enter Action Menu");

    UI actionMenu("Action Menu", Color(0x00FFAA));

    actionMenu.AddUIElement(new UIButton("Brightness", Color(0xFFFFFF), [&]() -> void {MatrixOS::SYS::NextBrightness();}), 4, Point(3, 3), Point(3, 4), Point(4, 3), Point(4, 4));

    //Rotation control and canvas
    actionMenu.AddUIElement(new UIButton("Clear Canvas", Color(0x00FF00), [&]() -> void {MatrixOS::Logging::LogDebug(name, "Clear Canvas");}), 2, Point(3, 2), Point(4, 2));
    actionMenu.AddUIElement(new UIButton("Rotate to this side", Color(0x00FF00), [&]() -> void {MatrixOS::SYS::Rotate(RIGHT);}), 2, Point(5, 3), Point(5, 4));
    actionMenu.AddUIElement(new UIButton("Rotate to this side", Color(0x00FF00), [&]() -> void {MatrixOS::SYS::Rotate(DOWN);}), 2, Point(3, 5), Point(4, 5));
    actionMenu.AddUIElement(new UIButton("Rotate to this side", Color(0x00FF00), [&]() -> void {MatrixOS::SYS::Rotate(LEFT);}), 2, Point(2, 3), Point(2, 4));

    actionMenu.AddUIElement(new UIButton("System Setting", Color(0xFFFFFF), [&]() -> void {MatrixOS::SYS::OpenSetting();}), Point(0, 7));

    actionMenu.AddUIElement(new UIButtonWithColorFunc("Compatibility Mode", [&]() -> Color{return compatibilityMode ? Color(0xFFFFFF) : Color(0x7F7F7F);}, [&]() -> void{compatibilityMode = !compatibilityMode; currentKeymap = compatibilityMode;}), Point(7, 0)); //Current the currentKeymap is directly linked to compatibilityMode. Do we really need > 2 keymap tho?

    actionMenu.AddFuncKeyHold([&]() -> void {Exit();});

    actionMenu.Start();

    MatrixOS::Logging::LogDebug(name, "Exit Action Menu");
    MatrixOS::LED::Update(); //TODO: Give UI a new LED layer
}

// #endif

