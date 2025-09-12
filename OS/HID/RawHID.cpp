#include "MatrixOS.h"
#include "USB.h"
#include "HID.h"
#include "tusb.h"
#include "Commands/CommandSpecs.h"
#include "message_buffer.h"

#include "../System/System.h"

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
            rawhid_message_buffer = xMessageBufferCreate(128);
        }
    }

    bool HandleMatrixOSHID(const uint8_t* report, size_t size)
    {
      // MLOGD("RAW HID", "Received MatrixOS HID Command: %d", report[0]);

      switch(report[0])
      {
        case MATRIXOS_COMMAND_GET_OS_VERSION:
        {
          vector<uint8_t> reply = { MATRIXOS_COMMAND_GET_OS_VERSION | 0x80, MATRIXOS_MAJOR_VER, MATRIXOS_MINOR_VER, MATRIXOS_PATCH_VER, MATRIXOS_BUILD_VER, MATRIXOS_RELEASE_VER};
          return Send(reply);
        }
        case MATRIXOS_COMMAND_GET_APP_ID:
        {
          vector<uint8_t> reply = { MATRIXOS_COMMAND_GET_APP_ID | 0x80, (uint8_t)(SYS::active_app_id >> 24), (uint8_t)(SYS::active_app_id >> 16), (uint8_t)(SYS::active_app_id >> 8), (uint8_t)(SYS::active_app_id)};
          return Send(reply);
        }
        case MATRIXOS_COMMAND_ENTER_APP_VIA_ID:
        {
          if(size != 9)
          {
            return false;
          }
          uint32_t app_id = ((uint32_t)report[1] << 24) + ((uint32_t)report[2] << 16) + ((uint32_t)report[3] << 8) + ((uint32_t)report[4]);
          SYS::ExecuteAPP(app_id);
          return true;
        }
        case MATRIXOS_COMMAND_QUIT_APP:
        {
          SYS::ExitAPP();
          return true;
        }
        case MATRIXOS_COMMAND_BOOTLOADER:
        {
          SYS::Bootloader();
          return true;
        }
        case MATRIXOS_COMMAND_REBOOT:
        {
          SYS::Reboot();
          return true;
        }
        default:
        {
          return false;
        }
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

    uint8_t report_buffer[32];

    size_t Get(uint8_t** report, uint32_t timeout_ms)
    {
        size_t received = xMessageBufferReceive(rawhid_message_buffer, (void*)&report_buffer, 32, pdMS_TO_TICKS(timeout_ms));
        
        if (received > 0)
        {
            *report = report_buffer;
        }

        return received;
    }

    bool Send(const vector<uint8_t> &report)
    {
			if(report.size() > 32)
			{
					MatrixOS::SYS::ErrorHandler("HID Report too large");
			}
      uint8_t reportBuffer[32];
      memcpy(reportBuffer, report.data(), report.size());
      memset(reportBuffer + report.size(), 0, 32 - report.size());

      return tud_hid_report(255, reportBuffer, 32);
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
  if(bufsize == 0)
  {
    return;
  }

    // MLOGD("RAW HID", "SET_REPORT - itf %d report id %u, type %u, len %u, data: %02X %02X %02X %02X %02X %02X %02X %02X",itf, report_id, report_type, bufsize, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
    
    if(buffer[0] == 0xCB)
    {
        MatrixOS::HID::RawHID::HandleMatrixOSHID(buffer + 1, bufsize - 1);
    }
    else if (buffer[0] == 0xFF)
    {
        (void)MatrixOS::HID::RawHID::NewReport(buffer + 1, bufsize - 1);
    }
}