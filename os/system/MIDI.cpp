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
    if (midiPacket.port ==  MIDI_PORT_FIRST_OF_ALL_EXT_CLASS)
    {
      // Start with the USB class
      uint16_t targetClass = MIDI_PORT_USB;
      // Temp variable to check if the packet was sent to the class
      bool send = false;
      // Iterate through all the ports in the midiPortMap
      for (std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.begin(); port != midiPortMap.end(); ++port)
      {
        // If port class exceeds the ext class. That means we have done our job
        if (port->first >= 0x8000) // 0x8000 means the port is internal ports (between system components or embedded hw)
        { return send; }
        // If the port is of higher class than the previous targetClass, send the packet to that port
        if (port->first >= targetClass)
        {
          send |= port->second->Receive(midiPacket, timeout_ms);
          targetClass = (port->first / 0x100 + 1) * 0x100;
        }
      }
    }
    else
    {
      // Find the port in the midiPortMap
      std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.find(midiPacket.port);
      // If the port is found, send the packet to that port
      if (port != midiPortMap.end())
      { return port->second->Receive(midiPacket, timeout_ms); }
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

  uint16_t OpenMidiPort(uint16_t port_id, MidiPort* midiPort) {
    // Invalid section (system macro)
    // MLOGV("MIDI", "Opening port: %d", port_id);
    if (port_id < 0x100 || port_id == MIDI_PORT_INVALID)
    {
      // MLOGE("MIDI", "Opening invalid port: %d", port_id);
      return MIDI_PORT_INVALID;
    }

    // Dynamic port allocation
    else if ((port_id & 0xFF) == 0)
    {
      // Find next available port
      uint16_t port_class = port_id & 0xFF00;
      port_id = port_class + 1;
      // MLOGV("MIDI", "Dynamic port allocation under class: 0x%04x", port_class);
      for(std::map<uint16_t, MidiPort*>::iterator port = midiPortMap.begin(); port != midiPortMap.end(); ++port)
      {
        if(port->first == port_id)
        {
          port_id++;
        }
        else if (port->first > (port_id | 0xFF))
        {
          break;
        }
      }
      if (port_id > port_class && port_id < (port_class + 0x80))
      {
        midiPortMap[port_id] = midiPort;
        // MLOGV("MIDI", "Dynamic port allocated: %d", port_id);
        return port_id;
      }
    }

    // Static port allocation - Check if port range is valid and not already taken
    else if ((port_id & 0xFF) < 0x80)
    { 
      if (midiPortMap.find(port_id) == midiPortMap.end())
      {
        midiPortMap[port_id] = midiPort;
        // MLOGV("MIDI", "Static port allocated: %d", port_id);
        return port_id;
      }
    }
    // MLOGE("MIDI", "Failed to open port: %d", port_id);
    return MIDI_PORT_INVALID;
  }

  bool CloseMidiPort(uint16_t port_id) {
    return midiPortMap.erase(port_id) > 0;
  }

  enum SysExState : uint8_t {IDLE, PENDING, COMPLETE, RELEASE, INVALID}; 
  // IDLE     = No sysex in yet
  // PENDING  = Not sure who's Sysex this is or pending for system to parse
  // COMPLETE = It is a system sysex and we have parsed it
  // RELEASE  = This is an application sysex, release to application
  // INVALID  = This is our sysex, don' release, just Destroy it

  SysExState ProcessSysEx(uint16_t port, vector<uint8_t> sysExBuffer, bool complete) {

    if(sysExBuffer[0] != MIDIv1_SYSEX_START) {return SysExState::INVALID; }

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
        #elif(MATRIXOS_RELEASE_VER < 32) // Special Release Version
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
  uint16_t activeSysExPort = MIDI_PORT_INVALID;
  SysExState currentSysExState = SysExState::IDLE;
  bool Receive(MidiPacket midiPacket, uint32_t timeout_ms) {
    // Handle SysEx
    if (midiPacket.SysEx())
    {
      // Concurrent SysEx, drop the later one -- TODO: Handle this better
      if (activeSysExPort != MIDI_PORT_INVALID && midiPacket.port != activeSysExPort)
      {
        return true; //Signal that we have handled this packet
      }

      if(midiPacket.SysExStart())
      {
        sysExBuffer.reserve(12);
        sysExBuffer.clear();
        activeSysExPort = midiPacket.port;
      }
      else if (currentSysExState == SysExState::INVALID)
      {
        return true; //Signal that we have handled this packet
      } 

      if (currentSysExState != SysExState::RELEASE)
      {
        sysExBuffer.insert(sysExBuffer.end(), midiPacket.data, midiPacket.data + 3);
        currentSysExState = ProcessSysEx(midiPacket.port, sysExBuffer, midiPacket.status == SysExEnd); 

        if(midiPacket.status == SysExEnd)
        {
          activeSysExPort = MIDI_PORT_INVALID;
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
      activeSysExPort = MIDI_PORT_INVALID;
    }
    return true;  //Signal that we have handled this packet
  }
}