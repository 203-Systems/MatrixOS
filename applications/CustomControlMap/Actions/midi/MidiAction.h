#include "MatrixOS.h"

namespace MidiAction
{
  const char* TAG = "MidiAction";

  constexpr uint32_t signature = StaticHash("midi");

  enum class AnalogSource : uint8_t {
    Momentary = 0,
    Persistent = 1,
    Toggle = 2,
    KeyForce = 3,
    Invalid = 0xFF
  };

  enum class MidiType {
    Note = 0x90,
    ControlChange = 0xB0,
    ProgramChange = 0xC0,
    ChannelPressure = 0xD0,
    PitchBend = 0xE0,
    SysEx = 0xF0,
    RPN = 0xF4,
    NRPN = 0xF5,
    Start = 0xFA,
    Continue = 0xFB,
    Stop = 0xFC,
    Reset = 0xFF,
  };


  struct MidiAction
  {
    MidiType type;
    uint8_t channel;
    
    union 
    {
      uint16_t note;
      uint16_t control;
      uint16_t sysex_length;
    };

    AnalogSource source;
    union
    {
      struct{
        uint16_t begin;
        uint16_t end;
      };
      uint8_t* sysex_data;
    };
  };

// type MidiNoteData = {
//     channel: number
//     note: number,
//     source: AnalogSource
//     begin: number
//     end: number
// }

// type MidiCCData = {
//     channel: number
//     control: number,
//     source: AnalogSource
//     begin: number
//     end: number
// }

// type MidiPCData = {
//     channel: number
//     control: number,
// }

// type MidiCPData = {
//     channel: number
//     source: AnalogSource
//     begin: number
//     end: number
// }

// type MidiPitchbendData = {
//     channel: number
//     source: AnalogSource
//     begin: number
//     end: number
// }


// type MidiRPNData = {
//     channel: number
//     control: number
//     source: AnalogSource
//     begin: number
//     end: number
// }


// type MidiNRPNData = {
//     channel: number
//     control: number
//     source: AnalogSource
//     begin: number
//     end: number
// }

// type MidiSysExData = {
//     sysex: string
// }

  static bool LoadData(cb0r_t actionData, MidiAction* action)
  {
      cb0r_s cbor_data;
      if(!cb0r_get_check_type(actionData, 1, &cbor_data, CB0R_INT))
      {
          MLOGE(TAG, "Failed to get midi type");
          return false;
      }

      action->type = (MidiType)cbor_data.value;

      if(action->type == MidiType::SysEx)
      {
          if(!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_BYTE))
          {
              MLOGE(TAG, "Failed to get midi sysex");
              return false;
          }
          action->sysex_length = cbor_data.length;
          action->sysex_data = cbor_data.start + cbor_data.header;
          action->source = AnalogSource::Persistent;

          return true;
      }

      if(action->type == MidiType::Start || action->type == MidiType::Continue || action->type == MidiType::Stop || action->type == MidiType::Reset)
      {
          action->source = AnalogSource::Persistent;
          return true;
      }

      if(action->type == MidiType::ProgramChange)
      {
        action->source = AnalogSource::Persistent;
      }

