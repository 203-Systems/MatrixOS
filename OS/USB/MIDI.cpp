#include "MatrixOS.h"
#include "USB.h"
#include "tusb.h"

namespace MatrixOS::MIDI
{
  extern MidiPort* osPort;
}

namespace MatrixOS::USB::MIDI
{
  std::vector<MidiPort*> ports;
  std::vector<TaskHandle_t> portTasks;
  std::vector<string> portTaskNames;
  
  std::vector<uint8_t> sysex_buffer;

  void portTask(void* param) {
    uint8_t itf = ports.size();
    string portname = "USB MIDI " + std::to_string(itf + 1);
    MidiPort port = MidiPort(portname, MIDI_PORT_USB + itf);
    ports.push_back(&port);
    MidiPacket packet;
    while (true)
    {
      if (port.Get(&packet, portMAX_DELAY))
      { tud_midi_stream_write(port.id % 0x100, packet.data, packet.Length()); }
    }
  }

  void Init() {
    ports.clear();
    
    for (TaskHandle_t portTask : portTasks)
    {
      vTaskDelete(portTask);
    }

    ports.reserve(USB_MIDI_COUNT);
    portTasks.reserve(USB_MIDI_COUNT);
    
    for (uint8_t i = 0; i < USB_MIDI_COUNT; i++)
    {
      portTasks.push_back(NULL);
      portTaskNames.push_back("USB MIDI " + std::to_string(i + 1));
      xTaskCreate(portTask, portTaskNames.back().c_str(), configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 2,
                  &portTasks.back());
    }
  }
}

void tud_midi_rx_cb(uint8_t itf) {
  uint8_t raw_packet[4];
  while (tud_midi_n_packet_read(itf, raw_packet))
  {
    uint16_t port = MIDI_PORT_USB + (raw_packet[0] >> 4);
    MidiPacket packet = MidiPacket(EMidiStatus::None);
    raw_packet[0] &= 0x0F;
    switch (raw_packet[0])
    {
      case CIN_3BYTE_SYS_COMMON:
        if (raw_packet[1] == MIDIv1_SONG_POSITION_PTR)
          packet = MidiPacket::SongPosition(raw_packet[2] | (raw_packet[3] << 7));
        break;

      case CIN_2BYTE_SYS_COMMON:
        switch (raw_packet[1])
        {
          case MIDIv1_SONG_SELECT:
            packet = MidiPacket::SongSelect(raw_packet[2]);
            break;
          case MIDIv1_MTC_QUARTER_FRAME:
            packet = MidiPacket::MTCQuarterFrame(raw_packet[2]);
            break;
        }
        break;
      case CIN_NOTE_OFF:
        packet = MidiPacket::NoteOff(raw_packet[1] & 0x0F, raw_packet[2], raw_packet[3]);
        break;
      case CIN_NOTE_ON:
        packet = MidiPacket::NoteOn(raw_packet[1] & 0x0F, raw_packet[2], raw_packet[3]);
        break;
      case CIN_AFTER_TOUCH:
        packet = MidiPacket::AfterTouch(raw_packet[1] & 0x0F, raw_packet[2], raw_packet[3]);
        break;
      case CIN_CONTROL_CHANGE:
        packet = MidiPacket::ControlChange(raw_packet[1] & 0x0F, raw_packet[2], raw_packet[3]);
        break;
      case CIN_PROGRAM_CHANGE:
        packet = MidiPacket::ProgramChange(raw_packet[1] & 0x0F, raw_packet[2]);
        break;
      case CIN_CHANNEL_PRESSURE:
        packet = MidiPacket::ChannelPressure(raw_packet[1] & 0x0F, raw_packet[2]);
        break;
      case CIN_PITCH_WHEEL:
        packet = MidiPacket::PitchBend(raw_packet[1] & 0x0F, raw_packet[2] | (raw_packet[3] << 7));
        break;
      case CIN_1BYTE:
        switch (raw_packet[1])
        {
          case MIDIv1_CLOCK:
            packet = MidiPacket::Sync();
            break;
          case MIDIv1_TICK:
            packet = MidiPacket::Tick();
            break;
          case MIDIv1_START:
            packet = MidiPacket::Start();
            break;
          case MIDIv1_CONTINUE:
            packet = MidiPacket::Continue();
            break;
          case MIDIv1_STOP:
            packet = MidiPacket::Stop();
            break;
          case MIDIv1_ACTIVE_SENSE:
            packet = MidiPacket::ActiveSense();
            break;
          case MIDIv1_RESET:
            packet = MidiPacket::Reset();
            break;
          case MIDIv1_TUNE_REQUEST:
            packet = MidiPacket::TuneRequest();
            break;
        }
        break;
      case CIN_SYSEX:
        packet = MidiPacket(EMidiStatus::SysExData, raw_packet[1], raw_packet[2], raw_packet[3]);
        break;
      case CIN_SYSEX_ENDS_IN_1:
      case CIN_SYSEX_ENDS_IN_2:
      case CIN_SYSEX_ENDS_IN_3:
        packet = MidiPacket(EMidiStatus::SysExEnd, raw_packet[1], raw_packet[2], raw_packet[3]);
        break;
      default: 
        return;
    }  

    // Since we know what we are doing here, just gonna skip the wrapper
    // MatrixOS::USB::MIDI::ports[itf]->Send(packet); // Wrapped implementation
    packet.SetPort(port);
    MatrixOS::MIDI::osPort->Receive(packet);
  }
}