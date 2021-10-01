// #ifdef GRID_8x8

#include "Performance8x8.h"
#include "ActionMenu8x8.h"
#include <functional>
#include <string>

void Performance::Setup()
{
    currentKeymap = 0;
}

void Performance::Loop()
{
    // midi_task();
}

void Performance::MidiEvent(MidiPacket midiPacket) 
{
    switch(midiPacket.status)
    {
        case NoteOn:
        case NoteOff:
            NoteHandler(midiPacket.channel(), midiPacket.note(), midiPacket.velocity());
            break;
        default:
            break;
    }
}


void Performance::NoteHandler(uint8_t channel, uint8_t note, uint8_t velocity)
{
    // MatrixOS::USB::CDC::Print("Note Handler ");
    // MatrixOS::USB::CDC::Print(std::to_string(channel).c_str());
    // MatrixOS::USB::CDC::Print(" ");
    // MatrixOS::USB::CDC::Print(std::to_string(note).c_str());
    // MatrixOS::USB::CDC::Print(" ");
    // MatrixOS::USB::CDC::Println(std::to_string(velocity).c_str());
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
            else
            {

            }
            break;
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
    if(keyID == 0 && keyInfo.state == PRESSED)
    {
        ActionMenu actionMenu;
        actionMenu.Start();
    }
}

// #endif