      // Get Channel
      if(action->type == MidiType::Note || action->type == MidiType::ControlChange || action->type == MidiType::ProgramChange || action->type == MidiType::ChannelPressure || action->type == MidiType::PitchBend || action->type == MidiType::RPN || action->type == MidiType::NRPN)
      {
          if(!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_INT))
          {
              MLOGE(TAG, "Failed to get midi channel");
              return false;
          }
          action->channel = cbor_data.value;
      }

      // Get Note or Control
      if(action->type == MidiType::Note || action->type == MidiType::ControlChange || action->type == MidiType::ProgramChange || action->type == MidiType::RPN || action->type == MidiType::NRPN)
      {
          if(!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_INT))
          {
            MLOGE(TAG, "Failed to get midi note or control");
            return false;
          }
          action->note = cbor_data.value;
      }

      // Get Analog Source, Begin and End
      if(action->type == MidiType::Note || action->type == MidiType::ControlChange || action->type == MidiType::ChannelPressure || action->type == MidiType::PitchBend || action->type == MidiType::RPN || action->type == MidiType::NRPN)
      {
          if(!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_INT))
          {
              MLOGE(TAG, "Failed to get analog source");
              return false;
          }
          action->source = (AnalogSource)cbor_data.value;

          if(!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_INT))
          {
              MLOGE(TAG, "Failed to get analog begin");
              return false;
          }
          action->begin = cbor_data.value;

          if(!cb0r_next_check_type(actionData, &cbor_data, &cbor_data, CB0R_INT))
          {
              MLOGE(TAG, "Failed to get analog end");
              return false;
          }
          action->end = cbor_data.value;
      }
      return true;
  }

  static bool KeyEvent(UADRuntime* uadRT, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo) {
    if (keyInfo->state != PRESSED && keyInfo->state != RELEASED && keyInfo->state != AFTERTOUCH)
    {
      return false;
    }

    MidiAction data;
    if (!LoadData(actionData, &data))
    {
      MLOGE(TAG, "Failed to load action");
      return false;
    }

    uint16_t output_value = 0;
    switch (data.source)
    {
      case AnalogSource::Invalid:
      {
          break;
      }
      case AnalogSource::Momentary:
      {
        if (keyInfo->state == PRESSED)
        {
          output_value = data.end;
        }
        else if (keyInfo->state == RELEASED)
        {
          output_value = data.begin;
        }
        else
        {
          return false;
        }
        break;
      }
      case AnalogSource::Persistent:
      {
        if (keyInfo->state == PRESSED)
        {
          output_value = data.end;
        }
        else
        {
          return false;
        }
        break;
      }
      case AnalogSource::Toggle:
      {
        ActionInfo groupActionInfo = *actionInfo;
        groupActionInfo.index = 255;
        groupActionInfo.actionType = ActionType::EFFECT;
        if (keyInfo->state == PRESSED)
        {
          uint32_t registerValue;
          if(!uadRT->GetRegister(actionInfo, &registerValue))
          {
            MLOGD(TAG, "Failed to get register. Creating new register");
          }
          MLOGD(TAG, "Register Value: %d", registerValue);
          registerValue ^= 1;
          if(registerValue & 1)
          {
            MLOGD(TAG, "Toggled On");
            output_value = data.end;
          }
          else
          {
            MLOGD(TAG, "Toggled Off");
            output_value = data.begin;
          }
          if(!uadRT->SetRegister(actionInfo, registerValue))
          {
            MLOGE(TAG, "Failed to set register");
          }

          uint32_t groupRegister;
          if(!uadRT->GetRegister(&groupActionInfo, &groupRegister))
          {
            MLOGD(TAG, "Failed to get group register. Creating new group register");
          }
          groupRegister = (groupRegister & 0xFFFFFFF0) + (registerValue & 1); // We use the lower 4 bits for the group register as the LED index for the action driven LED
          if(!uadRT->SetRegister(&groupActionInfo, groupRegister))
          {
            MLOGE(TAG, "Failed to set group register");
          }
        } 
        else 
        {
            return false;
        }
        break;
      }
      case AnalogSource::KeyForce:
      {
        if (keyInfo->state == RELEASED)
        {
          output_value = data.begin;
        }
        else if(keyInfo->velocity == FRACT16_MAX)
        {
          output_value = data.end;
        }
        else
        {
          int32_t range = data.end - data.begin;
          output_value = data.begin + (((uint16_t)keyInfo->velocity * range) >> 16); // I know this is offed by one (velocity max is 0x7FFF but >> 16 is 0x8000) but it's fine
        }
        break;
      }
    }

    switch (data.type)
    {
      case MidiType::Note:
      {
        MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::NoteOn, data.channel, data.note, output_value));
        return true;
      }
      case MidiType::ControlChange:
      {
        MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::ControlChange, data.channel, data.control, output_value));
        return true;
      }
      case MidiType::ProgramChange:
      {
        MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::ProgramChange, data.channel, data.control, 0));
        return true;
      }
      case MidiType::ChannelPressure:
      {
        MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::ChannelPressure, data.channel, output_value, 0));
        return true;
      }
      case MidiType::PitchBend:
      {
          MLOGD(TAG, "Pitch Bend: %d", output_value);
          MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::PitchChange, data.channel, output_value));
          return true;
      }
      case MidiType::SysEx:
      {
        MatrixOS::MIDI::SendSysEx(EMidiPortID::MIDI_PORT_EACH_CLASS, data.sysex_length, data.sysex_data);
        return true;
      }
      case MidiType::RPN:
      {
          MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::ControlChange, data.channel, 101, (data.control >> 7) & 0x7F));
          MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::ControlChange, data.channel, 100, data.control & 0x7F));
          MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::ControlChange, data.channel, 6, (output_value >> 7) & 0x7F));
          MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::ControlChange, data.channel, 38, output_value & 0x7F));
          return true;
      }
      case MidiType::NRPN:
      {
          MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::ControlChange, data.channel, 99, (data.control >> 7) & 0x7F));
          MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::ControlChange, data.channel, 98, data.control & 0x7F));
          MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::ControlChange, data.channel, 6, (output_value >> 7) & 0x7F));
          MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::ControlChange, data.channel, 38, output_value & 0x7F));
          return true;
      }
      case MidiType::Start:
      {
        MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::Start, 0, 0, 0));
        return true;
      }
      case MidiType::Continue:
      {
        MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::Continue, 0, 0, 0));
        return true;
      }
      case MidiType::Stop:
      {
        MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::Stop, 0, 0, 0));
        return true;
      }
      case MidiType::Reset:
      {
        MatrixOS::MIDI::Send(MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, EMidiStatus::Reset, 0, 0, 0));
        return true;
      }
    }
    return false;
  }
};