#include "MatrixOS.h"
#include <map>

namespace MatrixOS::MIDI
{
  QueueHandle_t midi_queue;
  std::map<uint16_t, MidiPort*> midiPortMap;

  void Init(void) {
    midi_queue = xQueueCreate(MIDI_QUEUE_SIZE, sizeof(MidiPacket));
  }

  bool Get(MidiPacket* midipacket_dest, uint16_t timeout_ms) {
    return xQueueReceive(midi_queue, midipacket_dest, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
  }

  bool Send(MidiPacket midiPacket) {
    if (midiPacket.port == MIDI_PORT_ALL_CLASS)
    {
      uint16_t targetClass = MIDI_PORT_USB;
      bool send = false;
      for (std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.begin(); port != midiPortMap.end(); ++port)
      {
        if (port->first >= MIDI_PORT_DEVICE_CUSTOM + 0x100)
        { return send; }
        if (port->first >= targetClass)
        {
          send |= port->second->Recive(midiPacket, 0);
          targetClass = (port->first / 0x100 + 1) * 0x100;
        }
      }
    }
    else
    {
      std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.find(midiPacket.port);
      if (port != midiPortMap.end())
      { return port->second->Recive(midiPacket, 0); }
    }
    return false;
  }

  bool RegisterMidiPort(uint16_t port_id, MidiPort* midiPort) {
    if (port_id < 0x100)
      return false;

    if (midiPortMap.find(port_id) == midiPortMap.end())
    {
      midiPortMap[port_id] = midiPort;
      return true;
    }
    return false;
  }

  void UnregisterMidiPort(uint16_t port_id) {
    midiPortMap.erase(port_id);
  }

  bool Recive(MidiPacket midipacket, uint32_t timeout_ms) {
    if (uxQueueSpacesAvailable(midi_queue) == 0)
    {
      // TODO: Drop first element
    }
    xQueueSend(midi_queue, &midipacket, pdMS_TO_TICKS(timeout_ms));
    return uxQueueSpacesAvailable(midi_queue) == 0;
  }
}