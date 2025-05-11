#include "Device.h"
#include "MatrixOS.h"
#include "ui/UI.h"

#include "esp_private/system_internal.h"  // For esp_reset_reason_set_hint

#include "esp_efuse.h"
#include "esp_efuse_table.h"

namespace Device
{
  namespace KeyPad
  {
    void Scan();
  }

  void DeviceInit() {
    // esp_timer_early_init();
    gpio_install_isr_service(0);
    USB::Init();
    NVS::Init();

    NeoTrellis::Init();
    LED::Init();
    KeyPad::Init();
    
    BLEMIDI::Init(name);
  }

  void DeviceStart() {
    Device::KeyPad::Start();
    Device::LED::Start();

    if (bluetooth)
    { BLEMIDI::Start(); }

    Device::KeyPad::Scan();
    //Use KeyInfo->velocity instead KeyInfo->Active() because it might still be debouncing
    if (KeyPad::GetKey(KeyPad::XY2ID(Point(0, 0)))->velocity && KeyPad::GetKey(KeyPad::XY2ID(Point(1, 1)))->velocity)
    { MatrixOS::SYS::ExecuteAPP("203 Systems", "Mystrix Factory Menu"); }
    else if (KeyPad::GetKey(KeyPad::XY2ID(Point(6, 6)))->velocity &&
             KeyPad::GetKey(KeyPad::XY2ID(Point(7, 7)))->velocity)
    {
      KeyPad::Clear();
      MatrixOS::UserVar::brightness.Set(Device::led_brightness_level[0]);
    }
    else if (KeyPad::GetKey(KeyPad::XY2ID(Point(0, 5)))->velocity &&
             KeyPad::GetKey(KeyPad::XY2ID(Point(1, 6)))->velocity &&
             KeyPad::GetKey(KeyPad::XY2ID(Point(0, 7)))->velocity)
    {
      MatrixOS::LED::SetColor(Point(2, 2), Color(0xFF00FF));
      MatrixOS::LED::SetColor(Point(5, 2), Color(0xFF00FF));
      MatrixOS::LED::SetColor(Point(2, 5), Color(0xFF00FF));
      MatrixOS::LED::SetColor(Point(5, 5), Color(0xFF00FF));
      MatrixOS::LED::Update();
      MatrixOS::SYS::DelayMs(1500);
      Device::NVS::Clear();
      MatrixOS::SYS::Reboot();
    }
  }

  void DeviceSettings()
  {
    UI deviceSettings("Device Settings", Color(0x00FFAA));

    UIButton bluetoothToggle;
    bluetoothToggle.SetName("Bluetooth");
    bluetoothToggle.SetColorFunc([&]() -> Color {
      return Color(0x0082fc).DimIfNot(Device::BLEMIDI::started);
    });
    bluetoothToggle.OnPress([&]() -> void {
      Device::BLEMIDI::Toggle();
      Device::bluetooth = Device::BLEMIDI::started;
    });
    bluetoothToggle.OnHold([&]() -> void {
      MatrixOS::UIUtility::TextScroll(bluetoothToggle.name + " " + (Device::BLEMIDI::started ? "Enabled" : "Disabled"), bluetoothToggle.GetColor());
    });
    deviceSettings.AddUIComponent(bluetoothToggle, Point(0, 0));

    deviceSettings.Start();
  }

  void Bootloader() {
// Check out esp_reset_reason_t for other Espressif pre-defined values
#define APP_REQUEST_UF2_RESET_HINT (esp_reset_reason_t)0x11F2

    // call esp_reset_reason() is required for idf.py to properly links esp_reset_reason_set_hint()
    (void)esp_reset_reason();
    esp_reset_reason_set_hint(APP_REQUEST_UF2_RESET_HINT);
    esp_restart();
  }

  void Reboot() {
    esp_restart();
  }

  void Delay(uint32_t interval) {
    vTaskDelay(pdMS_TO_TICKS(interval));
  }

  uint32_t Millis() {
    return ((((uint64_t)xTaskGetTickCount()) * 1000) / configTICK_RATE_HZ);
    // return 0;
  }

  void Log(string &format, va_list &valst) {
    // ESP_LOG_LEVEL((esp_log_level_t)level, tag.c_str(), format.c_str(), valst);
    // esp_log_writev(ESP_LOG_INFO, format.c_str(), valst);

    vprintf(format.c_str(), valst);
  }

  string GetSerial() {
    uint8_t uuid[16];
    esp_efuse_read_field_blob(ESP_EFUSE_OPTIONAL_UNIQUE_ID, (void*)uuid, 128);
    string uuid_str;
    uuid_str.reserve(33);
    const char char_table[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    for (uint8_t i = 0; i < 16; i++)
    {
      uuid_str += char_table[uuid[i] >> 4];
      uuid_str += char_table[uuid[i] & 0x0F];
    }
    return uuid_str;  // TODO
  }

  void ErrorHandler() {}

  namespace MIDI
  {
    uint32_t Available() {
      uint32_t packets = 0;
      if (BLEMIDI::started)
      { packets += BLEMIDI::MidiAvailable(); }
      if (ESPNOW::started)
      { packets += ESPNOW::MidiAvailable(); }
      packets += ESPNOW::MidiAvailable();
      return packets;
    }

    MidiPacket Get() {
      if (BLEMIDI::started && BLEMIDI::MidiAvailable())
      { return BLEMIDI::GetMidi(); }
      if (ESPNOW::started && ESPNOW::MidiAvailable())
      { return ESPNOW::GetMidi(); }
      return MidiPacket(EMidiPortID::MIDI_PORT_EACH_CLASS, None);
    }

    bool Send(MidiPacket packet) {
      if (BLEMIDI::started)
      { BLEMIDI::SendMidi(packet.data); }
      if (ESPNOW::started)
      { ESPNOW::SendMidi(packet.data); }
      return true;  // idk what bool should mean in a multi port situation. Leave it be for now
    }
  }
}

namespace MatrixOS::SYS
{
  void ErrorHandler(char const* error);
}

extern "C" {
int main();
void app_main(void) {
  main();
}
}
