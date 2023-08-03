#include "FreeRTOS.h"
#include "queue.h"

class MidiPort;
namespace MatrixOS::MIDI
{
  bool RegisterMidiPort(uint16_t port_id, MidiPort* midiPort);
  void UnregisterMidiPort(uint16_t port_id);
  bool Recive(MidiPacket midipacket, uint32_t timeout_ms);
}

class MidiPort {
 public:
  string name;
  uint16_t id = MIDI_PORT_INVAILD;
  QueueHandle_t midi_queue;

  uint16_t Register(uint16_t id, uint16_t queue_size = 64, uint16_t id_range = 1) {
    if (id == MIDI_PORT_INVAILD)  // Check if ID is vaild
    { return MIDI_PORT_INVAILD; }
    if (this->id != MIDI_PORT_INVAILD)  // If already registered, go unregister
    { Unregister(); }
    for (uint16_t i = 0; i < id_range; i++)  // Request for ID
    {
      if (MatrixOS::MIDI::RegisterMidiPort(id + i, this))
      {
        this->id = id + i;
        break;
      }
    }
    if (this->id == MIDI_PORT_INVAILD)  // Check if registered
    { return MIDI_PORT_INVAILD; }
    midi_queue = xQueueCreate(queue_size, sizeof(MidiPacket));
    return this->id;
  }

  void Unregister() {
    MatrixOS::MIDI::UnregisterMidiPort(id);
    this->id = MIDI_PORT_INVAILD;
    vQueueDelete(midi_queue);
  }

  void SetName(string name) { this->name = name; }

  bool Get(MidiPacket* midipacket_dest, uint32_t timeout_ms = 0) {
    return xQueueReceive(midi_queue, (void*)midipacket_dest, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
  }

  // This will modify the midipacket to be the same as the midiport
  bool Send(MidiPacket midipacket, uint32_t timeout_ms = 0) {
    midipacket.port = this->id;
    return MatrixOS::MIDI::Recive(midipacket, timeout_ms);
  }

  // This is for Matrix OS kernal to call
  bool Recive(MidiPacket midipacket, uint32_t timeout_ms = 0) {
    if (uxQueueSpacesAvailable(midi_queue) == 0)
    {
      // TODO: Drop first element
      return false;
    }
    xQueueSend(midi_queue, &midipacket, pdMS_TO_TICKS(timeout_ms));
    return true;
  }

  MidiPort() {}
  MidiPort(string name, uint16_t id, uint16_t queue_size = 64) {
    this->name = name;
    Register(id, queue_size);
  }

  MidiPort(string name, EMidiPortID port_class, uint16_t queue_size = 64) {
    this->name = name;
    Register(port_class, queue_size, 0x100);
  }

  ~MidiPort() { Unregister(); }
};
