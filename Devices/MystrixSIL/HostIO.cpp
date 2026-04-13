#include "HostIO.h"

#include "USB/USB.h"
#include "message_buffer.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace MatrixOS::HID::Gamepad
{
typedef struct {
  int16_t xAxis;
  int16_t yAxis;
  int16_t zAxis;
  int16_t rzAxis;
  int16_t rxAxis;
  int16_t ryAxis;
  uint8_t dPad;
  uint32_t buttons;
} HID_GamepadReport_Data_t;

extern HID_GamepadReport_Data_t _report;
} // namespace MatrixOS::HID::Gamepad

namespace MystrixSIL::HostIO
{
namespace
{
std::atomic<bool> usbConnectedState{true};

typedef union __attribute__((packed, aligned(1))) {
  uint8_t whole8[0];
  uint16_t whole16[0];
  uint32_t whole32[0];
  struct __attribute__((packed, aligned(1))) {
    uint8_t modifiers;
    uint8_t reserved;
    KeyboardKeycode keycodes[6];
  };
  uint8_t keys[8];
} HID_KeyboardReport_Data_t;

HID_KeyboardReport_Data_t keyboardReport = {};

MessageBufferHandle_t rawhidMessageBuffer = nullptr;
uint8_t rawhidReportBuffer[32];

void TapKeyboardTx() {
#ifdef __EMSCRIPTEN__
  MAIN_THREAD_ASYNC_EM_ASM({
    if (typeof window._matrixos_hid_tap === 'function')
      window._matrixos_hid_tap(0, 1, $0, $1, $2, $3, $4, $5, $6, $7);
  }, keyboardReport.keys[0], keyboardReport.keys[1], keyboardReport.keys[2], keyboardReport.keys[3],
     keyboardReport.keys[4], keyboardReport.keys[5], keyboardReport.keys[6], keyboardReport.keys[7]);
#endif
}

void TapGamepadTx() {
#ifdef __EMSCRIPTEN__
  MAIN_THREAD_ASYNC_EM_ASM({
    if (typeof window._matrixos_hid_tap === 'function')
      window._matrixos_hid_tap(1, 1, $0, $1, $2, $3, $4, $5, $6, $7);
  }, (int)MatrixOS::HID::Gamepad::_report.buttons, (int)MatrixOS::HID::Gamepad::_report.dPad,
     (int)MatrixOS::HID::Gamepad::_report.xAxis, (int)MatrixOS::HID::Gamepad::_report.yAxis, (int)MatrixOS::HID::Gamepad::_report.zAxis,
     (int)MatrixOS::HID::Gamepad::_report.rxAxis, (int)MatrixOS::HID::Gamepad::_report.ryAxis, (int)MatrixOS::HID::Gamepad::_report.rzAxis);
#endif
}

void TapRawHid(int direction, const uint8_t* report, size_t size) {
#ifdef __EMSCRIPTEN__
  uint8_t* tapBuf = static_cast<uint8_t*>(malloc(size));
  if (tapBuf)
  {
    memcpy(tapBuf, report, size);
    MAIN_THREAD_ASYNC_EM_ASM({
      if (typeof window._matrixos_hid_tap === 'function') {
        var data = [];
        for (var i = 0; i < $2; i++) data.push(HEAPU8[$1 + i]);
        window._matrixos_hid_tap(2, $0, data);
      }
      if (typeof Module !== 'undefined' && Module._free) Module._free($1);
    }, direction, (int)(uintptr_t)tapBuf, (int)size);
  }
#else
  (void)direction;
  (void)report;
  (void)size;
#endif
}

bool SetKeyboardKey(KeyboardKeycode keycode, bool state) {
  if (keycode >= KEY_LEFT_CTRL && keycode <= KEY_RIGHT_GUI)
  {
    keycode = KeyboardKeycode(uint8_t(keycode) - uint8_t(KEY_LEFT_CTRL));
    if (state)
    {
      keyboardReport.modifiers |= (1 << keycode);
    }
    else
    {
      keyboardReport.modifiers &= ~(1 << keycode);
    }
    return true;
  }

  const uint8_t keycodesSize = sizeof(keyboardReport.keycodes);
  if (state)
  {
    uint8_t emptyIndex = keycodesSize;
    for (uint8_t i = 0; i < keycodesSize; i++)
    {
      auto current = keyboardReport.keycodes[i];
      if (current == uint8_t(keycode))
      {
        return true;
      }
      if (current == KEY_RESERVED && emptyIndex == keycodesSize)
      {
        emptyIndex = i;
      }
    }
    if (emptyIndex != keycodesSize)
    {
      keyboardReport.keycodes[emptyIndex] = keycode;
      return true;
    }
  }
  else
  {
    for (uint8_t i = 0; i < keycodesSize; i++)
    {
      if (keyboardReport.keycodes[i] == keycode)
      {
        for (uint8_t j = i; j + 1 < keycodesSize; j++)
        {
          keyboardReport.keycodes[j] = keyboardReport.keycodes[j + 1];
        }
        keyboardReport.keycodes[keycodesSize - 1] = KEY_RESERVED;
        return true;
      }
    }
  }

  return false;
}

void EnsureRawHidBuffer() {
  if (!rawhidMessageBuffer)
  {
    rawhidMessageBuffer = xMessageBufferCreate(128);
  }
}

void ResetRawHidBuffer() {
  EnsureRawHidBuffer();
  if (rawhidMessageBuffer)
  {
    xMessageBufferReset(rawhidMessageBuffer);
  }
}
} // namespace

bool UsbConnected() {
  return usbConnectedState.load(std::memory_order_acquire);
}

void SetUsbConnected(bool connected) {
  usbConnectedState.store(connected, std::memory_order_release);
}

void TapSerial(int direction, const string& text) {
#ifdef __EMSCRIPTEN__
  char* tapStr = strdup(text.c_str());
  if (tapStr)
  {
    MAIN_THREAD_ASYNC_EM_ASM({
      if (typeof window._matrixos_serial_tap === 'function')
        window._matrixos_serial_tap($0, UTF8ToString($1));
      if (typeof Module !== 'undefined' && Module._free) Module._free($1);
    }, direction, tapStr);
  }
#else
  (void)direction;
  (void)text;
#endif
}

void TapMidi(int direction, uint16_t srcPort, uint16_t dstPort, const MidiPacket& midiPacket) {
#ifdef __EMSCRIPTEN__
  MAIN_THREAD_ASYNC_EM_ASM({
    if (typeof window._matrixos_midi_tap === 'function')
      window._matrixos_midi_tap($0, $1, $2, $3, $4, $5, $6);
  }, direction, (int)srcPort, (int)dstPort, (int)(uint8_t)midiPacket.status,
     (int)midiPacket.data[0], (int)midiPacket.data[1], (int)midiPacket.data[2]);
#else
  (void)direction;
  (void)srcPort;
  (void)dstPort;
  (void)midiPacket;
#endif
}

bool NewRawHidReport(const uint8_t* report, size_t size) {
  EnsureRawHidBuffer();
  TapRawHid(0, report, size);
  return xMessageBufferSendFromISR(rawhidMessageBuffer, report, size, nullptr) == pdTRUE;
}
} // namespace MystrixSIL::HostIO

