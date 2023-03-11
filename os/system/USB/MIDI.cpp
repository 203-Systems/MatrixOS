#include "MatrixOS.h"

namespace MatrixOS::USB::MIDI
{
  std::vector<MidiPort*> ports;
  std::vector<TaskHandle_t> portTasks;
  
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
    ports.reserve(USB_MIDI_COUNT);
    portTasks.reserve(USB_MIDI_COUNT);
    for (uint8_t i = 0; i < USB_MIDI_COUNT; i++)
    {
      string portname = "USB MIDI " + std::to_string(i + 1);
      xTaskCreate(portTask, portname.c_str(), configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 2,
                  &portTasks[i]);
    }
  }
}

void tud_midi_rx_cb(uint8_t itf) {
  uint8_t raw_packet[4];
  uint16_t port = MIDI_PORT_USB + itf;
  MidiPacket packet = MidiPacket(port, None);
  while (tud_midi_n_packet_read(itf, raw_packet))
  {
    switch (raw_packet[0])
    {
      case CIN_3BYTE_SYS_COMMON:
        if (raw_packet[1] == MIDIv1_SONG_POSITION_PTR)
          packet = MidiPacket(port, SongPosition, 2, &raw_packet[2]);
        break;

      case CIN_2BYTE_SYS_COMMON:
        switch (raw_packet[1])
        {
          case MIDIv1_SONG_SELECT:
            packet = MidiPacket(port, SongSelect, 1, &raw_packet[2]);
          case MIDIv1_MTC_QUARTER_FRAME:
            // reference library doesnt handle quarter frame.
            break;
        }
        break;
      case CIN_NOTE_OFF:
        packet = MidiPacket(port, NoteOff, 3, &raw_packet[1]);
        break;
      case CIN_NOTE_ON:
        packet = MidiPacket(port, NoteOn, 3, &raw_packet[1]);
        break;
      case CIN_AFTER_TOUCH:
        packet = MidiPacket(port, AfterTouch, 3, &raw_packet[1]);
        break;
      case CIN_CONTROL_CHANGE:
        packet = MidiPacket(port, ControlChange, 3, &raw_packet[1]);
        break;
      case CIN_PROGRAM_CHANGE:
        packet = MidiPacket(port, ProgramChange, 2, &raw_packet[1]);
        break;
      case CIN_CHANNEL_PRESSURE:
        packet = MidiPacket(port, ChannelPressure, 2, &raw_packet[1]);
        break;
      case CIN_PITCH_WHEEL:
        packet = MidiPacket(port, PitchChange, 3, &raw_packet[1]);
        break;
      case CIN_1BYTE:
        switch (raw_packet[1])
        {
          case MIDIv1_CLOCK:
            packet = MidiPacket(port, Sync);
            break;
          case MIDIv1_TICK:
            packet = MidiPacket(port, Tick);
            break;
          case MIDIv1_START:
            packet = MidiPacket(port, Start);
            break;
          case MIDIv1_CONTINUE:
            packet = MidiPacket(port, Continue);
            break;
          case MIDIv1_STOP:
            packet = MidiPacket(port, Stop);
            break;
          case MIDIv1_ACTIVE_SENSE:
            packet = MidiPacket(port, ActiveSense);
            break;
          case MIDIv1_RESET:
            packet = MidiPacket(port, Reset);
            break;
          case MIDIv1_TUNE_REQUEST:
            packet = MidiPacket(port, TuneRequest);
            break;
        }
        break;
      case CIN_SYSEX:
        packet = MidiPacket(port, SysExData, 3, &raw_packet[1]);
        break;
      case CIN_SYSEX_ENDS_IN_1:
      case CIN_SYSEX_ENDS_IN_2:
      case CIN_SYSEX_ENDS_IN_3:
        packet = MidiPacket(port, SysExEnd, 3, &raw_packet[1]);
        break;
      default: 
        return;
    }  

    // Since we know what we are doing here, just gonna skip the wrapper
    // MatrixOS::USB::MIDI::ports[itf]->Send(packet); // Wrapped implentmentation
    MatrixOS::MIDI::Recive(packet); //Direct call to MIDI::Recive
  }
}