#include "MatrixOS.h"
#include "MIDI.h"
#include "Commands/CommandSpecs.h"

#include "../System/System.h"

namespace MatrixOS::MIDI
{
static constexpr size_t MAX_SYSTEM_SYSEX_SIZE = 1024;
static constexpr uint32_t SYSEX_INACTIVITY_TIMEOUT_MS = 1000;
static uint32_t droppedAppMidiPackets = 0;
static uint32_t droppedOversizedSysExMessages = 0;
static uint32_t timedOutSysExSessions = 0;

static void LogDroppedAppMidiPacket() {
  droppedAppMidiPackets++;
  if (droppedAppMidiPackets == 1 || (droppedAppMidiPackets % 32) == 0)
  {
    MLOGW("MIDI", "Application MIDI queue overflow, dropped %lu packet(s)", droppedAppMidiPackets);
  }
}

static void LogDroppedOversizedSysEx() {
  droppedOversizedSysExMessages++;
  if (droppedOversizedSysExMessages == 1 || (droppedOversizedSysExMessages % 8) == 0)
  {
    MLOGW("MIDI", "Dropped oversized system SysEx message(s): %lu", droppedOversizedSysExMessages);
  }
}

static void LogTimedOutSysExSession() {
  timedOutSysExSessions++;
  if (timedOutSysExSessions == 1 || (timedOutSysExSessions % 8) == 0)
  {
    MLOGW("MIDI", "Dropped timed out SysEx session(s): %lu", timedOutSysExSessions);
  }
}

static void ResetActiveSysExSession(vector<uint8_t>& sysExBuffer, uint16_t& activeSysExPort, SysExState& currentSysExState,
                                    uint32_t& lastSysExActivityMs) {
  sysExBuffer.clear();
  activeSysExPort = MIDI_PORT_INVALID;
  currentSysExState = SysExState::SYSEX_IDLE;
  lastSysExActivityMs = 0;
}

void Init(void) {
  // Create the OS MIDI port if it doesn't exist
  if (!osPort)
  {
    osPort = new MidiPort("MatrixOS", MIDI_PORT_OS, MIDI_QUEUE_SIZE);
  }

  // Create the application queue if it doesn't exist
  if (!appQueue)
  {
    appQueue = xQueueCreate(MIDI_QUEUE_SIZE, sizeof(MidiPacket));
  }

  // Create the receive task if it doesn't exist
  // Only create task if scheduler is already running (ESP32) or will be started later
  if (!receiveTask && xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
    xTaskCreate(ReceiveTask, "MIDI_Receive", 2048, NULL, tskIDLE_PRIORITY + 2, &receiveTask);
  }
}

bool Get(MidiPacket* midiPacketDest, uint16_t timeoutMs) {
  if (!appQueue)
    return false;
  return xQueueReceive(appQueue, (void*)midiPacketDest, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

bool Send(MidiPacket midiPacket, uint16_t targetPort, uint16_t timeoutMs) {
  if (!osPort)
    return false;
  return osPort->Send(midiPacket, targetPort, timeoutMs);
}

void ReceiveTask(void* parameters) {

  static vector<uint8_t> sysExBuffer;
  static uint16_t activeSysExPort = MIDI_PORT_INVALID;
  static SysExState currentSysExState = SysExState::SYSEX_IDLE;
  static uint32_t lastSysExActivityMs = 0;

  MidiPacket packet;

  while (true)
  {
    // Blocking get from OS port
    if (osPort && osPort->Get(&packet, portMAX_DELAY))
    {
      // Process the packet (moved from old Receive function)
      bool shouldForwardToApp = true;
      uint32_t nowMs = SYS::Millis();

      if (activeSysExPort != MIDI_PORT_INVALID && (uint32_t)(nowMs - lastSysExActivityMs) > SYSEX_INACTIVITY_TIMEOUT_MS)
      {
        ResetActiveSysExSession(sysExBuffer, activeSysExPort, currentSysExState, lastSysExActivityMs);
        LogTimedOutSysExSession();
      }

      // Handle SysEx
      if (packet.SysEx())
      {
        if (packet.SysExStart())
        {
          if (activeSysExPort != MIDI_PORT_INVALID && packet.port != activeSysExPort)
          {
            continue; // Skip concurrent SysEx from another port
          }

          sysExBuffer.reserve(12);
          sysExBuffer.clear();
          activeSysExPort = packet.port;
          currentSysExState = SysExState::SYSEX_PENDING;
          lastSysExActivityMs = nowMs;
        }
        else
        {
          if (activeSysExPort != packet.port)
          {
            continue; // Skip fragments for inactive or concurrent ports
          }

          lastSysExActivityMs = nowMs;

          if (currentSysExState == SysExState::SYSEX_INVALID)
          {
            if (packet.status == SysExEnd)
            {
              ResetActiveSysExSession(sysExBuffer, activeSysExPort, currentSysExState, lastSysExActivityMs);
            }
            continue; // Skip this packet
          }
        }

        if (currentSysExState != SysExState::SYSEX_RELEASE)
        {
          uint8_t packetLength = packet.Length();
          if (sysExBuffer.size() + packetLength > MAX_SYSTEM_SYSEX_SIZE)
          {
            sysExBuffer.clear();
            LogDroppedOversizedSysEx();
            if (packet.status == SysExEnd)
            {
              ResetActiveSysExSession(sysExBuffer, activeSysExPort, currentSysExState, lastSysExActivityMs);
            }
            else
            {
              currentSysExState = SysExState::SYSEX_INVALID;
            }
            continue;
          }

          sysExBuffer.insert(sysExBuffer.end(), packet.data, packet.data + packetLength);
          currentSysExState = ProcessSysEx(packet.port, sysExBuffer, packet.status == SysExEnd);

          if (packet.status == SysExEnd)
          {
            ResetActiveSysExSession(sysExBuffer, activeSysExPort, currentSysExState, lastSysExActivityMs);
          }
          shouldForwardToApp = false; // System handled this packet
        }
      }

      // If current SysEx is the SysexEnd, free up the sysex lock for more sysex.
      if (packet.status == SysExEnd && packet.port == activeSysExPort)
      {
        ResetActiveSysExSession(sysExBuffer, activeSysExPort, currentSysExState, lastSysExActivityMs);
      }

      // Forward to application queue if not handled by system
      if (shouldForwardToApp && appQueue)
      {
        // Try to send to app queue, drop if full
        if (xQueueSend(appQueue, &packet, 0) != pdTRUE)
        {
          LogDroppedAppMidiPacket();
        }
      }
    }
  }
}

bool SendSysEx(uint16_t port, uint16_t length, uint8_t* data, bool includeMeta) {
  if (data == nullptr || length == 0)
  {
    return false;
  }

  if (includeMeta)
  {
    uint8_t header[6] = {MIDIv1_SYSEX_START, SYSEX_MFG_ID[0], SYSEX_MFG_ID[1], SYSEX_MFG_ID[2], SYSEX_FAMILY_ID[0], SYSEX_FAMILY_ID[1]};
    if (!Send(MidiPacket(EMidiStatus::SysExData, header[0], header[1], header[2]), port, 5))
    {
      return false;
    }

    if (!Send(MidiPacket(EMidiStatus::SysExData, header[3], header[4], header[5]), port, 5))
    {
      return false;
    }
  }

  uint8_t endFrameLength = includeMeta ? (length % 3) : ((length - 1) % 3 + 1);
  uint16_t dataPacketLength = length - endFrameLength;
  for (uint16_t index = 0; index < dataPacketLength; index += 3)
  {
    if (!Send(MidiPacket(EMidiStatus::SysExData, data[index], data[index + 1], data[index + 2]), port, 5))
    {
      return false;
    }
  }

  // Send End
  if (includeMeta)
  {
    uint8_t footer[3] = {0, 0, 0};
    for (uint8_t i = 0; i < endFrameLength; i++)
    {
      footer[i] = data[dataPacketLength + i];
    }
    footer[endFrameLength] = MIDIv1_SYSEX_END;

    if (!Send(MidiPacket(EMidiStatus::SysExEnd, footer[0], footer[1], footer[2]), port, 5))
    {
      return false;
    }
  }
  else
  {
    uint8_t footer[3] = {0, 0, 0};
    for (uint16_t i = 0; i < endFrameLength; i++)
    {
      footer[i] = data[dataPacketLength + i];
    }

    if (!Send(MidiPacket(EMidiStatus::SysExEnd, footer[0], footer[1], footer[2]), port, 5))
    {
      return false;
    }
  }
  return true;
}

SysExState ProcessSysEx(uint16_t port, vector<uint8_t>& sysExBuffer, bool complete) {
  if (sysExBuffer.empty())
  {
    return SysExState::SYSEX_INVALID;
  }

  if (sysExBuffer[0] != MIDIv1_SYSEX_START)
  {
    return SysExState::SYSEX_INVALID;
  }

  if (complete)
  {
    if (sysExBuffer.size() < 2)
    {
      return SysExState::SYSEX_INVALID;
    }

    if (sysExBuffer[1] == MIDIv1_UNIVERSAL_NON_REALTIME_ID) // Non real time messages
    {
      if (sysExBuffer.size() >= 5 && sysExBuffer[3] == USYSEX_GENERAL_INFO &&
          sysExBuffer[4] == USYSEX_GI_ID_REQUEST) // General Info - Identity Request I should be check channel here but it doesn't matter
      {
#if MATRIXOS_BUILD_VER == 0 // Release Version
        uint8_t osReleaseVersion = 0;
#elif (MATRIXOS_BUILD_VER == 4)   // Nighty Version
        uint8_t osReleaseVersion = 0x31; // 0b0011111 - Shares the same first two bit as release version but the last 5 bits are set
#elif (MATRIXOS_RELEASE_VER < 32) // Special Release Version
        uint8_t osReleaseVersion = (MATRIXOS_BUILD_VER << 5) + MATRIXOS_RELEASE_VER;
#else
        uint8_t osReleaseVersion = (MATRIXOS_BUILD_VER << 5) + 0x1F;
#endif

        uint8_t reply[] = {MIDIv1_SYSEX_START,    MIDIv1_UNIVERSAL_NON_REALTIME_ID,
                           USYSEX_ALL_CHANNELS,   USYSEX_GENERAL_INFO,
                           USYSEX_GI_ID_RESPONSE, SYSEX_MFG_ID[0],
                           SYSEX_MFG_ID[1],       SYSEX_MFG_ID[2],
                           SYSEX_FAMILY_ID[0],    SYSEX_FAMILY_ID[1],
                           SYSEX_MODEL_ID[0],     SYSEX_MODEL_ID[1],
                           MATRIXOS_MAJOR_VER,    MATRIXOS_MINOR_VER,
                           MATRIXOS_PATCH_VER,    osReleaseVersion,
                           MIDIv1_SYSEX_END};

        SendSysEx(port, sizeof(reply), reply, false);
      }
    }
    else if (sysExBuffer[1] == MATRIXOS_SYSEX_REQUEST) // Matrix OS specific sysex
    {
      if (sysExBuffer.size() >= 3)
      {
        HandleMatrixOSSysEx(port, sysExBuffer);
      }
    }
    return SysExState::SYSEX_COMPLETE;
  }
  else if (sysExBuffer.size() >= 6 && sysExBuffer[1] == SYSEX_MFG_ID[0] && sysExBuffer[2] == SYSEX_MFG_ID[1] &&
           sysExBuffer[3] == SYSEX_MFG_ID[2] && sysExBuffer[4] == SYSEX_FAMILY_ID[0] && sysExBuffer[5] == SYSEX_FAMILY_ID[1])
  {
    return SysExState::SYSEX_RELEASE;
  }
  return SysExState::SYSEX_PENDING;
}

void HandleMatrixOSSysEx(uint16_t port, vector<uint8_t>& sysExBuffer) {
  if (sysExBuffer.size() < 3)
  {
    return;
  }

  switch (sysExBuffer[2])
  {
  case MATRIXOS_COMMAND_GET_OS_VERSION: {
#if MATRIXOS_BUILD_VER == 0 // Release Version
    uint8_t osReleaseVersion = 0;
#elif (MATRIXOS_BUILD_VER == 4)   // Nighty Version
    uint8_t osReleaseVersion = 0x31; // 0b0011111 - Shares the same first two bit as release version but the last 5 bits are set
#elif (MATRIXOS_RELEASE_VER < 32) // Special Release Version
    uint8_t osReleaseVersion = (MATRIXOS_BUILD_VER << 5) + MATRIXOS_RELEASE_VER;
#else
    uint8_t osReleaseVersion = (MATRIXOS_BUILD_VER << 5) + 0x1F;
#endif
    uint8_t reply[] = {MIDIv1_SYSEX_START, MATRIXOS_SYSEX_RESPONSE, MATRIXOS_COMMAND_GET_OS_VERSION,
                       MATRIXOS_MAJOR_VER, MATRIXOS_MINOR_VER,      MATRIXOS_PATCH_VER,
                       osReleaseVersion,   MIDIv1_SYSEX_END};
    SendSysEx(port, sizeof(reply), reply, false);
  }
  break;
  case MATRIXOS_COMMAND_GET_APP_ID: {
    uint8_t reply[] = {MIDIv1_SYSEX_START,
                       MATRIXOS_SYSEX_RESPONSE,
                       MATRIXOS_COMMAND_GET_APP_ID,
                       (uint8_t)((SYS::activeAppId >> 25) & 0x7F),
                       (uint8_t)((SYS::activeAppId >> 18) & 0x7F),
                       (uint8_t)((SYS::activeAppId >> 11) & 0x7F),
                       (uint8_t)((SYS::activeAppId >> 4) & 0x7F),
                       (uint8_t)((SYS::activeAppId << 3) & 0x7F),
                       MIDIv1_SYSEX_END};
    SendSysEx(port, sizeof(reply), reply, false);
  }
  break;
  case MATRIXOS_COMMAND_ENTER_APP_VIA_ID: {
    if (sysExBuffer.size() != 9)
    {
      break;
    }
    uint32_t appId = ((uint32_t)sysExBuffer[3] << 25);
    appId += ((uint32_t)sysExBuffer[4] << 18);
    appId += ((uint32_t)sysExBuffer[5] << 11);
    appId += ((uint32_t)sysExBuffer[6] << 4);
    appId += ((uint32_t)sysExBuffer[7] >> 3);
    SYS::ExecuteAPP(appId);
    break;
  }
  case MATRIXOS_COMMAND_QUIT_APP: {
    SYS::ExitAPP();
    break;
  }
  case MATRIXOS_COMMAND_BOOTLOADER: {
    SYS::Bootloader();
    break;
  }
  case MATRIXOS_COMMAND_REBOOT: {
    SYS::Reboot();
    break;
  }
  // case MATRIXOS_COMMAND_SLEEP:
  // {
  //   SYS::Sleep();
  //   break;
  // }
  default: {
    MLOGE("MIDI", "Unknown MatrixOS SysEx Command: %d", sysExBuffer[2]);
  }
  }
}
} // namespace MatrixOS::MIDI
