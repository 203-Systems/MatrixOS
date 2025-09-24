#include "MatrixOS.h"
#include "pikaScript.h"
#include "PikaObj.h"
#include "../PikaObjUtils.h"

extern "C" {
    // PikaObj constructor
    PikaObj* New__MatrixOS_MidiPacket_MidiPacket(Args *args);

    // MidiPacket constructor with variable arguments
    void _MatrixOS_MidiPacket_MidiPacket___init__(PikaObj *self, PikaTuple* val) {
        uint8_t args_size = pikaTuple_getSize(val);
        
        if(args_size == 1)
        {
            Arg* status_arg = pikaTuple_getArg(val, 0);
            int status = arg_getInt(status_arg);
            createCppObjPtrInPikaObj<MidiPacket>(self, (EMidiStatus)status);
        }
        if(args_size == 2)
        {
            Arg* status_arg = pikaTuple_getArg(val, 0);
            Arg* data1_arg = pikaTuple_getArg(val, 0);

            int status = arg_getInt(status_arg);
            int data1 = arg_getInt(data1_arg);

            createCppObjPtrInPikaObj<MidiPacket>(self, (EMidiStatus)status, data1);
        }
        if(args_size == 3)
        {
            Arg* status_arg = pikaTuple_getArg(val, 0);
            Arg* data1_arg = pikaTuple_getArg(val, 1);
            Arg* data2_arg = pikaTuple_getArg(val, 2);

            int status = arg_getInt(status_arg);
            int data1 = arg_getInt(data1_arg);
            int data2 = arg_getInt(data2_arg);

            createCppObjPtrInPikaObj<MidiPacket>(self, (EMidiStatus)status, data1, data2);
        }
        else if(args_size == 4)
        {
            Arg* status_arg = pikaTuple_getArg(val, 0);
            Arg* data1_arg = pikaTuple_getArg(val, 1);
            Arg* data2_arg = pikaTuple_getArg(val, 2);
            Arg* data3_arg = pikaTuple_getArg(val, 3);

            int status = arg_getInt(status_arg);
            int data1 = arg_getInt(data1_arg);
            int data2 = arg_getInt(data2_arg);
            int data3 = arg_getInt(data3_arg);

            createCppObjPtrInPikaObj<MidiPacket>(self, (EMidiStatus)status, data1, data2, data3);
        }
        else
        {
            createCppObjPtrInPikaObj<MidiPacket>(self);
        }
    }

    // Factory methods for creating MIDI packets
    PikaObj* _MatrixOS_MidiPacket_MidiPacket_NoteOn(PikaObj *self, int channel, int note, int velocity) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::NoteOn(channel, note, velocity);

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_NoteOff(PikaObj *self, int channel, int note, int velocity) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::NoteOff(channel, note, velocity);

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_AfterTouch(PikaObj *self, int channel, int note, int pressure) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::AfterTouch(channel, note, pressure);

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_ControlChange(PikaObj *self, int channel, int controller, int value) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::ControlChange(channel, controller, value);

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_ProgramChange(PikaObj *self, int channel, int program) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::ProgramChange(channel, program);

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_ChannelPressure(PikaObj *self, int channel, int pressure) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::ChannelPressure(channel, pressure);

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_PitchBend(PikaObj *self, int channel, int value) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::PitchBend(channel, value);

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_Clock(PikaObj *self) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::Sync();

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_Start(PikaObj *self) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::Start();

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_Continue(PikaObj *self) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::Continue();

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_Stop(PikaObj *self) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::Stop();

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_ActiveSense(PikaObj *self) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::ActiveSense();

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_Reset(PikaObj *self) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::Reset();

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_SongPosition(PikaObj *self, int position) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::SongPosition(position);

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_SongSelect(PikaObj *self, int song) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::SongSelect(song);

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    PikaObj* _MatrixOS_MidiPacket_MidiPacket_TuneRequest(PikaObj *self) {
        PikaObj* new_midi_packet = newNormalObj(New__MatrixOS_MidiPacket_MidiPacket);
        MidiPacket packet = MidiPacket::TuneRequest();

        copyCppObjIntoPikaObj<MidiPacket>(new_midi_packet, packet);

        return new_midi_packet;
    }

    // Port getter
    int _MatrixOS_MidiPacket_MidiPacket_Port(PikaObj *self) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return MIDI_PORT_INVALID;
        return packet->port;
    }

    // Status getter/setter
    int _MatrixOS_MidiPacket_MidiPacket_Status(PikaObj *self) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return 0;
        return packet->status;
    }

    pika_bool _MatrixOS_MidiPacket_MidiPacket_SetStatus(PikaObj *self, int status) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return false;
        return packet->SetStatus((EMidiStatus)status);
    }

    // Channel getter/setter
    int _MatrixOS_MidiPacket_MidiPacket_Channel(PikaObj *self) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return 0;
        return packet->Channel();
    }

    pika_bool _MatrixOS_MidiPacket_MidiPacket_SetChannel(PikaObj *self, int channel) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return false;
        return packet->SetChannel((uint8_t)channel);
    }

    // Note getter/setter
    int _MatrixOS_MidiPacket_MidiPacket_Note(PikaObj *self) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return 0;
        return packet->Note();
    }

    pika_bool _MatrixOS_MidiPacket_MidiPacket_SetNote(PikaObj *self, int note) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return false;
        return packet->SetNote((uint8_t)note);
    }

    // Controller getter/setter (alias for Note)
    int _MatrixOS_MidiPacket_MidiPacket_Controller(PikaObj *self) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return 0;
        return packet->Controller();
    }

    pika_bool _MatrixOS_MidiPacket_MidiPacket_SetController(PikaObj *self, int controller) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return false;
        return packet->SetController((uint8_t)controller);
    }

    // Velocity getter/setter
    int _MatrixOS_MidiPacket_MidiPacket_Velocity(PikaObj *self) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return 0;
        return packet->Velocity();
    }

    pika_bool _MatrixOS_MidiPacket_MidiPacket_SetVelocity(PikaObj *self, int velocity) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return false;
        return packet->SetVelocity((uint8_t)velocity);
    }

    // Value getter/setter
    int _MatrixOS_MidiPacket_MidiPacket_Value(PikaObj *self) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return 0;
        return packet->Value();
    }

    pika_bool _MatrixOS_MidiPacket_MidiPacket_SetValue(PikaObj *self, int value) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return false;
        return packet->SetValue((uint16_t)value);
    }

    // Helper methods
    int _MatrixOS_MidiPacket_MidiPacket_Length(PikaObj *self) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return 0;
        return packet->Length();
    }

    pika_bool _MatrixOS_MidiPacket_MidiPacket_SysEx(PikaObj *self) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return false;
        return packet->SysEx();
    }

    pika_bool _MatrixOS_MidiPacket_MidiPacket_SysExStart(PikaObj *self) {
        MidiPacket* packet = getCppObjPtrInPikaObj<MidiPacket>(self);
        if(!packet) return false;
        return packet->SysExStart();
    }
}