namespace MatrixOS::USB
{
static uint8_t mode = USB_MODE_NORMAL;

void Init(USB_MODE newMode) {
  MatrixOS::USB::mode = newMode;
  if (newMode == USB_MODE_NORMAL)
  {
    USB::MIDI::Init();
  }
}

void SetMode(USB_MODE newMode) {
  MatrixOS::USB::mode = newMode;
  if (newMode == USB_MODE_NORMAL)
  {
    USB::MIDI::Init();
  }
}

uint8_t GetMode() {
  return mode;
}

bool Connected() {
  return MystrixSIL::HostIO::UsbConnected();
}

namespace CDC
{
bool Connected() {
  return MatrixOS::USB::Connected();
}

uint32_t Available() {
  return 0;
}

void Poll() {}

void Print(string str) {
  MystrixSIL::HostIO::TapSerial(1, str);
}

void Println(string str) {
  Print(str + "\n\r");
}

void Printf(string format, ...) {
  va_list valst;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvarargs"
  va_start(valst, format.c_str());
#pragma GCC diagnostic pop

  VPrintf(format, valst);
  va_end(valst);
}

void VPrintf(string format, va_list valst) {
  va_list copy;
  va_copy(copy, valst);
  int length = vsnprintf(nullptr, 0, format.c_str(), copy);
  va_end(copy);
  if (length <= 0)
  {
    return;
  }

  string buffer(static_cast<size_t>(length), '\0');
  vsnprintf(buffer.data(), buffer.size() + 1, format.c_str(), valst);
  Print(buffer);
}

void Flush() {}

int8_t Read() {
  return -1;
}

uint32_t ReadBytes(void* buffer, uint32_t length) {
  (void)buffer;
  (void)length;
  return 0;
}

string ReadString() {
  return "";
}
} // namespace CDC
} // namespace MatrixOS::USB

