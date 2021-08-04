#include "TestApp.h"

  void TestApp::main()
  {
    tusb_init();

    while(true)
    {
        tud_task();
        midi_task();  
        LED_task();
    }
  }

  //--------------------------------------------------------------------+
  // MIDI Task
  //--------------------------------------------------------------------+


void TestApp::midi_task(void)
{
  uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint
  uint8_t midi_buf[256];

  // The MIDI interface always creates input and output port/jack descriptors
  // regardless of these being used or not. Therefore incoming traffic should be read
  // (possibly just discarded) to avoid the sender blocking in IO
  if (!tud_midi_available())
        return;

  uint32_t count = tud_midi_stream_read(midi_buf, sizeof(midi_buf));
  tud_midi_stream_write(cable_num, (uint8_t *)midi_buf, count);
}

void TestApp::LED_task(void)
{
  if (TestApp::timer.Tick(100))
  { 
    MatrixOS::LED::SetColor(led_id, colorList[colorIndex]);
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