#include "ForceCalibration.h"

void ForceCalibration::LowCalibration() {
  uint32_t start_time = MatrixOS::SYS::Millis();

  const uint32_t stabilize_time = 1000; // 1 second
  const uint32_t calibration_time = 120000; // 2 minutes
  const uint32_t done_time = 3000; // 3 seconds

  const float offset = 1.25;

  uint16_t calibration_data[X_SIZE][Y_SIZE] = {0};

  MatrixOS::LED::Fill(Color(0x000000));
  MatrixOS::LED::Update();

  float last_progress = 0;

  for(uint8_t y = 0; y < Y_SIZE; y++)
  {
    for(uint8_t x = 0; x < X_SIZE; x++)
    {
      calibration_data[x][y] = 0;
    }
  }
  
  for(uint8_t y = 0; y < 8; y++)
  {
    for(uint8_t x = 0; x < 8; x++)
    {
      MLOGD("Low Calibration", "Existing %d %d: %d", x, y, (*Device::KeyPad::FSR::low_thresholds)[x][y]);
    }
  }


  while (!MatrixOS::KeyPad::GetKey(FUNCTION_KEY)->Active())
  {
    uint32_t elapsed_time = MatrixOS::SYS::Millis() - start_time;
    if(elapsed_time < stabilize_time) // IDLE, wait for stabilize
    {

    }
    else if(elapsed_time < (stabilize_time + calibration_time)) // Calibration
    {
        // Calibrate
        for(uint8_t y = 0; y < 8; y++)
        {
          for(uint8_t x = 0; x < 8; x++)
          {
            uint16_t reading = Device::KeyPad::FSR::GetRawReading(x, y);

            if(reading > calibration_data[x][y])
            {
              calibration_data[x][y] = reading;
              MLOGD("Low Calibration", "%d %d Updated: %d", x, y, calibration_data[x][y]);
            }
          }
        }

        // Render
        float progress = (elapsed_time - stabilize_time) / (float)calibration_time * X_SIZE * Y_SIZE;
        uint8_t active_x = (uint16_t)progress % X_SIZE;
        uint8_t active_y = (uint16_t)progress / X_SIZE;

        if((uint8_t)last_progress != (uint8_t)progress)
        {
          uint8_t last_x = (uint16_t)(last_progress - 1) % X_SIZE;
          uint8_t last_y = (uint16_t)(last_progress - 1) / X_SIZE;

          MatrixOS::LED::SetColor(Point(last_x, last_y), Color(0xFFFFFF), 0);
        }

        MatrixOS::LED::SetColor(Point(active_x, active_y), Color(0xFFFFFF).Scale(255 * (progress - (uint8_t)progress)), 0);
        last_progress = progress;

        // for(uint8_t y = 0; y < 8; y++)
        // {
        //   for(uint8_t x = 0; x < 8; x++)
        //   {
        //     if(y * 8 + x < progress)
        //     {
        //       MatrixOS::LED::SetColor(Point(x, y), Color(0xFFFFFF));
        //     }
        //     else if(y * 8 + x + 1 < progress)
        //     {
        //       float brightness = (progress - (y * 8 + x)) / 1.0;
        //       MatrixOS::LED::SetColor(Point(x, y), Color(0xFFFFFF).Scale(brightness * 255));
        //     }
        //     else
        //     {
        //       MatrixOS::LED::SetColor(Point(x, y), Color(0x000000));
        //     }
        //   }
        // }

        // MatrixOS::LED::Update();
      }
    else if(elapsed_time < (stabilize_time + calibration_time + done_time)) // Done
    {
      if(!lowCalibrationSaved)
      {
        uint32_t average = 0;

        for(uint8_t y = 0; y < 8; y++)
        {
          for(uint8_t x = 0; x < 8; x++)
          {
            MLOGD("Low Calibration", "%d %d: %d (%d)", x, y, calibration_data[x][y], (uint16_t)(calibration_data[x][y] * offset));
            calibration_data[x][y] = (uint16_t)(calibration_data[x][y] * offset);
            average += calibration_data[x][y];
          }
        }
    
        average /= 64;
        
        memcpy(Device::KeyPad::FSR::low_thresholds, calibration_data, sizeof(calibration_data));
        Device::KeyPad::FSR::SaveLowCalibration();
        lowCalibrationSaved = true;

        MatrixOS::LED::Fill(Color(0), 0);
        MatrixOS::LED::SetColor(Point(2, 2), Color(0x00FF00), 0);
        MatrixOS::LED::SetColor(Point(3, 2), Color(0x00FF00), 0);
        MatrixOS::LED::SetColor(Point(4, 2), Color(0x00FF00), 0);
        MatrixOS::LED::SetColor(Point(5, 2), Color(0x00FF00), 0);
        MatrixOS::LED::SetColor(Point(2, 3), Color(0x00FF00), 0);
        MatrixOS::LED::SetColor(Point(5, 3), Color(0x00FF00), 0);
        MatrixOS::LED::SetColor(Point(2, 4), Color(0x00FF00), 0);
        MatrixOS::LED::SetColor(Point(5, 4), Color(0x00FF00), 0);
        MatrixOS::LED::SetColor(Point(2, 5), Color(0x00FF00), 0);
        MatrixOS::LED::SetColor(Point(3, 5), Color(0x00FF00), 0);
        MatrixOS::LED::SetColor(Point(4, 5), Color(0x00FF00), 0);
        MatrixOS::LED::SetColor(Point(5, 5), Color(0x00FF00), 0);
        MLOGD("Low Calibration", "Done - Average: %d", average);
      }

    }
    else // Done Done
    {
      return;
    }
  }
}