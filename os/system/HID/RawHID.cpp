#include "MatrixOS.h"
#include "message_buffer.h"

namespace MatrixOS::HID::RawHID
{
    MessageBufferHandle_t rawhid_message_buffer;

    void Init()
    {
        if(rawhid_message_buffer)
        {
            xMessageBufferReset(rawhid_message_buffer);
        }
        else
        {
            rawhid_message_buffer = xMessageBufferCreate(64);
        }
    }

    bool NewReport(const uint8_t *report, size_t size)
    {
        if (xMessageBufferSendFromISR(rawhid_message_buffer, report, size, NULL) == pdTRUE)
        {
            return true;
        }
        return false;
    }

    uint8_t report_buffer[64];

    size_t Get(uint8_t** report, uint32_t timeout_ms)
    {
        size_t received = xMessageBufferReceive(rawhid_message_buffer, (void*)&report_buffer, 64, pdMS_TO_TICKS(timeout_ms));
        
        if (received > 0)
        {
            *report = report_buffer;
        }

        return received;
    }

    bool Send(const vector<uint8_t> &report)
    {
			if(report.size() > 16)
			{
					MatrixOS::SYS::ErrorHandler("HID Report too large");
			}
      uint8_t reportBuffer[16];
      memcpy(reportBuffer, report.data(), report.size());
      memset(reportBuffer + report.size(), 0, 16 - report.size());

      return tud_hid_report(255, reportBuffer, 16);
    }
}


// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
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
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  // This example doesn't use multiple report and report ID
//   (void) itf;
//   (void) report_id;
//   (void) report_type;

    // MLOGD("RAW HID", "SET_REPORT - itf %d report id %u, type %u, len %u, data: %02X %02X %02X %02X %02X %02X %02X %02X",itf, report_id, report_type, bufsize, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
    
    if(report_id == 0xFF && report_type == HID_REPORT_TYPE_FEATURE)
    {
        (void)MatrixOS::HID::RawHID::NewReport(buffer, bufsize);
    }
    else if(bufsize > 0 && buffer[0] == 0xFF)
    {
        (void)MatrixOS::HID::RawHID::NewReport(buffer + 1, bufsize - 1);
    }
    // }
    // else if(report_id == 0x00 && report_type == HID_REPORT_TYPE_INVALID && bufsize > 1)
    // {
    //     MatrixOS::HID::RawHID::NewReport(buffer + 1, bufsize - 1);
    // }
}