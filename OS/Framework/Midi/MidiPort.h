#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include <map>

class MidiPort {
 private:
  static std::map<uint16_t, MidiPort*> midiPortMap;

 public:
  string name;
  uint16_t id = MIDI_PORT_INVALID;
  QueueHandle_t midi_queue;

  uint16_t Open(uint16_t id, uint16_t queue_size = 64, uint16_t id_range = 1);
  void Close();
  void SetName(string name);
  bool Get(MidiPacket* midipacket_dest, uint32_t timeout_ms = 0);
  bool Send(MidiPacket midipacket, uint16_t targetPort = MIDI_PORT_OS, uint32_t timeout_ms = 0);
  bool Receive(MidiPacket midipacket, uint32_t timeout_ms = 0);

  MidiPort();
  MidiPort(string name, uint16_t id, uint16_t queue_size = 64);
  MidiPort(string name, EMidiPortID port_class, uint16_t queue_size = 64);
  ~MidiPort();

  // Static methods for port management
  static bool OpenMidiPort(uint16_t port_id, MidiPort* midiPort);
  static void CloseMidiPort(uint16_t port_id);
  static bool RouteMidiPacket(MidiPacket midiPacket, uint16_t targetPort, uint16_t timeout_ms);
};
