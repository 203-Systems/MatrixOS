#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "PikaObjUtils.h"


extern "C" {
    // Forward declaration for MidiPacket constructor
    PikaObj* New__MatrixOS_MidiPacket_MidiPacket(Args *args);

    Arg* _MatrixOS_MIDI_Get(PikaObj *self, int timeout_ms) {
        MidiPacket packet;
        if (MatrixOS::MIDI::Get(&packet, timeout_ms)) {
            PikaObj* midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
            copyCppObjIntoPikaObj<MidiPacket>(midi_packet, packet);

            return arg_newObj(midi_packet);
        }
        return arg_newNone();
    }

    pika_bool _MatrixOS_MIDI_Send(PikaObj *self, PikaObj* packet, int timeout_ms) {
        MidiPacket* midi_packet_ptr = getCppObjPtrInPikaObj<MidiPacket>(packet);
        if (!midi_packet_ptr) return false;

        return MatrixOS::MIDI::Send(*midi_packet_ptr, timeout_ms);
    }

    pika_bool _MatrixOS_MIDI_SendSysEx(PikaObj *self, int port, int length, uint8_t* data, pika_bool include_meta) {
        return MatrixOS::MIDI::SendSysEx(port, length, data, include_meta != 0);
    }
}