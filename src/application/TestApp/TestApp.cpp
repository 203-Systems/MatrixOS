#include "TestApp.h"
#include "system/MatrixOS.h"

void TestApp::main()
{
    tusb_init();

    while(true)
    {
        tud_task();
        midi_task();
    }
}

//--------------------------------------------------------------------+
// MIDI Task
//--------------------------------------------------------------------+

// Variable that holds the current position in the sequence.
// uint32_t note_pos = 0;

// // Store example melody as an array of note values
// uint8_t note_sequence[] =
// {
//   74,78,81,86,90,93,98,102,57,61,66,69,73,78,81,85,88,92,97,100,97,92,88,85,81,78,
//   74,69,66,62,57,62,66,69,74,78,81,86,90,93,97,102,97,93,90,85,81,78,73,68,64,61,
//   56,61,64,68,74,78,81,86,90,93,98,102
// };

void TestApp::midi_task(void)
{
  // static uint32_t start_ms = 0;

  uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint
  // uint8_t const channel   = 0; // 0 for channel 1
  uint8_t midi_buf[256];

  // The MIDI interface always creates input and output port/jack descriptors
  // regardless of these being used or not. Therefore incoming traffic should be read
  // (possibly just discarded) to avoid the sender blocking in IO
  if (!tud_midi_available())
        return;

  uint32_t count = tud_midi_stream_read(midi_buf, sizeof(midi_buf));
  tud_midi_stream_write(cable_num, (uint8_t *)midi_buf, count);



  // // send note every 1000 ms
  // if (board_millis() - start_ms < 286) return; // not enough time
  // start_ms += 286;

  // // Previous positions in the note sequence.
  // int previous = note_pos - 1;

  // // If we currently are at position 0, set the
  // // previous position to the last note in the sequence.
  // if (previous < 0) previous = sizeof(note_sequence) - 1;

  // // Send Note On for current position at full velocity (127) on channel 1.
  // uint8_t note_on[3] = { 0x90 | channel, note_sequence[note_pos], 127 };
  // tud_midi_stream_write(cable_num, note_on, 3);

  // // Send Note Off for previous note.
  // uint8_t note_off[3] = { 0x80 | channel, note_sequence[previous], 0};
  // tud_midi_stream_write(cable_num, note_off, 3);

  // // Increment position
  // note_pos++;

  // // If we are at the end of the sequence, start over.
  // if (note_pos >= sizeof(note_sequence)) note_pos = 0;
}