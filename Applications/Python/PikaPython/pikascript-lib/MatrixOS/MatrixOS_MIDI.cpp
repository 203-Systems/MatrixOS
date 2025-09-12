#include "MatrixOS.h"
#include "pikaScript.h"

extern "C" {
    PikaObj* _MatrixOS_MIDI_Get(PikaObj *self, int timeout_ms) {
        MidiPacket packet;
        if (MatrixOS::MIDI::Get(&packet, timeout_ms)) {
            // Create a new MidiPacket object using the proper constructor
            PikaObj* New__MatrixOS_MidiPacket_MidiPacket(Args *args);
            void _MatrixOS_MidiPacket_MidiPacket___init__(PikaObj *self, int status, int channel, int data1, int data2);
            
            PikaObj* midi_packet = New__MatrixOS_MidiPacket_MidiPacket(NULL);
            
            // Initialize with the raw data - extract channel from data[0] for channel-based messages
            int channel = packet.data[0] & 0x0F;
            _MatrixOS_MidiPacket_MidiPacket___init__(midi_packet, packet.status, channel, packet.data[1], packet.data[2]);
            
            // Set the port separately
            obj_setInt(midi_packet, (char*)"port", packet.port);
            
            return midi_packet;
        }
        return nullptr;
    }

    pika_bool _MatrixOS_MIDI_Send(PikaObj *self, PikaObj* packet, int timeout_ms) {
        // Extract packet data
        uint16_t port = obj_getInt(packet, (char*)"port");
        EMidiStatus status = (EMidiStatus)obj_getInt(packet, (char*)"status");
        uint8_t data[3] = {
            (uint8_t)obj_getInt(packet, (char*)"data0"),
            (uint8_t)obj_getInt(packet, (char*)"data1"),
            (uint8_t)obj_getInt(packet, (char*)"data2")
        };
        
        // Create MidiPacket
        MidiPacket midi_packet;
        midi_packet.port = port;
        midi_packet.status = status;
        midi_packet.data[0] = data[0];
        midi_packet.data[1] = data[1];
        midi_packet.data[2] = data[2];
        
        return MatrixOS::MIDI::Send(midi_packet, timeout_ms);
    }

    pika_bool _MatrixOS_MIDI_SendSysEx(PikaObj *self, int port, int length, uint8_t* data, pika_bool include_meta) {
        return MatrixOS::MIDI::SendSysEx(port, length, data, include_meta != 0);
    }
}