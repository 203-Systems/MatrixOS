#include "freertos.h"
#include "queue.h"

class MidiPort;
namespace MatrixOS::MIDI
{
  uint16_t OpenMidiPort(uint16_t port_id, MidiPort* midiPort);
  bool CloseMidiPort(uint16_t port_id);
  bool Receive(MidiPacket midipacket, uint32_t timeout_ms);
}

class MidiPort {
 public:
  string name;
  uint16_t id = MIDI_PORT_INVALID;
  QueueHandle_t midi_queue;

  uint16_t Open(uint16_t id, uint16_t queue_size = 64) {
    if (id == MIDI_PORT_INVALID && id < 0x100)  // Check if ID is valid
    { return MIDI_PORT_INVALID; }
    id = MatrixOS::MIDI::OpenMidiPort(id, this);
    if (id == MIDI_PORT_INVALID)  // Check if successful registrated
    { return MIDI_PORT_INVALID; }
    this->id = id;
    midi_queue = xQueueCreate(queue_size, sizeof(MidiPacket));
    return this->id;
  }

  void Close() {
    if (this->id == MIDI_PORT_INVALID)
    { return; }
    MatrixOS::MIDI::CloseMidiPort(id);
    this->id = MIDI_PORT_INVALID;
    vQueueDelete(midi_queue);
  }

  void SetName(string name) { this->name = name; }

  bool Get(MidiPacket* midipacket_dest, uint32_t timeout_ms = 0) {
    if (this->id == MIDI_PORT_INVALID)
    { return false; }
    return false;
    return xQueueReceive(midi_queue, (void*)midipacket_dest, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
  }

  // This will modify the midipacket to be the same as the midiport
  bool Send(MidiPacket midipacket, uint32_t timeout_ms = 0) {
    if (this->id == MIDI_PORT_INVALID)
    { return false; }
    midipacket.port = this->id;
    return MatrixOS::MIDI::Receive(midipacket, timeout_ms);
  }

  // This is for Matrix OS kernal to call
  bool Receive(MidiPacket midipacket, uint32_t timeout_ms = 0) {
    if (uxQueueSpacesAvailable(midi_queue) == 0)
    {
      // TODO: Drop first element
      return false;
    }
    xQueueSend(midi_queue, &midipacket, pdMS_TO_TICKS(timeout_ms));
    return true;
  }

  MidiPort() {}

  // Pass in class + 0 as id will auto assign the id
  MidiPort(string name, uint16_t id, uint16_t queue_size = 64) {
    this->name = name;
    Open(id, queue_size);
  }

  ~MidiPort() { Close(); }
};