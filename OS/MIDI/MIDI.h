#include "MidiPort.h"
#include "MidiPacket.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

// TODO Put this in device layer
const uint8_t SYSEX_MFG_ID[3] = {0x00, 0x02, 0x03};
const uint8_t SYSEX_FAMILY_ID[3] = {0x4D, 0x58}; // {'M', 'X'}
const uint8_t SYSEX_MODEL_ID[3] = {0x11, 0x01};

enum SysExState : uint8_t {
  SYSEX_IDLE,     // No SysEx yet
  SYSEX_PENDING,  // Not sure who's SysEx this is or pending for system to parse
  SYSEX_COMPLETE, // It is a system SysEx and we have parsed it
  SYSEX_RELEASE,  // This is an application SysEx, release to application
  SYSEX_INVALID   // This is our SysEx, don't release, just destroy it
};

namespace MatrixOS::MIDI
  {
    inline MidiPort* osPort = nullptr;
    inline QueueHandle_t appQueue = nullptr;
    inline TaskHandle_t receiveTask = nullptr;

    void Init(void);
    void ReceiveTask(void* parameters);

    bool Get(MidiPacket* midiPacketDest, uint16_t timeout_ms);
    bool Send(MidiPacket midiPacket, uint16_t targetPort, uint16_t timeout_ms);
    bool SendSysEx(uint16_t port, uint16_t length, uint8_t* data, bool includeMeta);  // If include meta, it will send the correct header and ending;
    void HandleMatrixOSSysEx(uint16_t port, vector<uint8_t>& sysExBuffer);
    SysExState ProcessSysEx(uint16_t port, vector<uint8_t>& sysExBuffer, bool complete);
  }