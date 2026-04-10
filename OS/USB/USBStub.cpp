// USB/HID/CDC stub for builds without TinyUSB (DEVICE_TINYUSB == 0)
// Provides compile-compatible no-op implementations.

#include "MatrixOS.h"
#include "USB.h"

// --- MatrixOS::USB ---
namespace MatrixOS::USB
{
void Init(USB_MODE mode) { (void)mode; }
void SetMode(USB_MODE mode) { (void)mode; }
uint8_t GetMode() { return USB_MODE_NORMAL; }
bool Connected() { return false; }

namespace MIDI
{
void Init() {}
}

namespace CDC
{
bool Connected() { return false; }
uint32_t Available() { return 0; }
void Poll() {}

void Print(string str) { (void)str; }
void Println(string str) { (void)str; }
void Printf(string format, ...) { (void)format; }
void VPrintf(string format, va_list valst) { (void)format; (void)valst; }
void Flush() {}

int8_t Read() { return -1; }
uint32_t ReadBytes(void* buffer, uint32_t length) { (void)buffer; (void)length; return 0; }
string ReadString() { return ""; }
} // namespace CDC
} // namespace MatrixOS::USB

// --- MatrixOS::HID ---
namespace MatrixOS::HID
{
void Init() {}
void Reset() {}
bool Ready() { return false; }

namespace Keyboard
{
bool Tap(KeyboardKeycode keycode, uint16_t lengthMs) { (void)keycode; (void)lengthMs; return false; }
bool Press(KeyboardKeycode keycode) { (void)keycode; return false; }
bool Release(KeyboardKeycode keycode) { (void)keycode; return false; }
void ReleaseAll() {}
} // namespace Keyboard

namespace Gamepad
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

HID_GamepadReport_Data_t _report;
void Tap(uint8_t buttonId, uint16_t lengthMs) { (void)buttonId; (void)lengthMs; }
void Press(uint8_t buttonId) { (void)buttonId; }
void Release(uint8_t buttonId) { (void)buttonId; }
void ReleaseAll() {}

void Button(uint8_t buttonId, bool state) { (void)buttonId; (void)state; }
void Buttons(uint32_t buttonId) { (void)buttonId; }

void XAxis(int16_t value) { (void)value; }
void YAxis(int16_t value) { (void)value; }
void ZAxis(int16_t value) { (void)value; }
void RXAxis(int16_t value) { (void)value; }
void RYAxis(int16_t value) { (void)value; }
void RZAxis(int16_t value) { (void)value; }
void DPad(GamepadDPadDirection direction) { (void)direction; }
} // namespace Gamepad

namespace RawHID
{
void Init() {}
size_t Get(uint8_t** report, uint32_t timeoutMs) { (void)report; (void)timeoutMs; return 0; }
bool Send(const vector<uint8_t>& report) { (void)report; return false; }
} // namespace RawHID
} // namespace MatrixOS::HID

// putchar_ used by printf library
void putchar_(char character) { (void)character; }
