#include "MatrixOS.h"
#include <map>

// TODO Put this in device layer
const uint8_t SYSEX_MFG_ID[3] = {0x00, 0x02, 0x03};
const uint8_t SYSEX_FAMILY_ID[3] = {0x4D, 0x58}; // {'M', 'X'}
const uint8_t SYSEX_MODEL_ID[3] = {0x11, 0x01};

namespace MatrixOS::MIDI
{
  QueueHandle_t midi_queue;
  std::map<uint16_t, MidiPort*> midiPortMap;

  void Init(void) {
    midi_queue = xQueueCreate(MIDI_QUEUE_SIZE, sizeof(MidiPacket));
  }

  bool Get(MidiPacket* midiPacketDest, uint16_t timeout_ms) {
    return xQueueReceive(midi_queue, midiPacketDest, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
  }

  bool Send(MidiPacket midiPacket, uint16_t timeout_ms) {
    if (midiPacket.port == MIDI_PORT_ALL_CLASS)
    {
      uint16_t targetClass = MIDI_PORT_USB;
      bool send = false;
      for (std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.begin(); port != midiPortMap.end(); ++port)
      {
        if (port->first >= MIDI_PORT_DEVICE_CUSTOM + 0x100)
        { return send; }
        if (port->first >= targetClass)
        {
          send |= port->second->Recive(midiPacket, timeout_ms);
          targetClass = (port->first / 0x100 + 1) * 0x100;
        }
      }
    }
    else
    {
      std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.find(midiPacket.port);
      if (port != midiPortMap.end())
      { return port->second->Recive(midiPacket, timeout_ms); }
    }
    return false;
  }

  bool SendSysEx(uint16_t port, uint16_t length, uint8_t* data, bool includeMeta)
  {
    if(includeMeta)
    {
      uint8_t header[6] = {MIDIv1_SYSEX_START, SYSEX_MFG_ID[0], SYSEX_MFG_ID[1], SYSEX_MFG_ID[2], SYSEX_FAMILY_ID[0], SYSEX_FAMILY_ID[1]};
      if(!Send(MidiPacket(port, SysExData, 3, header), 5))
      { return false; }

      if(!Send(MidiPacket(port, SysExData, 3, header + 3), 5))
      { return false; }
    }

    for (uint8_t index = 0; index < length - 3 - !includeMeta; index += 3)
    {
      if(!Send(MidiPacket(port, SysExData, 3, data + index), 5))
      { return false; }
    }

    // Send End
    if(includeMeta)
    { 
      uint16_t start_ptr = length - length % 3;
      uint8_t footer[3] = {0, 0, 0};
      for (uint16_t i = start_ptr; i < length; i++)
      {
        footer[i] = data[start_ptr + i];
      }
      footer[length % 3] = MIDIv1_SYSEX_END;
      
      if(!Send(MidiPacket(port, SysExEnd, length % 3 + 1, footer), 5))
      { return false;}
     }
    else
    {
      uint8_t end_frame_length = ((length - 1) % 3 + 1);
      uint16_t start_ptr = length - end_frame_length;
      uint8_t footer[3] = {0, 0, 0};
      for (uint16_t i = 0; i < end_frame_length; i++)
      {
        footer[i] = data[start_ptr + i];
      }
      
      if(!Send(MidiPacket(port, SysExEnd, end_frame_length, footer), 5))
      { return false; }
    }
    return true;
  }

  bool RegisterMidiPort(uint16_t port_id, MidiPort* midiPort) {
    if (port_id < 0x100)
      return false;

    if (midiPortMap.find(port_id) == midiPortMap.end())
    {
      midiPortMap[port_id] = midiPort;
      return true;
    }
    return false;
  }

  void UnregisterMidiPort(uint16_t port_id) {
    midiPortMap.erase(port_id);
  }

  enum SysExState : uint8_t {IDLE, PENDING, COMPLETE, RELEASE, INVAILD}; 
  // IDLE     = No sysex in yet
  // PENDING  = Not sure who's Sysex this is or pending for system to parse
  // COMPLETE = It is a system sysex and we have parsed it
  // RELEASE  = This is an appliction sysex, release to application
  // INVAILD  = This is our sysex, don' release, just destory it

  SysExState ProcessSysEx(uint16_t port, vector<uint8_t> sysExBuffer, bool complete) {

    if(sysExBuffer[0] != MIDIv1_SYSEX_START) {return SysExState::INVAILD; }

    if(complete)
    {
      if(sysExBuffer[1] == MIDIv1_UNIVERSAL_NON_REALTIME_ID) //Non real time messages
      {
       if(sysExBuffer[3] == USYSEX_GENERAL_INFO && sysExBuffer[4] == USYSEX_GI_ID_REQUEST) //General Info - Identity Request I should be check channel here but it doesn't matter
       {
        #if MATRIXOS_BUILD_VER == 0 // Release Version
        uint8_t osReleaseVersion = 0;
        #elif (MATRIXOS_BUILD_VER == 4) // Nighty Version
        uint8_t osReleaseVersion = 0x31; // 0b0011111 - Shares the same first two bit as release version but the last 5 bits are set
        #elif(MATRIXOS_RELEASE_VER < 32) // Speial Release Version
        uint8_t osReleaseVersion = (MATRIXOS_BUILD_VER << 5) + MATRIXOS_RELEASE_VER;
        #else
        uint8_t osReleaseVersion = (MATRIXOS_BUILD_VER << 5) + 0x1F;
        #endif

         uint8_t reply[] = {
          MIDIv1_SYSEX_START, MIDIv1_UNIVERSAL_NON_REALTIME_ID, USYSEX_ALL_CHANNELS, USYSEX_GENERAL_INFO, USYSEX_GI_ID_RESPONSE, 
          SYSEX_MFG_ID[0], SYSEX_MFG_ID[1], SYSEX_MFG_ID[2], 
          SYSEX_FAMILY_ID[0], SYSEX_FAMILY_ID[1], 
          SYSEX_MODEL_ID[0], SYSEX_MODEL_ID[1],
          MATRIXOS_MAJOR_VER, MATRIXOS_MINOR_VER, MATRIXOS_PATCH_VER, osReleaseVersion,
          MIDIv1_SYSEX_END};

          SendSysEx(port, sizeof(reply), reply, false);
       }
      }
      return SysExState::COMPLETE;
    }
    else if(
      sysExBuffer.size() >= 6 &&
      sysExBuffer[1] == SYSEX_MFG_ID[0] && sysExBuffer[2] == SYSEX_MFG_ID[1] && sysExBuffer[3] == SYSEX_MFG_ID[2] && 
      sysExBuffer[4] == SYSEX_FAMILY_ID[0] && sysExBuffer[5] == SYSEX_FAMILY_ID[1])
    {
      return SysExState::RELEASE;
    }
    return SysExState::PENDING;
  }

  vector<uint8_t> sysExBuffer;
  uint16_t activeSysExPort = MIDI_PORT_INVAILD;
  SysExState currentSysExState = SysExState::IDLE;
  bool Recive(MidiPacket midiPacket, uint32_t timeout_ms) {
    // Handle SysEx
    if (midiPacket.SysEx())
    {
      // Concurrent SysEx, drop the later one -- TODO: Handle this better
      if (activeSysExPort != MIDI_PORT_INVAILD && midiPacket.port != activeSysExPort)
      {
        return true; //Signal that we have handled this packet
      }

      if(midiPacket.SysExStart())
      {
        sysExBuffer.reserve(12);
        sysExBuffer.clear();
        activeSysExPort = midiPacket.port;
      }
      else if (currentSysExState == SysExState::INVAILD)
      {
        return true; //Signal that we have handled this packet
      } 

      if (currentSysExState != SysExState::RELEASE)
      {
        sysExBuffer.insert(sysExBuffer.end(), midiPacket.data, midiPacket.data + 3);
        currentSysExState = ProcessSysEx(midiPacket.port, sysExBuffer, midiPacket.status == SysExEnd); 

        if(midiPacket.status == SysExEnd)
        {
          activeSysExPort = MIDI_PORT_INVAILD;
          currentSysExState = SysExState::IDLE;
        }
        return true; //Signal that we have handled this packet
      }
    }

    if (uxQueueSpacesAvailable(midi_queue) == 0)
    {
      // TODO: Drop first element
      return false;
    }

    xQueueSend(midi_queue, &midiPacket, pdMS_TO_TICKS(timeout_ms));

    // If current SysEx is the SysexEnd, free up the sysex lock for more sysex.
    if(midiPacket.status == SysExEnd)
    {
      currentSysExState = SysExState::IDLE;
      activeSysExPort = MIDI_PORT_INVAILD;
    }
    return true;  //Signal that we have handled this packet
  }
}