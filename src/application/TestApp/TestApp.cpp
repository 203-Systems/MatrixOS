#include "TestApp.h"
// #include <string> 

void TestApp::main()
{
  // MatrixOS::MIDI::SetHandler(NoteOn, note_on_handler);
  // MatrixOS::MIDI::SetHandler(NoteOff, note_off_handler);
  while(true)
  {
      MatrixOS::SYS::SystemTask();
      LED_task();
  }
}

void TestApp::note_on_handler(uint8_t channel, uint8_t note, uint8_t velocity)
{
  MatrixOS::USB::CDC::Println("Note On Handler");
  MatrixOS::MIDI::SendPacket(MidiPacket(0, NoteOn, channel, note, velocity));
}

void TestApp::note_off_handler(uint8_t channel, uint8_t note, uint8_t velocity)
{
  MatrixOS::USB::CDC::Println("Note Off Handler");
  MatrixOS::MIDI::SendPacket(MidiPacket(0, NoteOff, channel, note, velocity));
}


void TestApp::LED_task(void)
{
  if (TestApp::timer.Tick(100))
  { 
    // MatrixOS::USB::CDC::Println("LED Index: ");
    // MatrixOS::USB::CDC::Println(std::to_string(led_id));

    MatrixOS::LED::SetColor((1 << 12) + led_id, colorList[colorIndex]);
    MatrixOS::LED::Update();
    led_id ++;

    if(led_id == Device::numsOfLED)
    {
      led_id = 0;
      colorIndex ++;
      colorIndex %= 5;
    }
  }
}