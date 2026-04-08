#include "ForceCalibration.h"

void ForceCalibration::HighCalibration() {
  Timer renderTimer;

  enum calibrationState { Idle, Stabilizing, Recording, Done };

  uint8_t progress = 0;
  uint32_t calibrationRecordingProgress = 0;
  uint64_t calibrationRecording = 0;
  uint32_t eventTimeStamp = 0;

  calibrationState calibrationState = Idle;

  uint32_t localMax = 0;
  uint32_t localMin = FRACT16_MAX;

  uint32_t scanCountCache = 0;

  uint16_t calibrationData[X_SIZE][Y_SIZE] = {0};

  const uint32_t stabilizeTime = 1000; // 1 seconds
  const uint32_t calibrationSamples = 300;
  const uint32_t calibrationThreshold = FRACT16_MAX * 0.25;

  const uint32_t doneTime = 3000; // 3 seconds

  MatrixOS::LED::Fill(Color(0x000000));
  MatrixOS::LED::Update();

  for (uint8_t y = 0; y < Y_SIZE; y++)
  {
    for (uint8_t x = 0; x < X_SIZE; x++)
    {
      MLOGD("High Calibration", "Existing %d %d: %d", x, y, (*Device::KeyPad::FSR::highThresholds)[x][y]);
    }
  }

  while (!MatrixOS::KeyPad::GetKey(FUNCTION_KEY)->Active())
  {
    if (progress < X_SIZE * Y_SIZE)
    {
      uint8_t x = progress % X_SIZE;
      uint8_t y = progress / X_SIZE;

      uint16_t reading = Device::KeyPad::FSR::GetRawReading(x, y);

      if (calibrationState == Idle && reading >= calibrationThreshold)
      {
        calibrationState = Stabilizing;
        eventTimeStamp = MatrixOS::SYS::Millis();
      }
      else if (calibrationState == Stabilizing)
      {
        if (reading < calibrationThreshold)
        {
          calibrationState = Idle;
          eventTimeStamp = MatrixOS::SYS::Millis();
        }
        else if (MatrixOS::SYS::Millis() - eventTimeStamp > stabilizeTime)
        {
          calibrationState = Recording;
          eventTimeStamp = MatrixOS::SYS::Millis();

          calibrationRecording = 0;
          calibrationRecordingProgress = 0;
          localMax = 0;
          localMin = FRACT16_MAX;
        }
      }
      else if (calibrationState == Recording && reading < calibrationThreshold)
      {
        calibrationState = Idle;
        eventTimeStamp = MatrixOS::SYS::Millis();
      }
      else if (calibrationState == Recording)
      {
        uint32_t scanCount = Device::KeyPad::FSR::GetScanCount();
        if (scanCount != scanCountCache)
        {
          scanCountCache = scanCount;
          calibrationRecording += reading;

          if (reading > localMax)
          {
            localMax = reading;
          }
          if (reading < localMin)
          {
            localMin = reading;
          }

          calibrationRecordingProgress++;

          if (calibrationRecordingProgress == calibrationSamples)
          {
            calibrationData[x][y] = calibrationRecording / calibrationSamples;

            calibrationState = Idle;
            eventTimeStamp = MatrixOS::SYS::Millis();
            progress++;

            MatrixOS::LED::SetColor(Point(x, y), Color(0x00FF00), 0);

            MLOGD("High Calibration", "%d %d completed - Avg %d, Max %d, Min %d", x, y, calibrationData[x][y], localMax, localMin);
          }
        }
      }

      // Render
      if (renderTimer.Tick(1000 / Device::LED::fps))
      {
        x = progress % X_SIZE;
        y = progress / X_SIZE;
        if (calibrationState == Idle)
        {
          // Breath Effect
          // Calculate brightness based on current time and period
          uint32_t time = MatrixOS::SYS::Millis() - eventTimeStamp;
          uint32_t period = 500; // Set the desired period in milliseconds
          float brightness = (1 + sin(2 * M_PI * time / period)) / 2 * 255;

          // Apply brightness to the color
          Color color = Color::White.Scale((uint8_t)brightness);

          MatrixOS::LED::SetColor(Point(x, y), color, 0);
        }
        else if (calibrationState == Stabilizing)
        {
          MatrixOS::LED::SetColor(Point(x, y), Color(0xFFFF00), 0);
        }
        else if (calibrationState == Recording)
        {

          float completion = calibrationRecordingProgress / (float)calibrationSamples;
          uint8_t brightness = 255 * completion;
          MatrixOS::LED::SetColor(Point(x, y), Color(0x00FF00).Scale(brightness), 0);
        }
      }
    }
    else if (progress == X_SIZE * Y_SIZE)
    {

      if (!highCalibrationSaved)
      {
        memcpy(Device::KeyPad::FSR::highThresholds, calibrationData, sizeof(calibrationData));
        Device::KeyPad::FSR::SaveHighCalibration();
        Device::KeyPad::FSR::SetHighOffset(0); // Reset Offset after manual calibration
        highCalibrationSaved = true;

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
        eventTimeStamp = MatrixOS::SYS::Millis();
      }
      else if (MatrixOS::SYS::Millis() - eventTimeStamp > doneTime)
      {
        progress++;
      }
    }
    else
    {
      return;
    }
  }
}