#pragma once

#include "Application.h"
#include "MatrixOS.h"

class DeveloperApp : public Application {
public:
  inline static Application_Info info = {
      .name = "DeveloperApp",
      .author = "203 Systems",
      .color = Color::White,
      .version = 1,
      .visibility = false,
  };

  void Setup(const vector<string>& args) override;
  void Loop() override;
  void End() override;

private:
  bool hidKeyReportEnabled = false;
  bool midiKeyReportEnabled = false;
  bool sysExKeyReportEnabled = false;
  vector<uint8_t> sysExBuffer;
  uint16_t activeSysExPort = MIDI_PORT_INVALID;
  uint16_t sysExReportPort = MIDI_PORT_INVALID;

  void DrainInput();
  void DrainHID();
  void DrainMIDI();

  void HandleInputEvent(const InputEvent& event);
  void HandleHIDReport(const uint8_t* report, size_t size);
  void HandleMIDIPacket(const MidiPacket& packet);
  void HandleSysExPacket(const MidiPacket& packet);
  void HandleSysExMessage(uint16_t port, const vector<uint8_t>& message);
};
