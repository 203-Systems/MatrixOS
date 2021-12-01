#include "TestApp.h"

// void TestApp::Setup()
// {

// }

void TestApp::Loop()
{ 
    LED_task();
}

void TestApp::MidiEvent(MidiPacket midiPacket)
{
  // MatrixOS::Logging::LogInfo("TestApp", "Midi Recived %d %d %d", midiPacket.data[0], midiPacket.data[1], midiPacket.data[2]);
  switch(midiPacket.status)
  {
    case NoteOn:
    {
      // MatrixOS::USB::CDC::Println("Note On Handler");

      // MatrixOS::USB::CDC::Print("Og - Port:");
      // MatrixOS::USB::CDC::Print(std::to_string(midiPacket.port).c_str());
      // MatrixOS::USB::CDC::Print(" ");
      // MatrixOS::USB::CDC::Print(std::to_string(midiPacket.status).c_str());
      // MatrixOS::USB::CDC::Print(" ");
      // MatrixOS::USB::CDC::Print(std::to_string(midiPacket.length).c_str());
      // MatrixOS::USB::CDC::Print(" ");
      // MatrixOS::USB::CDC::Print(std::to_string(midiPacket.data[0]).c_str());
      // MatrixOS::USB::CDC::Print(" ");
      // MatrixOS::USB::CDC::Print(std::to_string(midiPacket.data[1]).c_str());
      // MatrixOS::USB::CDC::Print(" ");
      // MatrixOS::USB::CDC::Println(std::to_string(midiPacket.data[2]).c_str());
      uint8_t channel = midiPacket.data[0];
      uint8_t note = midiPacket.data[1];
      uint8_t velocity = midiPacket.data[2];
      MatrixOS::MIDI::SendPacket(MidiPacket(0, NoteOn, channel, note, velocity));
      // MidiPacket newPacket = MidiPacket(0, NoteOn, channel, note, velocity);

      // MatrixOS::USB::CDC::Print("New - Port:");
      // MatrixOS::USB::CDC::Print(std::to_string(newPacket.port).c_str());
      // MatrixOS::USB::CDC::Print(" ");
      // MatrixOS::USB::CDC::Print(std::to_string(newPacket.status).c_str());
      // MatrixOS::USB::CDC::Print(" ");
      // MatrixOS::USB::CDC::Print(std::to_string(newPacket.length).c_str());
      // MatrixOS::USB::CDC::Print(" ");
      // MatrixOS::USB::CDC::Print(std::to_string(newPacket.data[0]).c_str());
      // MatrixOS::USB::CDC::Print(" ");
      // MatrixOS::USB::CDC::Print(std::to_string(newPacket.data[1]).c_str());
      // MatrixOS::USB::CDC::Print(" ");
      // MatrixOS::USB::CDC::Println(std::to_string(newPacket.data[2]).c_str());

      // MatrixOS::USB::CDC::Println("");

      // MatrixOS::MIDI::SendPacket(midiPacket);
      // MatrixOS::MIDI::SendPacket(newPacket);
      break;
    }
    case NoteOff:
    {
      uint8_t channel = midiPacket.data[0];
      uint8_t note = midiPacket.data[1];
      uint8_t velocity = midiPacket.data[2];
      MatrixOS::MIDI::SendPacket(MidiPacket(0, NoteOff, channel, note, velocity));
      break;
    }
    //   MatrixOS::USB::CDC::Println("Note Off Handler");
    //   MatrixOS::MIDI::SendPacket(midiPacket);
    //   break;
    default:
      break;
  }
}

void TestApp::KeyEvent(uint16_t keyID, KeyInfo keyInfo)
{
//   MatrixOS::Logging::LogInfo(name, "Key Event Handler %d", keyID);
  MatrixOS::MIDI::SendPacket(MidiPacket(0, NoteOn, 0, 127, 127));
}

#define NUMSLEDS 64
void TestApp::LED_task(void)
{
  // MatrixOS::Logging::Debug("TestApp", "LED_Task");
  if (TestApp::timer.Tick(100))
  { 
    MatrixOS::LED::SetColor((1 << 12) + led_id, colorList[colorIndex]);
    led_id ++;
    if(led_id == NUMSLEDS)
    {
      led_id = 0;
      colorIndex ++;
      colorIndex %= 5;
    }
    MatrixOS::LED::Update();
  }
}