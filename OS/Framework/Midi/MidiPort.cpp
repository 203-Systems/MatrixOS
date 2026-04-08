#include "MatrixOS.h"
#include "MidiPort.h"

// Define the static member variable
std::map<uint16_t, MidiPort*> MidiPort::midiPortMap;

uint16_t MidiPort::Open(uint16_t id, uint16_t queueSize, uint16_t idRange) {
  if (id == MIDI_PORT_INVALID) // Check if ID is valid
  {
    return MIDI_PORT_INVALID;
  }
  if (this->id != MIDI_PORT_INVALID) // If already registered, go unregister
  {
    Close();
  }
  for (uint16_t i = 0; i < idRange; i++) // Request for ID
  {
    if (MidiPort::OpenMidiPort(id + i, this))
    {
      this->id = id + i;
      break;
    }
  }
  if (this->id == MIDI_PORT_INVALID) // Check if registered
  {
    return MIDI_PORT_INVALID;
  }
  midiQueue = xQueueCreate(queueSize, sizeof(MidiPacket));
  return this->id;
}

void MidiPort::Close() {
  if (id != MIDI_PORT_INVALID)
  {
    MidiPort::CloseMidiPort(id);
    this->id = MIDI_PORT_INVALID;
  }
  if (midiQueue != nullptr)
  {
    vQueueDelete(midiQueue);
    midiQueue = nullptr;
  }
}

void MidiPort::SetName(string name) {
  this->name = name;
}

bool MidiPort::Get(MidiPacket* midiPacketDest, uint32_t timeoutMs) {
  return xQueueReceive(midiQueue, (void*)midiPacketDest, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

bool MidiPort::Send(MidiPacket midiPacket, uint16_t targetPort, uint32_t timeoutMs) {
  midiPacket.port = this->id;
  return MidiPort::RouteMidiPacket(midiPacket, targetPort, timeoutMs);
}

bool MidiPort::Receive(MidiPacket midiPacket, uint32_t timeoutMs) {
  if (midiQueue == nullptr)
    return false;

  if (uxQueueSpacesAvailable(midiQueue) == 0)
  {
    // Drop oldest packet to make room for new one (FIFO overflow behavior)
    MidiPacket discarded;
    xQueueReceive(midiQueue, &discarded, 0);
  }
  xQueueSend(midiQueue, &midiPacket, pdMS_TO_TICKS(timeoutMs));
  return true;
}

MidiPort::MidiPort() {
  midiQueue = nullptr;
}

MidiPort::MidiPort(string name, uint16_t id, uint16_t queueSize) {
  this->name = name;
  Open(id, queueSize);
}

MidiPort::MidiPort(string name, EMidiPortID portClass, uint16_t queueSize) {
  this->name = name;
  Open(portClass, queueSize, 0x100);
}

MidiPort::~MidiPort() {
  Close();
}

bool MidiPort::OpenMidiPort(uint16_t portId, MidiPort* midiPort) {
  if (portId < 0x100 || midiPort == nullptr)
    return false;

  if (midiPortMap.find(portId) == midiPortMap.end())
  {
    midiPortMap[portId] = midiPort;
    return true;
  }
  return false;
}

void MidiPort::CloseMidiPort(uint16_t portId) {
  midiPortMap.erase(portId);
}

bool MidiPort::RouteMidiPacket(MidiPacket midiPacket, uint16_t targetPort, uint16_t timeoutMs) {
  uint16_t sourcePort = midiPacket.port; // Where the packet came from

  if (targetPort == MIDI_PORT_EACH_CLASS)
  {
    uint16_t targetClass = MIDI_PORT_USB;
    bool send = false;
    for (std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.begin(); port != midiPortMap.end(); ++port)
    {
      if (port->first >= MIDI_PORT_DEVICE_CUSTOM + 0x100)
      {
        return send;
      }
      if (port->first >= targetClass && port->first != sourcePort)
      {
        send |= port->second->Receive(midiPacket, timeoutMs);
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
        send |= port->second->Receive(midiPacket, timeoutMs);
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
        return port->second->Receive(midiPacket, timeoutMs);
      }
    }
  }
  return false;
}