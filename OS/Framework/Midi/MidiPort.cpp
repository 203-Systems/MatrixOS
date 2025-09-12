#include "MatrixOS.h"
#include "MidiPort.h"

// Define the static member variable
std::map<uint16_t, MidiPort*> MidiPort::midiPortMap;

bool MidiPort::OpenMidiPort(uint16_t port_id, MidiPort* midiPort) {
    if (port_id < 0x100)
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