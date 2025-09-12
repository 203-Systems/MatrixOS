#include "ForceCalibration.h"

void ForceCalibration::HighCalibration() {
  Timer renderTimer;
  
  enum calibrationState
  {
    Idle,
    Stabilizing,
    Recording,
    Done
  };

  uint8_t progress = 0;
  uint32_t calibration_recording_progress = 0;
  uint64_t calibration_recording = 0;
  uint32_t event_time_stamp = 0;

  calibrationState calibration_state = Idle;

  uint32_t local_max = 0;
  uint32_t local_min = FRACT16_MAX;

  uint32_t scan_count_cache = 0;

  uint16_t calibration_data[X_SIZE][Y_SIZE] = {0};

  const uint32_t stabilizeTime = 1000; // 1 seconds
  const uint32_t calibrationSamples = 300;
  const uint32_t calibrationThreshold = FRACT16_MAX * 0.25;

  const uint32_t done_time = 3000; // 3 seconds

  MatrixOS::LED::Fill(Color(0x000000));
  MatrixOS::LED::Update();

  for(uint8_t y = 0; y < Y_SIZE; y++)
  {
    for(uint8_t x = 0; x < X_SIZE; x++)
    {
      MLOGD("High Calibration", "Existing %d %d: %d", x, y, (*Device::KeyPad::FSR::high_thresholds)[x][y]);
    }
  }

  while (!MatrixOS::KeyPad::GetKey(FUNCTION_KEY)->Active())
  {
    if(progress < X_SIZE * Y_SIZE)
    {
      uint8_t x = progress % X_SIZE;
      uint8_t y = progress / X_SIZE;

      uint16_t reading = Device::KeyPad::FSR::GetRawReading(x, y);

      if(calibration_state == Idle && reading >= calibrationThreshold)
      {
        calibration_state = Stabilizing;
        event_time_stamp = MatrixOS::SYS::Millis();
      }
      else if(calibration_state == Stabilizing)
      {
        if (reading < calibrationThreshold)
        {
          calibration_state = Idle;
          event_time_stamp = MatrixOS::SYS::Millis();
        }
        else if (MatrixOS::SYS::Millis() - event_time_stamp > stabilizeTime)
        {
          calibration_state = Recording;
          event_time_stamp = MatrixOS::SYS::Millis();

          calibration_recording = 0;
          calibration_recording_progress = 0;
          local_max = 0;
          local_min = FRACT16_MAX;
        }
      }
      else if (calibration_state == Recording && reading < calibrationThreshold)
      {
        calibration_state = Idle;
        event_time_stamp = MatrixOS::SYS::Millis();
      }
      else if(calibration_state == Recording )
      {
        uint32_t scan_count = Device::KeyPad::FSR::GetScanCount();
        if(scan_count != scan_count_cache)
        {
          scan_count_cache = scan_count;
          calibration_recording += reading;

          if (reading > local_max)
          {
            local_max = reading;
          }
          if (reading < local_min)
          {
            local_min = reading;
          }

          calibration_recording_progress ++;

          if (calibration_recording_progress == calibrationSamples)
          {
            calibration_data[x][y] = calibration_recording / calibrationSamples;

            calibration_state = Idle;
            event_time_stamp = MatrixOS::SYS::Millis();
            progress ++;
    
            MatrixOS::LED::SetColor(Point(x, y), Color(0x00FF00), 0);

            MLOGD("High Calibration", "%d %d completed - Avg %d, Max %d, Min %d", x, y, calibration_data[x][y], local_max, local_min);
          }
        }
      }

      // Render
      if(renderTimer.Tick(1000 / Device::LED::fps))
      {
        x = progress % X_SIZE;
        y = progress / X_SIZE;
        if (calibration_state == Idle)
        {
          // Breath Effect
          // Calculate brightness based on current time and period
          uint32_t time = MatrixOS::SYS::Millis() - event_time_stamp;
          uint32_t period = 500; // Set the desired period in milliseconds
          float brightness = (1 + sin(2 * M_PI * time / period)) / 2 * 255;

          // Apply brightness to the color
          Color color = Color(0xFFFFFF).Scale((uint8_t)brightness);

          MatrixOS::LED::SetColor(Point(x, y), color, 0);
        }
        else if(calibration_state == Stabilizing)
        {
          MatrixOS::LED::SetColor(Point(x, y), Color(0xFFFF00), 0);
        }
        else if(calibration_state == Recording)
        {

          float completion = calibration_recording_progress / (float)calibrationSamples;
          uint8_t brightness = 255 * completion;
          MatrixOS::LED::SetColor(Point(x, y), Color(0x00FF00).Scale(brightness), 0);
        }
      }
      
    }
    else if(progress == X_SIZE * Y_SIZE)
    {

      if(!highCalibrationSaved)
      {
        memcpy(Device::KeyPad::FSR::high_thresholds, calibration_data, sizeof(calibration_data));
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
        event_time_stamp = MatrixOS::SYS::Millis();
      }
      else if(MatrixOS::SYS::Millis() - event_time_stamp > done_time)
      {
        progress ++;
      }
    }
    else
    {
      return;
    }
  }
}