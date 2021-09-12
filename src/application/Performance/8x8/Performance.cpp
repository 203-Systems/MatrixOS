#ifdef GRID_8x8

#include "Performance.h"
#include <functional>
#include <string>

void Perofrmance::setup()
{
    // MatrixOS::MIDI::SetHandler(NoteOn, note_handler);
    // MatrixOS::MIDI::SetHandler(NoteOff, note_handler);
    // MatrixOS::KEYPAD::SetHandler(keyevent_handler);
    currentKeymap = 0;
}

void Performance::loop()
{
    midi_task();
}

void Performance::midi_task()
{
    while(MatrixOS::MIDI::Available())
    {
        MatrixOS::USB::CDC::Println("Midi Recived");
        MidiPacket packet = MatrixOS::MIDI::Get();
        switch(packet.status)
        {
            case NoteOn:
                note_handler(packet.data[0], packet.data[1], packet.data[2]);
                break;
            case NoteOff:
                note_handler(packet.data[0], packet.data[1], 0);
                break;
        }
    }
}

void Performance::note_handler(uint8_t channel, uint8_t note, uint8_t velocity)
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
 
// void Performance::keypad_task()
// {
//     // MatrixOS::USB::CDC::Print(std::to_string(MatrixOS::KEYPAD::GetKey(0).velocity.to7bits()).c_str());
//     // switch(MatrixOS::KEYPAD::GetKey(0).state)
//     // {
//     //     case IDLE:
//     //         MatrixOS::USB::CDC::Println(" IDLE");
//     //         break;
//     //     case ACTIVATED:
//     //         MatrixOS::USB::CDC::Println(" ACTIVATED");
//     //         break;
//     //     case PRESSED:
//     //         MatrixOS::USB::CDC::Println(" PRESSED");
//     //         break;
//     //     case RELEASED:
//     //         MatrixOS::USB::CDC::Println(" RELEASED");
//     //         break;
//     //     case HOLD:
//     //         MatrixOS::USB::CDC::Println(" HOLD");
//     //         break;
//     //     case AFTERTOUCH:
//     //         MatrixOS::USB::CDC::Println(" AFTERTOUCH");
//     //         break;
//     //     case INVAILD:
//     //         MatrixOS::USB::CDC::Println(" INVAILD");
//     //         break;
//     //     default:
//     //         MatrixOS::USB::CDC::Print(" UNKNOWN");
//     //         MatrixOS::USB::CDC::Println(std::to_string(MatrixOS::KEYPAD::GetKey(0).state).c_str());
    
//     //         break;
//     // }
//     for(uint16_t i = 0; i < changelist[0]; i++)
//     {
//     //    MatrixOS::USB::CDC::Println(std::to_string(changelsist[i+1]).c_str());
//        keyevent_handler(changelist[i+1]);
//     }
//     // if(changelist[0]) MatrixOS::USB::CDC::Println("");
// }

void Performance::keyevent_handler(uint16_t keyID)
{
    KeyInfo keyInfo = MatrixOS::KEYPAD::GetKey(keyID);
    Point xy = MatrixOS::KEYPAD::ID2XY(keyID);
    // if(1 || keyInfo.state == PRESSED || keyInfo.state == RELEASED)
    // {
    //     MatrixOS::USB::CDC::Print("Key Press Detected [ID: ");
    //     MatrixOS::USB::CDC::Print(std::to_string(keyID).c_str());
    //     MatrixOS::USB::CDC::Print("] [XY: ");
    //     Point keyXY = Device::KeyPad::ID2XY(keyID);
    //     MatrixOS::USB::CDC::Print(std::to_string(keyXY.x).c_str());
    //     MatrixOS::USB::CDC::Print(",");
    //     MatrixOS::USB::CDC::Print(std::to_string(keyXY.y).c_str());
    //     MatrixOS::USB::CDC::Print("] [Velocity");
    //     MatrixOS::USB::CDC::Print(std::to_string((uint16_t)keyInfo.velocity.to7bits()).c_str());
    //     MatrixOS::USB::CDC::Print("] [State:");
    //     // MatrixOS::USB::CDC::Print(std::to_string(keyInfo.state).c_str());
    //     switch(keyInfo.state)
    //     {
    //         case PRESSED:
    //             MatrixOS::USB::CDC::Print("PRESSED"); 
    //             break;
    //         case RELEASED:
    //             MatrixOS::USB::CDC::Print("RELEASED"); 
    //             break;
    //     }
    //     MatrixOS::USB::CDC::Print("] [EventTime:");
    //     MatrixOS::USB::CDC::Print(std::to_string(keyInfo.lastEventTime).c_str());
    //     MatrixOS::USB::CDC::Println("]");

    // }
    if(xy) //IF XY is vaild, means it's on the main grid
    {
        grid_keyevent(xy, keyInfo);
    }
    else //XY Not vaild, 
    {

    }
}

void Performance::grid_keyevent(Point xy, KeyInfo keyInfo)
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

#endif

