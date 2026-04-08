#include "Device.h"
#include "BLEMidi/BLEMidi.h"

#include <queue>
using std::queue;

#define TAG "BLE-MIDI"

#define BLE_MIDI_PORT_ID 2

namespace Device
{
namespace BLEMIDI
{
bool started = false;
string name;
MidiPort midiPortInstance;
MidiPort* midiPort = nullptr;
TaskHandle_t portTaskHandle = NULL;

void Callback(uint8_t blemidi_port, uint16_t timestamp, uint8_t midiStatus, uint8_t* remainingMessage, size_t len,
              size_t continuedSysexPos) {
  // MLOGD(TAG, "CALLBACK blemidi_port=%d, timestamp=%d, midiStatus=0x%02x, len=%d,
  // continuedSysexPos=%d, remainingMessage:", blemidi_port, timestamp, midiStatus, len, continuedSysexPos);
  // MLOGD(TAG, "Received 0x%02x 0x%02x 0x%02x", midiStatus, remainingMessage[0],
  // remainingMessage[1]);
  if (len == 2 && midiPort != nullptr)
  {
    midiPort->Send(MidiPacket((EMidiStatus)(midiStatus & 0xF0), midiStatus, remainingMessage[0], remainingMessage[1]));
  }
}

void Toggle() {
  if (started == false)
  {
    Start();
  }
  else
  {
    Stop();
  }
}

void Init(string name) {
  BLEMIDI::name = name;
}

void portTask(void* param) {
  MidiPacket packet;
  while (true)
  {
    if (midiPort != nullptr && midiPort->Get(&packet, portMAX_DELAY))
    {
      blemidi_send_message(midiPort->id % 0x100, packet.data, 3);
    }
  }
}

void Start() {
  int status = blemidi_init((void*)Callback, name.c_str());
  if (status < 0)
  {
    ESP_LOGE(TAG, "BLE MIDI Driver returned status=%d", status);
  }
  else
  {
    ESP_LOGI(TAG, "BLE MIDI Driver initialized successfully");
    if (midiPort == nullptr)
    {
      midiPort = &midiPortInstance;
      midiPort->SetName("Bluetooth");
      midiPort->Open(MIDI_PORT_BLUETOOTH);
    }
    xTaskCreate(portTask, "Bluetooth Midi Port", configMINIMAL_STACK_SIZE * 2, NULL, configMAX_PRIORITIES - 2, &portTaskHandle);
    started = true;
  }
}

void Stop() {
  int status = blemidi_deinit();
  if (status < 0)
  {
    ESP_LOGE(TAG, "BLE MIDI Driver returned status=%d", status);
  }
  else
  {
    ESP_LOGI(TAG, "BLE MIDI Driver deinitialized successfully");
    if (portTaskHandle != NULL)
    {
      vTaskDelete(portTaskHandle);
      portTaskHandle = NULL;
    }
    if (midiPort != nullptr)
    {
      midiPort->Close();
      midiPort = nullptr;
    }
    started = false;
  }
}
} // namespace BLEMIDI
} // namespace Device
