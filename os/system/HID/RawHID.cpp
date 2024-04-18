#include "MatrixOS.h"

namespace MatrixOS::HID::RawHID
{
      int Available(void)
      {
        return 0;
      }

      int Read(void)
      {
        return 0;
      }

      int Peek(void)
      {
        return 0;
      }

      void Flush(void)
      {

      }

      size_t Write(uint8_t c)
      {
        return 0;
      }

      size_t Write(uint8_t* buffer, size_t size)
      {
        return 0;
      }

}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, /*uint16_t*/ uint8_t len)
{
  (void) instance;
  (void) len;

  // TODO
//   MLOGD("HID", "Report sent");
}


// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  // if (MatrixOS::USB::Inited())
  // {
  ESP_LOGI("HID", "Report requested: instance=%d, report_id=%d, report_type=%d, reqlen=%d", instance, report_id, report_type, reqlen);
  // }

  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;

  // if (report_id > 0)
  // {
   ESP_LOGI("HID", "Report recived: instance=%d, report_id=%d, report_type=%d, bufsize=%d", instance, report_id, report_type, bufsize);

   printf("Report: ");
    for (int i = 0; i < bufsize; i++)
    {
      printf("%02X ", buffer[i]);
    }
    printf("\n");
  // }
}