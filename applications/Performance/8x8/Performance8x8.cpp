#include "Performance8x8.h"

void Performance::Setup()
{
    //Load variable
    // MatrixOS
    canvasLedLayer = MatrixOS::LED::CurrentLayer();
    currentKeymap = compatibilityMode;
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
            int8_t x = (note % 10) - 1;
            int8_t y = 8 - ((note / 10));
            return Point(x, y);
        }
    }
    return Point::Invalid();
}

void Performance::NoteHandler(uint8_t channel, uint8_t note, uint8_t velocity)
{
    // MatrixOS::Logging::LogDebug("Performance", "Midi Recivied %#02X %#02X %#02X", channel, note, velocity);
    Point xy = NoteToXY(note);

    if(compatibilityMode)
    {
        channel = 1; //So it will use legacy palette
    }

    if(xy && !(velocity == 0 && stfu))
    {
        // MatrixOS::Logging::LogDebug("Performance", "Set LED");
        MatrixOS::LED::SetColor(xy, palette[channel % 2][velocity], canvasLedLayer);
        MatrixOS::LED::Update(canvasLedLayer);
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

void Performance::KeyEvent(uint16_t KeyID, KeyInfo* keyInfo)
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

void Performance::GridKeyEvent(Point xy, KeyInfo* keyInfo)
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

    if(keyInfo->state == PRESSED)
    {
        MatrixOS::MIDI::SendPacket(MidiPacket(0, NoteOn, 0, note, keyInfo->velocity.to7bits()));
    }
    else if(keyInfo->state == AFTERTOUCH)
    {
        MatrixOS::MIDI::SendPacket(MidiPacket(0, AfterTouch, 0, note, keyInfo->velocity.to7bits()));
    } 
    else if(keyInfo->state == RELEASED)
    {
        MatrixOS::MIDI::SendPacket(MidiPacket(0, compatibilityMode ? NoteOn : NoteOff, 0, note, keyInfo->velocity.to7bits()));
    } 
}

void Performance::IDKeyEvent(uint16_t keyID, KeyInfo* keyInfo)
{
    // MatrixOS::Logging::LogDebug(name, "Key Event");
    if(keyID == 0 && keyInfo->state == (menuLock ? HOLD : PRESSED))
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
                MatrixOS::LED::SetColor(xy, 0, canvasLedLayer);
                MatrixOS::LED::Update(canvasLedLayer);
            }
            stfuMap[note] = -1;
        }
    }
}

void Performance::ActionMenu()
{
    MatrixOS::Logging::LogDebug(name, "Enter Action Menu");

    UI actionMenu("Action Menu", Color(0x00FFAA), true);

    UINotePad* notePad = new UINotePad(Dimension(8, 2), keymap_color[currentKeymap], keymap_channel[currentKeymap], (uint8_t*)note_pad_map[currentKeymap]);

    actionMenu.AddUIComponent(new UIButtonLarge("Brightness", Color(0xFFFFFF), Dimension(2,2), [&]() -> void {MatrixOS::SYS::NextBrightness();}, [&]() -> void {BrightnessControl().Start();}), Point(3, 3));
    
    //Rotation control and canvas
    actionMenu.AddUIComponent(new UIButtonLarge("Clear Canvas", Color(0x00FF00), Dimension(2,1), [&]() -> void {MatrixOS::LED::Fill(0, canvasLedLayer);}), Point(3, 2));
    actionMenu.AddUIComponent(new UIButtonLarge("Rotate to this side", Color(0x00FF00), Dimension(1,2), [&]() -> void {MatrixOS::SYS::Rotate(RIGHT);}), Point(5, 3));
    actionMenu.AddUIComponent(new UIButtonLarge("Rotate to this side", Color(0x00FF00), Dimension(2,1), [&]() -> void {MatrixOS::SYS::Rotate(DOWN);}), Point(3, 5));
    actionMenu.AddUIComponent(new UIButtonLarge("Rotate to this side", Color(0x00FF00), Dimension(1,2), [&]() -> void {MatrixOS::SYS::Rotate(LEFT);}), Point(2, 3));

    actionMenu.AddUIComponent(new UIButton("System Setting", Color(0xFFFFFF), [&]() -> void {MatrixOS::SYS::OpenSetting();}), Point(7, 5));

    actionMenu.AddUIComponent(new UIButtonDimmable("Menu Lock", Color(0xA0FF00), [&]() -> bool{return menuLock;}, [&]() -> void{menuLock = !menuLock;}), Point(0, 5)); //Current the currentKeymap is directly linked to compatibilityMode. Do we really need > 2 keymap tho?
    actionMenu.AddUIComponent(new UIButtonDimmable("Flicker Reduction", Color(0xAAFF00), [&]() -> bool{return stfu;}, [&]() -> void{stfu = bool(!stfu) * STFU_DEFAULT;}), Point(0, 0)); //Current the currentKeymap is directly linked to compatibilityMode. Do we really need > 2 keymap tho?
    
    actionMenu.AddUIComponent(new UIButtonDimmable(
        "Compatibility Mode", 
        Color(0xFFFF00), 
        [&]() -> bool{return compatibilityMode;}, 
        [&]() -> void{
            compatibilityMode = !compatibilityMode; 
            currentKeymap = compatibilityMode; 
            notePad->SetColor(keymap_color[currentKeymap]);
            notePad->SetChannel(keymap_channel[currentKeymap]);
            notePad->SetMap((uint8_t*)note_pad_map[currentKeymap]);
        }), 
        Point(7, 0)); //Current the currentKeymap is directly linked to compatibilityMode. Do we really need > 2 keymap tho?

    actionMenu.AddUIComponent(notePad, Point(0, 6)); 

    actionMenu.AddFuncKeyHold([&]() -> void {Exit();});
    actionMenu.SetLoopFunc([&]() -> void{GetMidi();});

    actionMenu.Start();

    MatrixOS::Logging::LogDebug(name, "Exit Action Menu");
}

// #endif

