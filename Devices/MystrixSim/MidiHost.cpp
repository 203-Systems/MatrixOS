#include "HostIO.h"

#include "MIDI/MIDI.h"
#include "USB/USB.h"
#include "Commands/CommandHandler.h"
#include "Framework/Midi/MidiPort.h"

#include "System/System.h"

namespace MatrixOS::MIDI
{
static constexpr size_t MAX_SYSTEM_SYSEX_SIZE = 1024;
static constexpr size_t MATRIXOS_SYSEX_REPLY_OVERHEAD = 3;
static constexpr uint32_t SYSEX_INACTIVITY_TIMEOUT_MS = 1000;
static uint32_t droppedAppMidiPackets = 0;
static uint32_t droppedOversizedSysExMessages = 0;
static uint32_t timedOutSysExSessions = 0;

static bool SendCommandSysExReply(const vector<uint8_t>& reply, bool end, void* context) {
  (void)end;
  if (context == nullptr)
  {
    return false;
  }

  for (uint8_t byte : reply)
  {
    if ((byte & 0x80) != 0)
    {
      return false;
    }
  }

  vector<uint8_t> sysExReply;
  sysExReply.reserve(reply.size() + 3);
  sysExReply.push_back(MIDIv1_SYSEX_START);
  sysExReply.push_back(MATRIXOS_SYSEX_RESPONSE);
  sysExReply.insert(sysExReply.end(), reply.begin(), reply.end());
  sysExReply.push_back(MIDIv1_SYSEX_END);
  return SendSysEx(*static_cast<uint16_t*>(context), sysExReply.size(), sysExReply.data(), false);
}

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
    MystrixSim::HostIO::TapMidi(1, midiPacket.port, targetPort, midiPacket);
  }
  return result;
}

void ReceiveTask(void* parameters) {
  static vector<uint8_t> sysExBuffer;
  static uint16_t activeSysExPort = MIDI_PORT_INVALID;
  static SysExState currentSysExState = SysExState::SYSEX_IDLE;
  static uint32_t lastSysExActivityMs = 0;

  MidiPacket packet;

  while (true)
  {
    if (osPort && osPort->Get(&packet, portMAX_DELAY))
    {
      bool shouldForwardToApp = true;
      uint32_t nowMs = SYS::Millis();

      if (activeSysExPort != MIDI_PORT_INVALID && (uint32_t)(nowMs - lastSysExActivityMs) > SYSEX_INACTIVITY_TIMEOUT_MS)
      {
        ResetActiveSysExSession(sysExBuffer, activeSysExPort, currentSysExState, lastSysExActivityMs);
        LogTimedOutSysExSession();
      }

      if (packet.SysEx())
      {
        if (packet.SysExStart())
        {
          if (activeSysExPort != MIDI_PORT_INVALID && packet.port != activeSysExPort)
          {
            continue;
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
            continue;
          }

          lastSysExActivityMs = nowMs;

          if (currentSysExState == SysExState::SYSEX_INVALID)
          {
            if (packet.status == SysExEnd)
            {
              ResetActiveSysExSession(sysExBuffer, activeSysExPort, currentSysExState, lastSysExActivityMs);
            }
            continue;
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
          shouldForwardToApp = false;
        }
      }

      if (packet.status == SysExEnd && packet.port == activeSysExPort)
      {
        ResetActiveSysExSession(sysExBuffer, activeSysExPort, currentSysExState, lastSysExActivityMs);
      }

      if (shouldForwardToApp && appQueue)
      {
        if (xQueueSend(appQueue, &packet, 0) != pdTRUE)
        {
          LogDroppedAppMidiPacket();
        }
        else
        {
          MystrixSim::HostIO::TapMidi(0, packet.port, 0, packet);
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

    if (sysExBuffer[1] == MIDIv1_UNIVERSAL_NON_REALTIME_ID)
    {
      if (sysExBuffer.size() >= 5 && sysExBuffer[3] == USYSEX_GENERAL_INFO && sysExBuffer[4] == USYSEX_GI_ID_REQUEST)
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
  if (sysExBuffer.size() < 4 || sysExBuffer.back() != MIDIv1_SYSEX_END)
  {
    return;
  }

  const uint8_t* request = sysExBuffer.data() + 2;
  size_t requestSize = sysExBuffer.size() - 3;

  if (!Command::Submit(Command::Encoding::SysEx7Bit, request, requestSize, MAX_SYSTEM_SYSEX_SIZE - MATRIXOS_SYSEX_REPLY_OVERHEAD,
                       SendCommandSysExReply, port))
  {
    MLOGE("MIDI", "Dropped MatrixOS SysEx command: %d", sysExBuffer[2]);
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
