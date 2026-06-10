#include "MatrixOS.h"
#include "USB.h"
#include "HID.h"
#include "tusb.h"
#include "Commands/CommandHandler.h"
#include "message_buffer.h"

#include "../System/System.h"

namespace MatrixOS::HID::RawHID
{
static constexpr size_t RAW_HID_REPORT_SIZE = 32;
MessageBufferHandle_t rawhidMessageBuffer;

static bool SendCommandReply(const vector<uint8_t>& reply, bool end, void* context) {
  (void)end;
  (void)context;
  return Send(reply);
}

void Init() {
  if (rawhidMessageBuffer)
  {
    xMessageBufferReset(rawhidMessageBuffer);
  }
  else
  {
    rawhidMessageBuffer = xMessageBufferCreate(128);
  }
}

bool HandleMatrixOSHID(const uint8_t* report, size_t size) {
  // MLOGD("RAW HID", "Received MatrixOS HID Command: %d", report[0]);

  return Command::Handle(report, size, Command::Encoding::HID, RAW_HID_REPORT_SIZE, SendCommandReply, nullptr);
}

bool NewReport(const uint8_t* report, size_t size) {
  if (xMessageBufferSendFromISR(rawhidMessageBuffer, report, size, NULL) == pdTRUE)
  {
    return true;
  }
  return false;
}

uint8_t reportBuffer[RAW_HID_REPORT_SIZE];

size_t Get(uint8_t** report, uint32_t timeoutMs) {
  size_t received = xMessageBufferReceive(rawhidMessageBuffer, (void*)&reportBuffer, RAW_HID_REPORT_SIZE, pdMS_TO_TICKS(timeoutMs));

  if (received > 0)
  {
    *report = reportBuffer;
  }

  return received;
}

bool Send(const vector<uint8_t>& report) {
  if (report.size() > RAW_HID_REPORT_SIZE)
  {
    MatrixOS::SYS::ErrorHandler("HID Report too large");
    return false;
  }
  uint8_t reportBuffer[RAW_HID_REPORT_SIZE];
  memcpy(reportBuffer, report.data(), report.size());
  memset(reportBuffer + report.size(), 0, RAW_HID_REPORT_SIZE - report.size());

  return tud_hid_report(255, reportBuffer, RAW_HID_REPORT_SIZE);
}
} // namespace MatrixOS::HID::RawHID

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
  // TODO not Implemented
  //   (void) itf;
  //   (void) report_id;
  //   (void) report_type;
  //   (void) buffer;
  //   (void) reqlen;

  //   MLOGD("RAW HID", "GET_REPORT - itf %d report id %u, type %u, len %u", itf, report_id, report_type, reqlen);

  return 0;
}

// Invoked whn received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
  if (bufsize == 0)
  {
    return;
  }

  // MLOGD("RAW HID", "SET_REPORT - itf %d report id %u, type %u, len %u, data: %02X %02X %02X %02X %02X %02X %02X %02X",itf, report_id,
  // report_type, bufsize, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);

  if (buffer[0] == 0xCB)
  {
    MatrixOS::HID::RawHID::HandleMatrixOSHID(buffer + 1, bufsize - 1);
  }
  else if (buffer[0] == 0xFF)
  {
    (void)MatrixOS::HID::RawHID::NewReport(buffer + 1, bufsize - 1);
  }
}
