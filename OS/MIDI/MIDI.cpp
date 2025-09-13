#include "MatrixOS.h"
#include "MIDI.h"
#include "Commands/CommandSpecs.h"

#include "../System/System.h"

namespace MatrixOS::MIDI
{
  void Init(void) {
    // Create the OS MIDI port if it doesn't exist
    if (!osPort) {
      osPort = new MidiPort("MatrixOS", MIDI_PORT_OS, MIDI_QUEUE_SIZE);
    }
    
    // Create the application queue if it doesn't exist
    if (!appQueue) {
      appQueue = xQueueCreate(MIDI_QUEUE_SIZE, sizeof(MidiPacket));
    }
    
    // Create the receive task if it doesn't exist
    if (!receiveTask) {
      xTaskCreate(ReceiveTask, "MIDI_Receive", 2048, NULL, tskIDLE_PRIORITY + 2, &receiveTask);
    }
  }

  bool Get(MidiPacket* midiPacketDest, uint16_t timeout_ms) {
    if (!appQueue) return false;
    return xQueueReceive(appQueue, (void*)midiPacketDest, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
  }

  bool Send(MidiPacket midiPacket, uint16_t targetPort, uint16_t timeout_ms) {
    if (!osPort) return false;
    return osPort->Send(midiPacket, targetPort, timeout_ms);
  }

  void ReceiveTask(void* parameters) {

    static vector<uint8_t> sysExBuffer;
    static uint16_t activeSysExPort = MIDI_PORT_INVALID;
    static SysExState currentSysExState = SysExState::SYSEX_IDLE;

    MidiPacket packet;
    
    while (true) {
      // Blocking get from OS port
      if (osPort && osPort->Get(&packet, portMAX_DELAY)) {
        // Process the packet (moved from old Receive function)
        bool shouldForwardToApp = true;
        
        // Handle SysEx
        if (packet.SysEx())
        {
          // Concurrent SysEx, drop the later one -- TODO: Handle this better
          if (activeSysExPort != MIDI_PORT_INVALID && packet.port != activeSysExPort)
          {
            continue; // Skip this packet
          }

          if(packet.SysExStart())
          {
            sysExBuffer.reserve(12);
            sysExBuffer.clear();
            activeSysExPort = packet.port;
          }
          else if (currentSysExState == SysExState::SYSEX_INVALID)
          {
            continue; // Skip this packet
          } 

          if (currentSysExState != SysExState::SYSEX_RELEASE)
          {
            sysExBuffer.insert(sysExBuffer.end(), packet.data, packet.data + 3);
            currentSysExState = ProcessSysEx(packet.port, sysExBuffer, packet.status == SysExEnd); 

            if(packet.status == SysExEnd)
            {
              activeSysExPort = MIDI_PORT_INVALID;
              currentSysExState = SysExState::SYSEX_IDLE;
            }
            shouldForwardToApp = false; // System handled this packet
          }
        }

        // If current SysEx is the SysexEnd, free up the sysex lock for more sysex.
        if(packet.status == SysExEnd)
        {
          currentSysExState = SysExState::SYSEX_IDLE;
          activeSysExPort = MIDI_PORT_INVALID;
        }

        // Forward to application queue if not handled by system
        if (shouldForwardToApp && appQueue) {
          // Try to send to app queue, drop if full
          xQueueSend(appQueue, &packet, 0);
        }
      }
    }
  }

  bool SendSysEx(uint16_t port, uint16_t length, uint8_t* data, bool includeMeta) {
    if(includeMeta)
    {
      uint8_t header[6] = {MIDIv1_SYSEX_START, SYSEX_MFG_ID[0], SYSEX_MFG_ID[1], SYSEX_MFG_ID[2], SYSEX_FAMILY_ID[0], SYSEX_FAMILY_ID[1]};
      if(!Send(MidiPacket(EMidiStatus::SysExData, 3, header), port, 5))
      { return false; }

      if(!Send(MidiPacket(EMidiStatus::SysExData, 3, header + 3), port, 5))
      { return false; }
    }

    for (uint8_t index = 0; index < length - 3 - !includeMeta; index += 3)
    {
      if(!Send(MidiPacket(EMidiStatus::SysExData, 3, data + index), port, 5))
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
      
      if(!Send(MidiPacket(EMidiStatus::SysExEnd, length % 3 + 1, footer), port, 5))
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
      
      if(!Send(MidiPacket(EMidiStatus::SysExEnd, end_frame_length, footer), port, 5))
      { return false; }
    }
    return true;
  }


  SysExState ProcessSysEx(uint16_t port, vector<uint8_t>& sysExBuffer, bool complete) { 
    if(sysExBuffer[0] != MIDIv1_SYSEX_START) {return SysExState::SYSEX_INVALID; }

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
      else if(sysExBuffer[1] == MATRIXOS_SYSEX_REQUEST) //Matrix OS specific sysex
      {
        HandleMatrixOSSysEx(port, sysExBuffer);
      }
      return SysExState::SYSEX_COMPLETE;
    }
    else if(
      sysExBuffer.size() >= 6 &&
      sysExBuffer[1] == SYSEX_MFG_ID[0] && sysExBuffer[2] == SYSEX_MFG_ID[1] && sysExBuffer[3] == SYSEX_MFG_ID[2] && 
      sysExBuffer[4] == SYSEX_FAMILY_ID[0] && sysExBuffer[5] == SYSEX_FAMILY_ID[1])
    {
      return SysExState::SYSEX_RELEASE;
    }
    return SysExState::SYSEX_PENDING;
  }

