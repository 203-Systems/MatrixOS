#include "MIDI.h"

namespace MatrixOS::USB {

  MIDI::MIDI() {

  }

  vector<MidiPort>* MIDI::Begin(string interface_name, uint16_t ep_size) {
    // this->interface_name = interface_name;
    // this->ep_size = ep_size;
    // this->cable_nums = 1;

    // MIDI::midi_interfaces.push_back(this);
    // this->midi_id = MIDI::midi_interfaces.size() - 1;
    // USB::AddInterface(this);
    Init(1, interface_name, ep_size);
    return &this->midi_ports;
  }

  vector<MidiPort>* MIDI::Begin(uint8_t cable_nums, string interface_name, uint16_t ep_size) {
    // this->interface_name = interface_name;
    // this->ep_size = ep_size;
    // this->cable_nums = cable_nums;

    Init(cable_nums, interface_name, ep_size);
    return &this->midi_ports;
  }

  MIDI::~MIDI() {

  }

  USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[512];
// USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[512];

  void MIDI::InterfaceTask(void* param) {
    vector<uint8_t>* out_ep = (vector<uint8_t>*)param;
    while(1)
    {
      if(USB::inited)
      {
        for(uint8_t i = 0; i < out_ep->size(); i++) {
          uint8_t ep = (*out_ep)[i];
          if (!MIDI::out_ep_busy[ep])
          {
            MLOGV("USBMIDI", "Starting read on ep %d to %p\n", ep, &read_buffer[4 * i]);
            int ret = usbd_ep_start_read(USB_BUS_ID, ep, &read_buffer[4 * i], 4);
            if (ret == 0)
            {
              MIDI::out_ep_busy[ep] = true;
            }
          }
        }
      }
      taskYIELD();
    }
  }


  void MIDI::Init(uint8_t cable_nums, string interface_name, uint16_t ep_size) {

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
      uint8_t total_length = 7 + cable_nums * 2 * (6 + 9) + cable_nums * 2 * (9 + 4 + 1);
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

    midi_ports.clear();
    midi_ports.reserve(cable_nums);

    in_ep.clear();
    in_ep.reserve(cable_nums);

    out_ep.clear();
    out_ep.reserve(cable_nums);

    for (uint8_t cable_idx = 0; cable_idx < cable_nums; cable_idx++)
    {
      // if (cable has input)
      {
        usbd_endpoint* ep = USB::AddEndpoint(EndpointType::Out, MIDI::usbd_midi_bulk_out);
        MLOGV("USBMIDI", "Adding MIDI Out Endpoint: 0x%02x\n", ep->ep_addr);
        out_ep.push_back(ep->ep_addr);
        out_ep_buffer[ep->ep_addr] = pvPortMalloc(4);
        out_ep_busy[ep->ep_addr] = false;
        vector<uint8_t> desc = {MIDI_DESCRIPTOR_ENDPOINT(ep->ep_addr, ep_size, 1), MIDI_JACKID_IN(MIDI_JACK_TYPE_EMBEDDED, cable_idx)};
        USB::AddInterfaceDescriptor(desc);
      }

      // if (cable has output)
      {
        usbd_endpoint* ep = USB::AddEndpoint(EndpointType::In, MIDI::usbd_midi_bulk_in);
        MLOGV("USBMIDI", "Adding MIDI In Endpoint: 0x%02x\n", ep->ep_addr);
        in_ep.push_back(ep->ep_addr);
        in_ep_buffer[ep->ep_addr] = pvPortMalloc(4);
        in_ep_busy[ep->ep_addr] = false;
        vector<uint8_t> desc = {MIDI_DESCRIPTOR_ENDPOINT(ep->ep_addr, ep_size, 1), MIDI_JACKID_OUT(MIDI_JACK_TYPE_EMBEDDED, cable_idx)};
        USB::AddInterfaceDescriptor(desc);
      }
      
      this->midi_ports.push_back(MidiPort("Midi Port", MIDI_PORT_USB));
    }

    
    // uint8_t ep_idx = USB_EP_GET_IDX(in_ep[0]);
    // void* otg_inep = (void*)USB_OTG_INEP(ep_idx);

    // MLOGV("MIDI" "ep: %d ep_idx: %d otg_inep: %p\n", in_ep[0], ep_idx, otg_inep);


    xTaskCreate(InterfaceTask, "Midi Port Task", configMINIMAL_STACK_SIZE * 4, &this->out_ep, configMAX_PRIORITIES - 2, &this->interface_task);
  }


  void MIDI::usbd_midi_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
  {
    // USB_LOG_RAW("Bulk Out - busid: %d, ep: %d, nbytes: %d\n", busid, ep, nbytes);
    out_ep_busy[ep] = false;
    // memcpy(&write_buffer[0], &read_buffer[0], nbytes);
    // usbd_ep_start_read(USB_BUS_ID, ep, read_buffer, sizeof(read_buffer));
  }

  void MIDI::usbd_midi_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
  {
    // USB_LOG_RAW("Bulk In - busid: %d, ep: %d, nbytes: %d\n", busid, ep, nbytes);
    in_ep_busy[ep] = false;
    //   if ((nbytes % 64) == 0 && nbytes) {
    //     usbd_ep_start_write(busid, ep, NULL, nbytes);
    // } 
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