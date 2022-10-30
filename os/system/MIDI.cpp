#include "MatrixOS.h"
#include <map>

namespace MatrixOS::MIDI
{  
    QueueHandle_t midi_queue; 
    std::map<uint16_t, MidiPort*> midiPortMap;

    void Init(void)
    {
        midi_queue = xQueueCreate(MIDI_QUEUE_SIZE, sizeof(MidiPacket));
    }

    bool Get(MidiPacket* midipacket_dest, uint16_t timeout_ms)
    {
        return xQueueReceive(midi_queue, midipacket_dest, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
    }

    bool Send(MidiPacket* midiPacket)
    {
        if(midiPacket->port == MIDI_PORT_ALL_CLASS)
        {
            uint16_t targetClass = MIDI_PORT_USB;
            bool send = false;
            for(std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.begin(); port != midiPortMap.end(); ++port)
            {
                if(port->first >= MIDI_PORT_DEVICE_CUSTOM + 0x100)
                {
                    return send;
                }
                if(port->first >= targetClass)
                {
                    send |= port->second->Recive(midiPacket, 0);
                    targetClass = (port->first / 0x100 + 1) * 0x100;
                }
            }
        }
        else
        {
            std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.find(midiPacket->port);
            if(port != midiPortMap.end())
            {
                return port->second->Recive(midiPacket, 0);
            }
        }
        return false;
    }

    bool RegisterMidiPort(uint16_t port_id, MidiPort* midiPort)
    {   
        if(port_id < 0x100) return false;

        if (midiPortMap.find(port_id) == midiPortMap.end()) 
        {
            midiPortMap[port_id] = midiPort;
            return true;
        }
        return false;
    }

    void UnregisterMidiPort(uint16_t port_id)
    {
        midiPortMap.erase(port_id);
    }

    bool Recive(MidiPacket* midipacket_prt, uint32_t timeout_ms)
    {
        if(uxQueueSpacesAvailable(midi_queue) == 0)
        {
            //TODO: Drop first element
        }
        xQueueSend(midi_queue, midipacket_prt, pdMS_TO_TICKS(timeout_ms));
        return uxQueueSpacesAvailable(midi_queue) == 0;
    }

    MidiPacket DispatchUSBPacket(uint8_t rawPacket[4])
    {
        uint8_t packet[4];
        uint8_t port = 1;
        memcpy(packet, rawPacket, 4);
        // MatrixOS::Logging::LogDebug("USB MIDI Packet", "%#02X %#02X %#02X %#02X", packet[0], packet[1], packet[2], packet[3]);
        switch (packet[0]) 
        {
            case CIN_3BYTE_SYS_COMMON:
                if (packet[1] == MIDIv1_SONG_POSITION_PTR) 
                    return MidiPacket(port, SongPosition, 2, &packet[2]);
                break;

            case CIN_2BYTE_SYS_COMMON:
                switch (packet[1]) {
                    case MIDIv1_SONG_SELECT:
                        return MidiPacket(port, SongSelect, 1, &packet[2]);
                    case MIDIv1_MTC_QUARTER_FRAME:
                        // reference library doesnt handle quarter frame.
                        break;
                }
                break;
            case CIN_NOTE_OFF:
                // packet[1] = MIDIv1_VOICE_CHANNEL(packet[1]); //Strip status value, leaving only channel
                return MidiPacket(port, NoteOff, 3, &packet[1]);
            case CIN_NOTE_ON:
                // packet[1] = MIDIv1_VOICE_CHANNEL(packet[1]);
                return MidiPacket(port, NoteOn, 3, &packet[1]);
            case CIN_AFTER_TOUCH:
                // packet[1] = MIDIv1_VOICE_CHANNEL(packet[1]);
                return MidiPacket(port, AfterTouch, 3, &packet[1]);
            case CIN_CONTROL_CHANGE:
                // packet[1] = MIDIv1_VOICE_CHANNEL(packet[1]);
                return MidiPacket(port, ControlChange, 3, &packet[1]);
            case CIN_PROGRAM_CHANGE:
                // packet[1] = MIDIv1_VOICE_CHANNEL(packet[1]);
                return MidiPacket(port, ProgramChange, 2, &packet[1]);
            case CIN_CHANNEL_PRESSURE:
                // packet[1] = MIDIv1_VOICE_CHANNEL(packet[1]);
                return MidiPacket(port, ChannelPressure, 2, &packet[1]);
            case CIN_PITCH_WHEEL:
                // packet[1] = MIDIv1_VOICE_CHANNEL(packet[1]);
                return MidiPacket(port, PitchChange, 3, &packet[1]);
            case CIN_1BYTE:
                switch (packet[1]) 
                {
                    case MIDIv1_CLOCK:
                        return MidiPacket(port, Sync);
                    case MIDIv1_TICK:
                        return MidiPacket(port, Tick);
                    case MIDIv1_START:
                        return MidiPacket(port, Start);
                    case MIDIv1_CONTINUE:
                        return MidiPacket(port, Continue);
                    case MIDIv1_STOP:
                        return MidiPacket(port, Stop);
                    case MIDIv1_ACTIVE_SENSE:
                        return MidiPacket(port, ActiveSense);
                    case MIDIv1_RESET:
                        return MidiPacket(port, Reset);
                    case MIDIv1_TUNE_REQUEST:
                        return MidiPacket(port, TuneRequest);
                }
                break;
        }
        return MidiPacket(port, None);
    }

    void SendPacketUSB(MidiPacket midiPacket)
    {
        if(midiPacket.port == 0 || midiPacket.port == 1)
        {
            tud_midi_stream_write(0, midiPacket.data, midiPacket.Length());
            // MatrixOS::USB::CDC::Println("Sent Midi Packet");
        }
    }

    void SendPacket(MidiPacket midiPacket)
    {
        SendPacketUSB(midiPacket);

        #ifdef DEVICE_MIDI
        Device::MIDI::Send(midiPacket);
        #endif
    }

    void SendNoteOff(uint8_t channel, uint8_t note, uint8_t velocity)
    {
        uint8_t packet[3] = { (uint8_t)(MIDIv1_NOTE_OFF | (channel & 0x0f)), note, velocity};
        tud_midi_stream_write(0, packet, 3);

    }

    void SendNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
    {
        uint8_t packet[3] = { (uint8_t)(MIDIv1_NOTE_ON | (channel & 0x0f)), note, velocity};
        tud_midi_stream_write(0, packet, 3);
    }

    void SendAfterTouch(uint8_t channel, uint8_t note, uint8_t velocity)
    {   
        uint8_t packet[3] = { (uint8_t)(MIDIv1_AFTER_TOUCH | (channel & 0x0f)), note, velocity};
        tud_midi_stream_write(0, packet, 3);
    }

    void SendControlChange(uint8_t channel, uint8_t controller, uint8_t value)
    {
        uint8_t packet[3] = { (uint8_t)(MIDIv1_CONTROL_CHANGE | (channel & 0x0f)), controller, value};
        tud_midi_stream_write(0, packet, 3);
    }

    void SendProgramChange(uint8_t channel, uint8_t program)
    {
        uint8_t packet[2] = { (uint8_t)(MIDIv1_PROGRAM_CHANGE | (channel & 0x0f)), program};
        tud_midi_stream_write(0, packet, 2);
    }

    void SendChannelPressure(uint8_t channel, uint8_t velocity)
    {
        uint8_t packet[2] = { (uint8_t)(MIDIv1_CHANNEL_PRESSURE | (channel & 0x0f)), velocity};
        tud_midi_stream_write(0, packet, 2);
    }

    void SendPitchChange(uint8_t channel, uint16_t pitch)
    {
        uint8_t packet[3] = { (uint8_t)(MIDIv1_PITCH_WHEEL | (channel & 0x0f)), (uint8_t)(pitch & 0x07F), (uint8_t)((pitch>>7) & 0x7f)};
        tud_midi_stream_write(0, packet, 3);
    }

    void SendSongPosition(uint16_t position)
    {
        uint8_t packet[3] = { MIDIv1_SONG_POSITION_PTR, (uint8_t)(position & 0x07F), (uint8_t)((position>>7) & 0x7f)};
        tud_midi_stream_write(0, packet, 3);
    }

    void SendSongSelect(uint8_t song)
    {
        uint8_t packet[2] = { MIDIv1_SONG_SELECT, song};
        tud_midi_stream_write(0, packet, 2);
    }

    void SendTuneRequest(void)
    {
        uint8_t packet[1] = { MIDIv1_TUNE_REQUEST};
        tud_midi_stream_write(0, packet, 1);
    }

    void SendSync(void)
    {
        uint8_t packet[1] = { MIDIv1_CLOCK };
        tud_midi_stream_write(0, packet, 1);
    }

    void SendStart(void)
    {
        uint8_t packet[1] = { MIDIv1_START};
        tud_midi_stream_write(0, packet, 1);
    }

    void SendContinue(void)
    {
        uint8_t packet[1] = { MIDIv1_CONTINUE};
        tud_midi_stream_write(0, packet, 1);
    }

    void SendStop(void)
    {
        uint8_t packet[1] = { MIDIv1_STOP};
        tud_midi_stream_write(0, packet, 1);
    }

    void SendActiveSense(void)
    {
        uint8_t packet[1] = { MIDIv1_ACTIVE_SENSE};
        tud_midi_stream_write(0, packet, 1);
    }

    void SendReset(void)
    {
        uint8_t packet[1] = { MIDIv1_RESET};
        tud_midi_stream_write(0, packet, 1);
    }
}