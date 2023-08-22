#include "MatrixOS.h"

namespace MidiAction
{
  const char* TAG = "MidiAction";

  constexpr uint32_t signature = StaticHash("midi");

  // struct MidiAction
  // {
  //   uint8_t data0;
  //   uint8_t data1;
  //   uint8_t data2;
  // };

  // static bool LoadData(cb0r_t actionData, MidiAction* action)
  // {
  //     cb0r_s cbor_data;
  //     if(!cb0r_get(actionData, 1, &cbor_data) || cbor_data.type != CB0R_INT)
  //     {
  //         MLOGE(TAG, "Failed to get action data 0");
  //         return false;
  //     }
  //     action->data0 = cbor_data.value;

  //     if(!cb0r_get(actionData, 2, &cbor_data) || cbor_data.type != CB0R_INT)
  //     {
  //         MLOGE(TAG, "Failed to get action data 1");
  //         return false;
  //     }
  //     action->data1 = cbor_data.value;

  //     if(!cb0r_get(actionData, 3, &cbor_data) || cbor_data.type != CB0R_INT)
  //     {
  //         MLOGE(TAG, "Failed to get action data 2");
  //         return false;
  //     }
  //     action->data2 = cbor_data.value;
  //     return true;
  // }

  static uint8_t GetData(cb0r_t actionData, uint16_t index) {
    cb0r_s cbor_data;
    if (!cb0r_get(actionData, 1 + index, &cbor_data) || cbor_data.type != CB0R_INT)
    {
      MLOGE(TAG, "Failed to get action data 0");
      return 0;
    }

    return (uint8_t)cbor_data.value;
  }

  static bool KeyEvent(UAD* UAD, ActionInfo* actionInfo, cb0r_t actionData, KeyInfo* keyInfo) {
    MLOGV(TAG, "KeyEvent - Data Length: %d", actionData->length);
    if (keyInfo->state != PRESSED && keyInfo->state != RELEASED && keyInfo->state != AFTERTOUCH)
    {
      MLOGV(TAG, "Not useful");
      return false;
    }

    uint8_t signature = GetData(actionData, 0);

    // MLOGV(TAG, "Data: %d, %d, %d", data.data0, data.data1, data.data2);
    switch ((signature & 0xF0))
    {
        case EMidiStatus::NoteOn:
        {        
            uint8_t note = GetData(actionData, 1);
            uint8_t velocity = GetData(actionData, 2); 

            if (keyInfo->state == PRESSED)  // Velocity sensing is disabled && key press action
            {
                MatrixOS::MIDI::Send(MidiPacket(0, EMidiStatus::NoteOn, signature & 0x0F, note & 0x7F, velocity));
                return true;
            }
            else if (keyInfo->state == RELEASED)
            {
                MatrixOS::MIDI::Send(MidiPacket(0, EMidiStatus::NoteOn, signature & 0x0F, note & 0x7F, 0));
                return true;
            }
        }
        case EMidiStatus::AfterTouch:
        {
            uint8_t note = GetData(actionData, 1);
            if (keyInfo->state == AfterTouch)
            {
                MatrixOS::MIDI::Send(MidiPacket(0, EMidiStatus::AfterTouch, signature & 0x0F, note & 0x7F, keyInfo->velocity.to7bits()));
                return true;
            }
            else if (keyInfo->state == PRESSED)
            {
                MatrixOS::MIDI::Send(MidiPacket(0, EMidiStatus::AfterTouch, signature & 0x0F, note & 0x7F, keyInfo->velocity.to7bits()));
                return true;
            }
            else if(keyInfo->state == RELEASED)
            {
                MatrixOS::MIDI::Send(MidiPacket(0, EMidiStatus::NoteOn, signature & 0x0F, note & 0x7F, 0));
                return true;
            }
        }
        case EMidiStatus::ControlChange:
        {
            if (keyInfo->state == PRESSED)  // Velocity sensing is disabled && key press action
            {   
                uint8_t control = GetData(actionData, 1);
                uint8_t value = GetData(actionData, 2);
                MatrixOS::MIDI::Send(MidiPacket(0, EMidiStatus::ControlChange, signature & 0x0F, control, value));
                return true;
            }
        }
    }

    return false;
  }
};