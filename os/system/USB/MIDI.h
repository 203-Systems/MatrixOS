#define USB_MIDI_COUNT 1
namespace MatrixOS::USB::MIDI
{
  void Init();
}

void tud_midi_rx_cb(uint8_t itf);