namespace MatrixOS::HID
{
bool Ready() {
  return MatrixOS::USB::Connected();
}

void Reset() {
  MystrixSIL::HostIO::ResetRawHidBuffer();
  memset(&MystrixSIL::HostIO::keyboardReport, 0, sizeof(MystrixSIL::HostIO::keyboardReport));
  memset(&Gamepad::_report, 0, sizeof(Gamepad::_report));
}

void Init() {
  Reset();
}

namespace Keyboard
{
bool Tap(KeyboardKeycode keycode, uint16_t lengthMs) {
  bool result = Press(keycode);
  if (result)
  {
    SYS::DelayMs(lengthMs);
    result = Release(keycode);
  }
  return result;
}

bool Press(KeyboardKeycode keycode) {
  bool result = MystrixSIL::HostIO::SetKeyboardKey(keycode, true);
  if (result)
  {
    MystrixSIL::HostIO::TapKeyboardTx();
  }
  return result;
}

bool Release(KeyboardKeycode keycode) {
  bool result = MystrixSIL::HostIO::SetKeyboardKey(keycode, false);
  if (result)
  {
    MystrixSIL::HostIO::TapKeyboardTx();
  }
  return result;
}

void ReleaseAll() {
  memset(&MystrixSIL::HostIO::keyboardReport, 0, sizeof(MystrixSIL::HostIO::keyboardReport));
  MystrixSIL::HostIO::TapKeyboardTx();
}
} // namespace Keyboard

namespace Gamepad
{
HID_GamepadReport_Data_t _report = {};

void Send() {
  MystrixSIL::HostIO::TapGamepadTx();
}

void Tap(uint8_t buttonId, uint16_t lengthMs) {
  Press(buttonId);
  SYS::DelayMs(lengthMs);
  Release(buttonId);
}

void Press(uint8_t buttonId) {
  _report.buttons |= (uint32_t)1 << buttonId;
  Send();
}

void Release(uint8_t buttonId) {
  _report.buttons &= ~((uint32_t)1 << buttonId);
  Send();
}

void ReleaseAll(void) {
  memset(&_report, 0, sizeof(_report));
  Send();
}

void Button(uint8_t buttonId, bool state) {
  if (state)
  {
    Press(buttonId);
  }
  else
  {
    Release(buttonId);
  }
}

void Buttons(uint32_t buttonState) {
  _report.buttons = buttonState;
  Send();
}

void XAxis(int16_t value) {
  _report.xAxis = value;
  Send();
}

void YAxis(int16_t value) {
  _report.yAxis = value;
  Send();
}

void ZAxis(int16_t value) {
  _report.zAxis = value;
  Send();
}

void RXAxis(int16_t value) {
  _report.rxAxis = value;
  Send();
}

void RYAxis(int16_t value) {
  _report.ryAxis = value;
  Send();
}

void RZAxis(int16_t value) {
  _report.rzAxis = value;
  Send();
}

void DPad(GamepadDPadDirection direction) {
  _report.dPad = direction;
  Send();
}
} // namespace Gamepad

namespace RawHID
{
void Init() {
  MystrixSIL::HostIO::ResetRawHidBuffer();
}

size_t Get(uint8_t** report, uint32_t timeoutMs) {
  MystrixSIL::HostIO::EnsureRawHidBuffer();
  size_t received = xMessageBufferReceive(MystrixSIL::HostIO::rawhidMessageBuffer, (void*)&MystrixSIL::HostIO::rawhidReportBuffer, 32,
                                          pdMS_TO_TICKS(timeoutMs));
  if (received > 0)
  {
    *report = MystrixSIL::HostIO::rawhidReportBuffer;
  }
  return received;
}

bool Send(const vector<uint8_t>& report) {
  if (report.size() > 32)
  {
    MatrixOS::SYS::ErrorHandler("HID Report too large");
  }

  uint8_t reportBuffer[32] = {};
  memcpy(reportBuffer, report.data(), std::min<size_t>(report.size(), 32));
  MystrixSIL::HostIO::TapRawHid(1, reportBuffer, sizeof(reportBuffer));
  return Ready();
}
} // namespace RawHID
} // namespace MatrixOS::HID

void putchar_(char character) {
  (void)character;
}
