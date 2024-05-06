#include "MIDI.h"

namespace MatrixOS::USB {

  MIDI::MIDI(string interface_name, uint16_t ep_size) {
    this->interface_name = interface_name;
    this->ep_size = ep_size;
    this->cable_nums = 1;

    // MIDI::midi_interfaces.push_back(this);
    // this->midi_id = MIDI::midi_interfaces.size() - 1;
    // USB::AddInterface(this);
    Begin();
  }

  MIDI::MIDI(uint8_t cable_nums, string interface_name, uint16_t ep_size) {
    this->interface_name = interface_name;
    this->ep_size = ep_size;
    this->cable_nums = cable_nums;

    Begin();
  }

  MIDI::~MIDI() {

  }

  void MIDI::Begin() {

    // ESP_LOGI("USB MIDI", "Adding MIDI Interface: %s", interface_name.c_str());
    // ESP_LOGI("USB MIDI", "ITF: %d", itf);
    // ESP_LOGI("USB MIDI", "ITF Streaming: %d", itf_streaming);
    // ESP_LOGI("USB MIDI", "EP OUT: %d", ep_out);
    // ESP_LOGI("USB MIDI", "EP IN: %d", ep_in);
    // ESP_LOGI("USB MIDI", "ITF String Index: %d", itf_str_idx);

    // Add interface descriptor
    {
      usbd_interface* itf = USB::AddInterface();
      usbd_interface* itf_streaming = USB::AddInterface();

      /* Audio Control (AC) Interface */
      USB::AddInterfaceDescriptor({AUDIO_AC_STANDARD_DESCRIPTOR_INIT(itf->intf_num, USB::AddString(interface_name))});
      /* AC Header */
      USB::AddInterfaceDescriptor({AUDIO_AC_HEADER_DESCRIPTOR_INIT(itf_streaming->intf_num)});
      /* MIDI Streaming (MS) Interface */
      USB::AddInterfaceDescriptor({AUDIO_MS_STANDARD_DESCRIPTOR_INIT(itf_streaming->intf_num, 2)});
      /* MS Header */
      uint8_t total_length = 9 + 9 + 9 + 7 + cable_nums * 2 * (6 + 9) + cable_nums * 2 * (9 + 4 + 1);
      USB::AddInterfaceDescriptor({MIDI_CS_HEADER_DESCRIPTOR_INIT(total_length)});
    }

    for (uint8_t cable_idx = 0; cable_idx < cable_nums; cable_idx++)
    {
      uint8_t str_idx = 0;   // TODO

      // if (cable has input)
      {
        vector<uint8_t> desc = {MIDI_IN_JACK_DESCRIPTOR_INIT(MIDI_JACK_TYPE_EXTERNAL, cable_idx, str_idx),
                                MIDI_OUT_JACK_DESCRIPTOR_INIT(MIDI_JACK_TYPE_EMBEDDED, cable_idx, MIDI_JACK_TYPE_EXTERNAL, cable_idx, str_idx)};
        USB::AddInterfaceDescriptor(desc);
      }
      
      // if (cable has output)
      {
        vector<uint8_t> desc = {MIDI_IN_JACK_DESCRIPTOR_INIT(MIDI_JACK_TYPE_EMBEDDED, cable_idx, str_idx),
                                MIDI_OUT_JACK_DESCRIPTOR_INIT(MIDI_JACK_TYPE_EXTERNAL, cable_idx, MIDI_JACK_TYPE_EMBEDDED, cable_idx, str_idx)};
        USB::AddInterfaceDescriptor(desc);
      }
    }

    // Endpoint descriptors

    for (uint8_t cable_idx = 0; cable_idx < cable_nums; cable_idx++)
    {
      // if (cable has input)
      {
        usbd_endpoint* ep = USB::AddEndpoint(EndpointType::In, MIDI::usbd_midi_bulk_in);
        vector<uint8_t> desc = {MIDI_DESCRIPTOR_ENDPOINT(ep->ep_addr, ep_size, 1), MIDI_JACKID_OUT(MIDI_JACK_TYPE_EMBEDDED, cable_idx)};
        USB::AddInterfaceDescriptor(desc);
      }
      
      // if (cable has output)
      {
        usbd_endpoint* ep = USB::AddEndpoint(EndpointType::Out, MIDI::usbd_midi_bulk_out);
        vector<uint8_t> desc = {MIDI_DESCRIPTOR_ENDPOINT(ep->ep_addr, ep_size, 1), MIDI_JACKID_IN(MIDI_JACK_TYPE_EMBEDDED, cable_idx)};
        USB::AddInterfaceDescriptor(desc);
      }
    }

      // // Start port tasks
      // for (uint8_t cable_idx = 0; cable_idx < cable_nums; cable_idx++)
      // {
      //   // TODO Generate name
      //   this->midi_ports.push_back(MidiPort("Midi Port", MIDI_PORT_USB + (itf << 4) + cable_idx));
      //   // xTaskCreate(PortTask, "Midi Port Task", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 2,
      //   //     &this->port_tasks[cable_idx]);
      // }
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

  void MIDI::usbd_midi_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
  {
  }

  void MIDI::usbd_midi_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
  {
  }
}

// void tud_midi_rx_cb(uint8_t itf) {
//   uint8_t raw_packet[4];
//   uint16_t port = MIDI_PORT_USB + itf;
//   MidiPacket packet = MidiPacket(port, None);
//   while (tud_midi_n_packet_read(itf, raw_packet))
//   {
//     switch (raw_packet[0])
//     {
//       case CIN_3BYTE_SYS_COMMON:
//         if (raw_packet[1] == MIDIv1_SONG_POSITION_PTR)
//           packet = MidiPacket(port, SongPosition, 2, &raw_packet[2]);
//         break;

//       case CIN_2BYTE_SYS_COMMON:
//         switch (raw_packet[1])
//         {
//           case MIDIv1_SONG_SELECT:
//             packet = MidiPacket(port, SongSelect, 1, &raw_packet[2]);
//           case MIDIv1_MTC_QUARTER_FRAME:
//             // reference library does not handle quarter frame.
//             break;
//         }
//         break;
//       case CIN_NOTE_OFF:
//         packet = MidiPacket(port, NoteOff, 3, &raw_packet[1]);
//         break;
//       case CIN_NOTE_ON:
//         packet = MidiPacket(port, NoteOn, 3, &raw_packet[1]);
//         break;
//       case CIN_AFTER_TOUCH:
//         packet = MidiPacket(port, AfterTouch, 3, &raw_packet[1]);
//         break;
//       case CIN_CONTROL_CHANGE:
//         packet = MidiPacket(port, ControlChange, 3, &raw_packet[1]);
//         break;
//       case CIN_PROGRAM_CHANGE:
//         packet = MidiPacket(port, ProgramChange, 2, &raw_packet[1]);
//         break;
//       case CIN_CHANNEL_PRESSURE:
//         packet = MidiPacket(port, ChannelPressure, 2, &raw_packet[1]);
//         break;
//       case CIN_PITCH_WHEEL:
//         packet = MidiPacket(port, PitchChange, 3, &raw_packet[1]);
//         break;
//       case CIN_1BYTE:
//         switch (raw_packet[1])
//         {
//           case MIDIv1_CLOCK:
//             packet = MidiPacket(port, Sync);
//             break;
//           case MIDIv1_TICK:
//             packet = MidiPacket(port, Tick);
//             break;
//           case MIDIv1_START:
//             packet = MidiPacket(port, Start);
//             break;
//           case MIDIv1_CONTINUE:
//             packet = MidiPacket(port, Continue);
//             break;
//           case MIDIv1_STOP:
//             packet = MidiPacket(port, Stop);
//             break;
//           case MIDIv1_ACTIVE_SENSE:
//             packet = MidiPacket(port, ActiveSense);
//             break;
//           case MIDIv1_RESET:
//             packet = MidiPacket(port, Reset);
//             break;
//           case MIDIv1_TUNE_REQUEST:
//             packet = MidiPacket(port, TuneRequest);
//             break;
//         }
//         break;
//       case CIN_SYSEX:
//         packet = MidiPacket(port, SysExData, 3, &raw_packet[1]);
//         break;
//       case CIN_SYSEX_ENDS_IN_1:
//       case CIN_SYSEX_ENDS_IN_2:
//       case CIN_SYSEX_ENDS_IN_3:
//         packet = MidiPacket(port, SysExEnd, 3, &raw_packet[1]);
//         break;
//       default:
//         return;
//     }

//     // Since we know what we are doing here, just gonna skip the wrapper
//     // MatrixOS::USB::MIDI::ports[itf]->Send(packet); // Wrapped implementation
//     MatrixOS::MIDI::Receive(packet);  // Direct call to MIDI::Receive
//   }
// }