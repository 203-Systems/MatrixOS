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
  QueueHandle_t midiQueue;

  uint16_t Open(uint16_t id, uint16_t queueSize = 64, uint16_t idRange = 1);
  void Close();
  void SetName(string name);
  bool Get(MidiPacket* midiPacketDest, uint32_t timeoutMs = 0);
  bool Send(MidiPacket midiPacket, uint16_t targetPort = MIDI_PORT_OS, uint32_t timeoutMs = 0);
  bool Receive(MidiPacket midiPacket, uint32_t timeoutMs = 0);

  MidiPort();
  MidiPort(string name, uint16_t id, uint16_t queueSize = 64);
  MidiPort(string name, EMidiPortID portClass, uint16_t queueSize = 64);
  ~MidiPort();

  // Static methods for port management
  static bool OpenMidiPort(uint16_t portId, MidiPort* midiPort);
  static void CloseMidiPort(uint16_t portId);
  static bool RouteMidiPacket(MidiPacket midiPacket, uint16_t targetPort, uint16_t timeoutMs);
};
