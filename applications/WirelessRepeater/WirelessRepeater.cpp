#include "WirelessRepeater.h"

void WirelessRepeater::MidiEvent(MidiPacket midiPacket)
{
  // MatrixOS::Logging::LogInfo("WirelessRepeater", "Midi Recived %d %d %d", midiPacket.data[0], midiPacket.data[1], midiPacket.data[2]);
  if(midiPacket.port == 3)
  {
    MidiPacket usbPacket = midiPacket;
    usbPacket.port = 1;
    MatrixOS::MIDI::Send(usbPacket);
  }
}