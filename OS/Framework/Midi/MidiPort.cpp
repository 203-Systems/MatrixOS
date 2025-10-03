#include "MatrixOS.h"
#include "MidiPort.h"

// Define the static member variable
std::map<uint16_t, MidiPort*> MidiPort::midiPortMap;

uint16_t MidiPort::Open(uint16_t id, uint16_t queue_size, uint16_t id_range) {
  if (id == MIDI_PORT_INVALID)  // Check if ID is valid
  { return MIDI_PORT_INVALID; }
  if (this->id != MIDI_PORT_INVALID)  // If already registered, go unregister
  { Close(); }
  for (uint16_t i = 0; i < id_range; i++)  // Request for ID
  {
    if (MidiPort::OpenMidiPort(id + i, this))
    {
      this->id = id + i;
      break;
    }
  }
  if (this->id == MIDI_PORT_INVALID)  // Check if registered
  { return MIDI_PORT_INVALID; }
  midi_queue = xQueueCreate(queue_size, sizeof(MidiPacket));
  return this->id;
}

void MidiPort::Close() {
  if (id != MIDI_PORT_INVALID) {
    MidiPort::CloseMidiPort(id);
    this->id = MIDI_PORT_INVALID;
  }
  if (midi_queue != nullptr) {
    vQueueDelete(midi_queue);
    midi_queue = nullptr;
  }
}

void MidiPort::SetName(string name) {
  this->name = name;
}

bool MidiPort::Get(MidiPacket* midipacket_dest, uint32_t timeout_ms) {
  return xQueueReceive(midi_queue, (void*)midipacket_dest, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

bool MidiPort::Send(MidiPacket midipacket, uint16_t targetPort, uint32_t timeout_ms) {
  midipacket.port = this->id;
  return MidiPort::RouteMidiPacket(midipacket, targetPort, timeout_ms);
}

bool MidiPort::Receive(MidiPacket midipacket, uint32_t timeout_ms) {
  if (midi_queue == nullptr)
    return false;

  if (uxQueueSpacesAvailable(midi_queue) == 0)
  {
    // Drop oldest packet to make room for new one (FIFO overflow behavior)
    MidiPacket discarded;
    xQueueReceive(midi_queue, &discarded, 0);
  }
  xQueueSend(midi_queue, &midipacket, pdMS_TO_TICKS(timeout_ms));
  return true;
}

MidiPort::MidiPort() {
  midi_queue = nullptr;
}

MidiPort::MidiPort(string name, uint16_t id, uint16_t queue_size) {
  this->name = name;
  Open(id, queue_size);
}

MidiPort::MidiPort(string name, EMidiPortID port_class, uint16_t queue_size) {
  this->name = name;
  Open(port_class, queue_size, 0x100);
}

MidiPort::~MidiPort() {
  Close();
}

bool MidiPort::OpenMidiPort(uint16_t port_id, MidiPort* midiPort) {
    if (port_id < 0x100 || midiPort == nullptr)
      return false;

    if (midiPortMap.find(port_id) == midiPortMap.end())
    {
      midiPortMap[port_id] = midiPort;
      return true;
    }
    return false;
  }

void MidiPort::CloseMidiPort(uint16_t port_id) {
  midiPortMap.erase(port_id);
}

bool MidiPort::RouteMidiPacket(MidiPacket midiPacket, uint16_t targetPort, uint16_t timeout_ms) {
    uint16_t sourcePort = midiPacket.port; // Where the packet came from
    
    if (targetPort == MIDI_PORT_EACH_CLASS)
    {
      uint16_t targetClass = MIDI_PORT_USB;
      bool send = false;
      for (std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.begin(); port != midiPortMap.end(); ++port)
      {
        if (port->first >= MIDI_PORT_DEVICE_CUSTOM + 0x100)
        { return send; }
        if (port->first >= targetClass && port->first != sourcePort)
        {
          send |= port->second->Receive(midiPacket, timeout_ms);
          targetClass = (port->first / 0x100 + 1) * 0x100;
        }
      }
      return send;
    }
    else if (targetPort == MIDI_PORT_ALL)
    {
      bool send = false;
      for (std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.begin(); port != midiPortMap.end(); ++port)
      {
        // Don't send back to source port
        if (port->first != sourcePort)
        {
          send |= port->second->Receive(midiPacket, timeout_ms);
        }
      }
      return send;
    }
    else
    {
      std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.find(targetPort);
      if (port != midiPortMap.end())
      { 
        // Don't send back to source port
        if (port->first != sourcePort)
        {
          return port->second->Receive(midiPacket, timeout_ms); 
        }
      }
    }
    return false;
}