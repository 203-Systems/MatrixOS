#include "ForceCalibration.h"

void ForceCalibration::LowCalibration() {
  uint32_t startTime = MatrixOS::SYS::Millis();

  const uint32_t stabilizeTime = 1000;     // 1 second
  const uint32_t calibrationTime = 120000; // 2 minutes
  const uint32_t doneTime = 3000;          // 3 seconds

  const float offset = 1.25;

  uint16_t calibrationData[X_SIZE][Y_SIZE] = {0};

  MatrixOS::LED::Fill(Color(0x000000));
  MatrixOS::LED::Update();

  float lastProgress = 0;

  for (uint8_t y = 0; y < Y_SIZE; y++)
  {
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      calibrationData[x][y] = 0;
    }
  }

  for (uint8_t y = 0; y < 8; y++)
  {
    for (uint8_t x = 0; x < 8; x++)
    {
      MLOGD("Low Calibration", "Existing %d %d: %d", x, y, (*Device::KeyPad::FSR::lowThresholds)[x][y]);
    }
  }

  while (!MatrixOS::KeyPad::GetKey(FUNCTION_KEY)->Active())
  {
    uint32_t elapsedTime = MatrixOS::SYS::Millis() - startTime;
    if (elapsedTime < stabilizeTime) // IDLE, wait for stabilize
    {
    }
    else if (elapsedTime < (stabilizeTime + calibrationTime)) // Calibration
    {
      // Calibrate
      for (uint8_t y = 0; y < 8; y++)
      {
        for (uint8_t x = 0; x < 8; x++)
        {
          uint16_t reading = Device::KeyPad::FSR::GetRawReading(x, y);

          if (reading > calibrationData[x][y])
          {
            calibrationData[x][y] = reading;
            MLOGD("Low Calibration", "%d %d Updated: %d", x, y, calibrationData[x][y]);
          }
        }
      }

      // Render
      float progress = (elapsedTime - stabilizeTime) / (float)calibrationTime * X_SIZE * Y_SIZE;
      uint8_t activeX = (uint16_t)progress % X_SIZE;
      uint8_t activeY = (uint16_t)progress / X_SIZE;

      if ((uint8_t)lastProgress != (uint8_t)progress)
      {
        uint8_t lastX = (uint16_t)(lastProgress - 1) % X_SIZE;
        uint8_t lastY = (uint16_t)(lastProgress - 1) / X_SIZE;

        MatrixOS::LED::SetColor(Point(lastX, lastY), Color::White, 0);
      }

      MatrixOS::LED::SetColor(Point(activeX, activeY), Color::White.Scale(255 * (progress - (uint8_t)progress)), 0);
      lastProgress = progress;

      // for(uint8_t y = 0; y < 8; y++)
      // {
      //   for(uint8_t x = 0; x < 8; x++)
      //   {
      //     if(y * 8 + x < progress)
      //     {
      //       MatrixOS::LED::SetColor(Point(x, y), Color::White);
      //     }
      //     else if(y * 8 + x + 1 < progress)
      //     {
      //       float brightness = (progress - (y * 8 + x)) / 1.0;
      //       MatrixOS::LED::SetColor(Point(x, y), Color::White.Scale(brightness * 255));
      //     }
      //     else
      //     {
      //       MatrixOS::LED::SetColor(Point(x, y), Color(0x000000));
      //     }
      //   }
      // }

      // MatrixOS::LED::Update();
    }
    else if (elapsedTime < (stabilizeTime + calibrationTime + doneTime)) // Done
    {
      if (!lowCalibrationSaved)
      {
        uint32_t average = 0;

        for (uint8_t y = 0; y < 8; y++)
        {
          for (uint8_t x = 0; x < 8; x++)
          {
            MLOGD("Low Calibration", "%d %d: %d (%d)", x, y, calibrationData[x][y], (uint16_t)(calibrationData[x][y] * offset));
            calibrationData[x][y] = (uint16_t)(calibrationData[x][y] * offset);
            average += calibrationData[x][y];
          }
        }

        average /= 64;

        memcpy(Device::KeyPad::FSR::lowThresholds, calibrationData, sizeof(calibrationData));
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