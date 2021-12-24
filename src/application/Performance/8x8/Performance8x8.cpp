// #ifdef GRID_8x8

#include "Performance8x8.h"
#include "ActionMenu8x8.h"
#include <functional>
#include <string>

void Performance::Setup()
{
    MatrixOS::LED::StartAutoUpdate();
}


void Performance::MidiEvent(MidiPacket midiPacket) 
{
    // MatrixOS::Logging::LogVerbose("Performance", "Midi Recived %d %d %d", midiPacket.data[0], midiPacket.data[1], midiPacket.data[2]);
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


void Performance::NoteHandler(uint8_t channel, uint8_t note, uint8_t velocity)
{
    switch(currentKeymap)
    {
        case 0:
        {
            if (note > 35 && note < 100)
            {   
                uint8_t xy_raw = user1_keymap_optimized[note - 36];
                Point xy = Point(xy_raw >> 4, xy_raw & 0x0f);
                MatrixOS::LED::SetColor(xy, palette[channel % 2][velocity]);
            }
        }
        case 1:
            break; //TODO
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
    uint8_t note = keymap[currentKeymap][xy.y][xy.x];
    if(keyInfo.state == PRESSED)
    {
        MatrixOS::MIDI::SendPacket(MidiPacket(0, NoteOn, 0, note, keyInfo.velocity.to7bits()));
    }
    else if(keyInfo.state == RELEASED)
    {
        MatrixOS::MIDI::SendPacket(MidiPacket(0, NoteOff, 0, note, keyInfo.velocity.to7bits()));
    } 
}

void Performance::IDKeyEvent(uint16_t keyID, KeyInfo keyInfo)
{
    MatrixOS::Logging::LogDebug(name, "Key Event");
    if(keyID == 0 && keyInfo.state == PRESSED)
    {
        ActionMenu();
    }
}

void Performance::ActionMenu()
{
    MatrixOS::LED::PauseAutoUpdate();
    MatrixOS::Logging::LogDebug(name, "Enter Action Menu");

    UI actionMenu("Action Menu", Color(0x00FFAA));

    actionMenu.AddUIElement(UIElement("Brightness", Color(0xFFFFFF), [&]() -> void {MatrixOS::SYS::NextBrightness();}), 4, Point(3, 3), Point(3, 4), Point(4, 3), Point(4, 4));

    //Rotation control and canvas
    actionMenu.AddUIElement(UIElement("Clear Canvas", Color(0x00FF00), [&]() -> void {MatrixOS::Logging::LogDebug(name, "Clear Canvas");}), 2, Point(3, 2), Point(4, 2));
    actionMenu.AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), [&]() -> void {MatrixOS::SYS::Rotate(RIGHT);}), 2, Point(5, 3), Point(5, 4));
    actionMenu.AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), [&]() -> void {MatrixOS::SYS::Rotate(DOWN);}), 2, Point(3, 5), Point(4, 5));
    actionMenu.AddUIElement(UIElement("Rotate to this side", Color(0x00FF00), [&]() -> void {MatrixOS::SYS::Rotate(LEFT);}), 2, Point(2, 3), Point(2, 4));


    actionMenu.AddUIElement(UIElement("System Setting", Color(0x00FFFF), [&]() -> void {MatrixOS::SYS::OpenSetting();}), Point(0, 7));
    actionMenu.AddUIElement(UIElement("Menu Lock", Color(0x00FFFF), [&]() -> void {}), Point(7, 7));
    
    actionMenu.Start();

    MatrixOS::Logging::LogDebug(name, "Exit Action Menu");
    MatrixOS::LED::StartAutoUpdate();
}

// #endif