  void HandleMatrixOSSysEx(uint16_t port, vector<uint8_t>& sysExBuffer) {
    switch(sysExBuffer[2])
    {
      case MATRIXOS_COMMAND_GET_OS_VERSION:
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
          MIDIv1_SYSEX_START, MATRIXOS_SYSEX_RESPONSE, MATRIXOS_COMMAND_GET_OS_VERSION, MATRIXOS_MAJOR_VER, MATRIXOS_MINOR_VER, MATRIXOS_PATCH_VER, osReleaseVersion, MIDIv1_SYSEX_END};
        SendSysEx(port, sizeof(reply), reply, false);
      }
      break;
      case MATRIXOS_COMMAND_GET_APP_ID:
      {
        uint8_t reply[] = {
          MIDIv1_SYSEX_START, MATRIXOS_SYSEX_RESPONSE, MATRIXOS_COMMAND_GET_APP_ID, (uint8_t)((SYS::active_app_id >> 25) & 0x7F), (uint8_t)((SYS::active_app_id >> 18) & 0x7F), (uint8_t)((SYS::active_app_id >> 11) & 0x7F), (uint8_t)((SYS::active_app_id >> 4) & 0x7F), (uint8_t)((SYS::active_app_id << 3) & 0x7F), MIDIv1_SYSEX_END};
        SendSysEx(port, sizeof(reply), reply, false);
      }
      break;
      case MATRIXOS_COMMAND_ENTER_APP_VIA_ID:
      {
        if(sysExBuffer.size() != 9)
        {
          break;
        }
        uint32_t app_id = ((uint32_t)sysExBuffer[4] << 25) + ((uint32_t)sysExBuffer[5] << 18) + ((uint32_t)sysExBuffer[6] << 11) + ((uint32_t)sysExBuffer[7] << 4) + ((uint32_t)sysExBuffer[8] >> 3);
        SYS::ExecuteAPP(app_id);
        break;
      }
      case MATRIXOS_COMMAND_QUIT_APP:
      {
        SYS::ExitAPP();
        break;
      }
      case MATRIXOS_COMMAND_BOOTLOADER:
      {
        SYS::Bootloader();
        break;
      }
      case MATRIXOS_COMMAND_REBOOT:
      {
        SYS::Reboot();
        break;
      }
      // case MATRIXOS_COMMAND_SLEEP:
      // {
      //   SYS::Sleep();
      //   break;
      // }
      default:
      {
        MLOGE("MIDI", "Unknown MatrixOS SysEx Command: %d", sysExBuffer[1]);
      }
    }
  }
}