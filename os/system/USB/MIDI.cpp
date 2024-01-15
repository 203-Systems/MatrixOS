#include "MatrixOS.h"
#include "MIDI.h"

namespace MatrixOS::USB
{

  MIDI::MIDI(string interface_name, uint16_t ep_size)
  {
      this->interface_name = interface_name;
      this->ep_size = ep_size;
      this->cable_nums = 1;

      // MIDI::midi_interfaces.push_back(this);
      // this->midi_id = MIDI::midi_interfaces.size() - 1;
      USB::AddInterface(this);
      Begin();
  }

  MIDI::MIDI(uint8_t cable_nums, string interface_name, uint16_t ep_size)
  { 
      this->interface_name = interface_name;
      this->ep_size = ep_size;
      this->cable_nums = cable_nums;

      // MIDI::midi_interfaces.push_back(this);
      // this->midi_id = MIDI::midi_interfaces.size() - 1;
      USB::AddInterface(this);
      Begin();
  }


  MIDI::~MIDI()
  {
    // for (TaskHandle_t port_task : port_tasks)
    // {
    //   vTaskDelete(port_task);
    // }
    
    // for (MidiPort midi_port : midi_ports)
    // {
    //   midi_port.Close();
    // }

    // MIDI::midi_interfaces[this->midi_id] = NULL;
  }


  void MIDI::Begin()
  {
    uint8_t itf = USB::RequestInterface();
    uint8_t itf_streaming = USB::RequestInterface();
    uint8_t ep_out = USB::RequestEndpoint(EndpointType::Out);
    uint8_t ep_in = USB::RequestEndpoint(EndpointType::In);

    uint8_t itf_str_idx = USB::AddString(interface_name);
    vector <uint8_t> cable_str_idx;

    ESP_LOGI("USB MIDI", "Adding MIDI Interface: %s", interface_name.c_str());
    ESP_LOGI("USB MIDI", "ITF: %d", itf);
    ESP_LOGI("USB MIDI", "ITF Streaming: %d", itf_streaming);
    ESP_LOGI("USB MIDI", "EP OUT: %d", ep_out);
    ESP_LOGI("USB MIDI", "EP IN: %d", ep_in);
    ESP_LOGI("USB MIDI", "ITF String Index: %d", itf_str_idx);

    // Add interface descriptor
    USB::AddInterfaceDescriptor({TUD_MIDI_DESC_HEAD(itf, itf_str_idx, cable_nums)});
    for (uint8_t cable_idx = 0; cable_idx < cable_nums; cable_idx++)
    {
      vector<uint8_t> desc = {TUD_MIDI_DESC_JACK_DESC(cable_idx + 1, 0)};
      USB::AddInterfaceDescriptor(desc);
    }
    USB::AddInterfaceDescriptor({TUD_MIDI_DESC_EP(ep_out, ep_size, cable_nums)});
    for (uint8_t cable_idx = 0; cable_idx < cable_nums; cable_idx++)
    {
      vector<uint8_t> desc = {TUD_MIDI_JACKID_IN_EMB(cable_idx + 1)};
      USB::AddInterfaceDescriptor(desc);
    }
    USB::AddInterfaceDescriptor({TUD_MIDI_DESC_EP(ep_in, ep_size, cable_nums)});
    for (uint8_t cable_idx = 0; cable_idx < cable_nums; cable_idx++)
    {
      vector<uint8_t> desc = {TUD_MIDI_JACKID_OUT_EMB(cable_idx + 1)};
      USB::AddInterfaceDescriptor(desc);
    }

    // Start port tasks
    for (uint8_t cable_idx = 0; cable_idx < cable_nums; cable_idx++)
    { 
      // TODO Generate name
      this->midi_ports.push_back(MidiPort("Midi Port", MIDI_PORT_USB + (itf << 4) + cable_idx));
      // xTaskCreate(PortTask, "Midi Port Task", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 2,
      //     &this->port_tasks[cable_idx]);
    }
  }

  // TODO NEED REWORK!
  // PASS IN itf for tud_midi_stream_write
  // Generate id based on # of MIDI interfaces + cable number


  void MIDI::PortTask(void* param) {
    // MidiPort* port = (MidiPort*)param;
    // MidiPacket packet;
    // while (true)
    // {
    //   if (port->Get(&packet, portMAX_DELAY))
    //   { tud_midi_n_stream_write(port->id % 0x100, packet.data, packet.Length()); }
    // }
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
            // reference library does not handle quarter frame.
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
    // MatrixOS::USB::MIDI::ports[itf]->Send(packet); // Wrapped implementation
    MatrixOS::MIDI::Receive(packet); //Direct call to MIDI::Receive
  }
}