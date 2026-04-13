#include "HostIO.h"

#include "MIDI/MIDI.h"
#include "USB/USB.h"
#include "Commands/CommandSpecs.h"
#include "Framework/Midi/MidiPort.h"

#include "System/System.h"

namespace MatrixOS::MIDI
{
static constexpr size_t MAX_SYSTEM_SYSEX_SIZE = 1024;
static uint32_t droppedAppMidiPackets = 0;
static uint32_t droppedOversizedSysExMessages = 0;

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

void Init(void) {
  if (!osPort)
  {
    osPort = new MidiPort("MatrixOS", MIDI_PORT_OS, MIDI_QUEUE_SIZE);
  }

  if (!appQueue)
  {
    appQueue = xQueueCreate(MIDI_QUEUE_SIZE, sizeof(MidiPacket));
  }

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
  bool result = osPort->Send(midiPacket, targetPort, timeoutMs);
  if (result)
  {
    MystrixSIL::HostIO::TapMidi(1, midiPacket.port, targetPort, midiPacket);
  }
  return result;
}

void ReceiveTask(void* parameters) {
  static vector<uint8_t> sysExBuffer;
  static uint16_t activeSysExPort = MIDI_PORT_INVALID;
  static SysExState currentSysExState = SysExState::SYSEX_IDLE;

  MidiPacket packet;

  while (true)
  {
    if (osPort && osPort->Get(&packet, portMAX_DELAY))
    {
      bool shouldForwardToApp = true;

      if (packet.SysEx())
      {
        if (activeSysExPort != MIDI_PORT_INVALID && packet.port != activeSysExPort)
        {
          continue;
        }

        if (packet.SysExStart())
        {
          sysExBuffer.reserve(12);
          sysExBuffer.clear();
          activeSysExPort = packet.port;
        }
        else if (currentSysExState == SysExState::SYSEX_INVALID)
        {
          if (packet.status == SysExEnd)
          {
            activeSysExPort = MIDI_PORT_INVALID;
            currentSysExState = SysExState::SYSEX_IDLE;
          }
          continue;
        }

        if (currentSysExState != SysExState::SYSEX_RELEASE)
        {
          if (sysExBuffer.size() + 3 > MAX_SYSTEM_SYSEX_SIZE)
          {
            sysExBuffer.clear();
            LogDroppedOversizedSysEx();
            if (packet.status == SysExEnd)
            {
              activeSysExPort = MIDI_PORT_INVALID;
              currentSysExState = SysExState::SYSEX_IDLE;
            }
            else
            {
              currentSysExState = SysExState::SYSEX_INVALID;
            }
            continue;
          }

          sysExBuffer.insert(sysExBuffer.end(), packet.data, packet.data + 3);
          currentSysExState = ProcessSysEx(packet.port, sysExBuffer, packet.status == SysExEnd);

          if (packet.status == SysExEnd)
          {
            activeSysExPort = MIDI_PORT_INVALID;
            currentSysExState = SysExState::SYSEX_IDLE;
          }
          shouldForwardToApp = false;
        }
      }

      if (packet.status == SysExEnd)
      {
        currentSysExState = SysExState::SYSEX_IDLE;
        activeSysExPort = MIDI_PORT_INVALID;
      }

      if (shouldForwardToApp && appQueue)
      {
        if (xQueueSend(appQueue, &packet, 0) != pdTRUE)
        {
          LogDroppedAppMidiPacket();
        }
        else
        {
          MystrixSIL::HostIO::TapMidi(0, packet.port, 0, packet);
        }
      }
    }
  }
}

bool SendSysEx(uint16_t port, uint16_t length, uint8_t* data, bool includeMeta) {
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

  for (uint8_t index = 0; index < length - 3 - !includeMeta; index += 3)
  {
    if (!Send(MidiPacket(EMidiStatus::SysExData, data[index], data[index + 1], data[index + 2]), port, 5))
    {
      return false;
    }
  }

  if (includeMeta)
  {
    uint16_t startPtr = length - length % 3;
    uint8_t footer[3] = {0, 0, 0};
    for (uint16_t i = startPtr; i < length; i++)
    {
      footer[i] = data[startPtr + i];
    }
    footer[length % 3] = MIDIv1_SYSEX_END;

    if (!Send(MidiPacket(EMidiStatus::SysExEnd, footer[0], footer[1], footer[2]), port, 5))
    {
      return false;
    }
  }
  else
  {
    uint8_t endFrameLength = ((length - 1) % 3 + 1);
    uint16_t startPtr = length - endFrameLength;
    uint8_t footer[3] = {0, 0, 0};
    for (uint16_t i = 0; i < endFrameLength; i++)
    {
      footer[i] = data[startPtr + i];
    }

    if (!Send(MidiPacket(EMidiStatus::SysExEnd, footer[0], footer[1], footer[2]), port, 5))
    {
      return false;
    }
  }
  return true;
}

SysExState ProcessSysEx(uint16_t port, vector<uint8_t>& sysExBuffer, bool complete) {
  if (sysExBuffer[0] != MIDIv1_SYSEX_START)
  {
    return SysExState::SYSEX_INVALID;
  }

  if (complete)
  {
    if (sysExBuffer[1] == MIDIv1_UNIVERSAL_NON_REALTIME_ID)
    {
      if (sysExBuffer[3] == USYSEX_GENERAL_INFO && sysExBuffer[4] == USYSEX_GI_ID_REQUEST)
      {
#if MATRIXOS_BUILD_VER == 0
        uint8_t osReleaseVersion = 0;
#elif (MATRIXOS_BUILD_VER == 4)
        uint8_t osReleaseVersion = 0x31;
#elif (MATRIXOS_RELEASE_VER < 32)
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
    else if (sysExBuffer[1] == MATRIXOS_SYSEX_REQUEST)
    {
      HandleMatrixOSSysEx(port, sysExBuffer);
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
  switch (sysExBuffer[2])
  {
  case MATRIXOS_COMMAND_GET_OS_VERSION: {
#if MATRIXOS_BUILD_VER == 0
    uint8_t osReleaseVersion = 0;
#elif (MATRIXOS_BUILD_VER == 4)
    uint8_t osReleaseVersion = 0x31;
#elif (MATRIXOS_RELEASE_VER < 32)
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
    uint32_t appId = ((uint32_t)sysExBuffer[4] << 25) + ((uint32_t)sysExBuffer[5] << 18) + ((uint32_t)sysExBuffer[6] << 11) +
                     ((uint32_t)sysExBuffer[7] << 4) + ((uint32_t)sysExBuffer[8] >> 3);
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
  default: {
    MLOGE("MIDI", "Unknown MatrixOS SysEx Command: %d", sysExBuffer[1]);
  }
  }
}
} // namespace MatrixOS::MIDI

namespace MatrixOS::USB::MIDI
{
std::vector<MidiPort> ports;
std::vector<TaskHandle_t> portTasks;
std::vector<string> portTaskNames;

void portTask(void* param) {
  uint8_t itf = (uint8_t)(uintptr_t)param;
  MidiPacket packet;
  while (true)
  {
    (void)ports[itf].Get(&packet, portMAX_DELAY);
  }
}

void Init() {
  for (TaskHandle_t portTaskHandle : portTasks)
  {
    vTaskDelete(portTaskHandle);
  }
  portTasks.clear();
  portTaskNames.clear();
  ports.clear();

  ports.reserve(USB_MIDI_COUNT);
  portTasks.reserve(USB_MIDI_COUNT);

  for (uint8_t i = 0; i < USB_MIDI_COUNT; i++)
  {
    string portname = "USB MIDI " + std::to_string(i + 1);
    ports.emplace_back(portname, MIDI_PORT_USB + i);

    portTasks.push_back(NULL);
    portTaskNames.push_back(portname);
    xTaskCreate(portTask, portTaskNames.back().c_str(), configMINIMAL_STACK_SIZE * 2, (void*)(uintptr_t)i, configMAX_PRIORITIES - 2,
                &portTasks.back());
  }
}
} // namespace MatrixOS::USB::MIDI