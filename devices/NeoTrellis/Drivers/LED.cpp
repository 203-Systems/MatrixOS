#include "Device.h"
#include "framework/Color.h"
#include "driver/i2c_master.h"
#include "include/NeoTrellis.h"

namespace Device
{
  namespace LED
  {
    void Init() {
      for (uint8_t i = 0; i < 4; i++)
      {
        bool success = true;
      // Set LED Speed
      {
        uint8_t cmd[] = { SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_SPEED, 1 };  // 1 = 800kHz
        if (i2c_master_transmit(NeoTrellis::neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
          ESP_LOGE("NT-LED", "Failed to set LED speed for %d", i);
          success = false;
        }
        else {
          ESP_LOGI("NT-LED", "NeoTrellis %d LED speed set to 800kHz", i);
        } 
      }

      // Set up LED length
      {
        uint8_t cmd[] = {
          SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_BUF_LENGTH,
          (NEO_TRELLIS_NUM_KEYS * 3) >> 8, (NEO_TRELLIS_NUM_KEYS * 3) & 0xFF
        };

        if (i2c_master_transmit(NeoTrellis::neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
          ESP_LOGE("NT-LED", "Failed to set LED buffer length for %d", i);
          success = false;
        }
        else
        {
          ESP_LOGI("NT-LED", "NeoTrellis %d LED buffer length set to %d", i, NEO_TRELLIS_NUM_KEYS * 3);
        }
      }

      // Set LED pin
      {
        uint8_t cmd[] = { SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_PIN, NEO_TRELLIS_NEOPIX_PIN };
        if (i2c_master_transmit(NeoTrellis::neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
          ESP_LOGE("NT-LED", "Failed to set LED pin for %d", i);
          success = false;
        }
        // else
        // {
        //   ESP_LOGI("NT-LED", "NeoTrellis %d LED pin set to %d", i, NEO_TRELLIS_NEOPIX_PIN);
        // }
      }

      if (success)
      {
        ESP_LOGI("NT-LED", "NeoTrellis %d LED initialized", i);
      }
      else
      {
        ESP_LOGE("NT-LED", "NeoTrellis %d LED initialization failed", i);
      }
    }
  }

    void Start() {}

    void Update(Color* frameBuffer, vector<uint8_t>& brightness)  // Render LED
    {
      if (xSemaphoreTake(NeoTrellis::neotrellis_i2c_semaphore, portMAX_DELAY) == pdTRUE) 
      {
        for (uint8_t i = 0; i < 4; i++)
        {
          for (uint8_t s = 0; s < 2; s++)
          {
            uint16_t offset = s * NEO_TRELLIS_NUM_KEYS * 3 / 2;
            uint8_t cmd[4 + (NEO_TRELLIS_NUM_KEYS * 3 / 2)];
            cmd[0] = SEESAW_NEOPIXEL_BASE;
            cmd[1] = SEESAW_NEOPIXEL_BUF;
            cmd[2] = offset >> 8;
            cmd[3] = offset & 0xFF;

            for (uint16_t j = 0; j < NEO_TRELLIS_NUM_KEYS / 2; j++)
            {
              Color c = frameBuffer[j + i * NEO_TRELLIS_NUM_KEYS + (s * NEO_TRELLIS_NUM_KEYS / 2)];
              c = c.Scale(brightness[0]);

              cmd[4 + j * 3] = c.G;
              cmd[4 + j * 3 + 1] = c.R;
              cmd[4 + j * 3 + 2] = c.B;
            }

            if (i2c_master_transmit(NeoTrellis::neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
              ESP_LOGE("NT-LED", "Failed to set LED buffer for (%d-%d)", i, s);

              // return;
            }
          }
        }

        for (uint8_t i = 0; i < 4; i++)
        {
          uint8_t cmd[] = { SEESAW_NEOPIXEL_BASE, SEESAW_NEOPIXEL_SHOW };
          if (i2c_master_transmit(NeoTrellis::neotrellis_i2c_dev[i], cmd, sizeof(cmd), NEO_TRELLIS_I2C_TIMEOUT_VALUE_MS) != ESP_OK) {
            ESP_LOGE("NT-LED", "Failed to set LED Show for (%d)", i);
            // return;
          }
          // else
          // {
          //   ESP_LOGI("NT-LED", "NeoTrellis %d LED Show set", i);
          // }
        }
        xSemaphoreGive(NeoTrellis::neotrellis_i2c_semaphore);
      } else {
        ESP_LOGE("NT-LED", "Failed to take I2C semaphore");
      }
    }

    uint16_t XY2Index(Point xy) {
      if (xy.x >= 0 && xy.x < 4 && xy.y >= 0 && xy.y < 4)  // Main grid top left
      { return xy.x + xy.y * 4; }
      else if (xy.x >= 4 && xy.x < 8 && xy.y >= 0 && xy.y < 4)  // Main grid top right
      {return 16 + (xy.x - 4) + xy.y * 4; }
      else if (xy.x >= 0 && xy.x < 4 && xy.y >= 4 && xy.y < 8)  // Main grid bottom left
      { return 32 + xy.x + (xy.y - 4) * 4; }
      else if (xy.x >= 4 && xy.x < 8 && xy.y >= 4 && xy.y < 8)  // Main grid bottom right
      { return 48 + (xy.x - 4) + (xy.y - 4) * 4; }
      return UINT16_MAX;
    }

    Point Index2XY(uint16_t index)
    {
      if (index < 16) // Main grid top left
      {
        return Point(index % 4, index / 4);
      }
      else if (index < 32)  // Main grid top right
      {
        return Point(4 + (index % 4), index / 4 - 4);
      }
      else if (index < 48)  // Main grid bottom left
      {
        return Point(index % 4, 4 + (index / 4 - 8));
      }
      else if (index < 64)  // Main grid bottom right
      {
        return Point(4 + (index % 4), 4 + (index / 4 - 12));
      }
      return Point::Invalid();
    }

    // TODO This text is very wrong (GRID)
    // Matrix use the following ID Struct
    //  CCCC IIIIIIIIIIII
    //  C as class (4 bits), I as index (12 bits). I could be split by the class definition, for example, class 1
    //  (grid), it's split to XXXXXXX YYYYYYY. Class List: Class 0 - Raw Index - IIIIIIIIIIII Class 1 - Grid - XXXXXX
    //  YYYYYY Class 2 - TouchBar - IIIIIIIIIIII Class 3 - Underglow - IIIIIIIIIIII

    uint16_t ID2Index(uint16_t ledID) {
      uint8_t ledClass = ledID >> 12;
      switch (ledClass)
      {
        case 0:
          if (ledID < led_count)
            return ledID;
          break;
      }
      return UINT16_MAX;
    }
  }
}