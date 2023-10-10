#include "ForceCalibration.h"

void ForceCalibration::Setup() {
  // If not force sensitive, then exit
  if (!Device::KeyPad::velocity_sensitivity)
  { return; }

  // Set up state
  for (int x = 0; x < 8; x++)
  { for (int y = 0; y < 8; y++)
    { state[x][y] = State::NotCalibrated; } }
}

void ForceCalibration::Render()
{
  for (uint8_t x = 0; x < 8; x++)
  { 
    for (uint8_t y = 0; y < 8; y++)
    {
      switch (state[x][y])
      {
        case State::NotCalibrated:
          MatrixOS::LED::SetColor(Point(x, y), Color(0x000000));
          break;
        case State::WaitingToStablize:
          MatrixOS::LED::SetColor(Point(x, y), Color(0xFF00FF));
          break;
        case State::Capturing:
          MatrixOS::LED::SetColor(Point(x, y), Color(0x00FFFF));
          break;
        case State::Calibrated:
          MatrixOS::LED::SetColor(Point(x, y), Color(0x00FF00));
          break;
      }
    } 
  }
  MatrixOS::LED::Update();
}

void ForceCalibration::Loop() {
  struct KeyEvent keyEvent;
  while (MatrixOS::KEYPAD::Get(&keyEvent))
  { KeyEventHandler(keyEvent.id, &keyEvent.info); }
  
  if (renderTimer.Tick(10))
  {
    Render();
  }

  // If currentCalibrateKey is not invalid
  if(currentCalibrateKey)
  {
    Calibrate(currentCalibrateKey);
  }
}

void ForceCalibration::KeyEventHandler(uint16_t KeyID, KeyInfo* keyInfo) {
  if (KeyID == FUNCTION_KEY && keyInfo->state == KeyState::HOLD)
  {
    Exit();
  }

  Point xy = MatrixOS::KEYPAD::ID2XY(KeyID);
  if (xy.x < 0 || xy.x > Device::x_size || xy.y < 0 || xy.y > Device::y_size) {
    return;
  }
  
  // If a key reached max force, then calibrate
  if (keyInfo->velocity == FRACT16_MAX)
  {
    Calibrate(xy);
  }
}

uint16_t ForceCalibration::MiddleOfThree(uint16_t a, uint16_t b, uint16_t c) {
    // Checking for a
    if ((b <= a && a <= c) || (c <= a && a <= b))
      return a;

    // Checking for b
    if ((a <= b && b <= c) || (c <= b && b <= a))
      return b;

    return c;
  }

void ForceCalibration::Calibrate(Point keyToCalibrate)
{
  if(!keyToCalibrate)
  {
    MLOGE("ForceCalibration", "Calibrate: Invalid key");
  }
  else if(keyToCalibrate != currentCalibrateKey && currentCalibrateKey)
  {
   MLOGE("ForceCalibration", "Calibrate: Already in progress, skip");
  }
  else if(!currentCalibrateKey)
  {
   MLOGI("ForceCalibration", "Calibrate: Starting on key (%d, %d) - Start Stablizing", keyToCalibrate.x, keyToCalibrate.y);
    currentCalibrateKey = keyToCalibrate;
    calibrationProgress = 0;
    state[keyToCalibrate.x][keyToCalibrate.y] = State::WaitingToStablize;
    calibrationStateTimestamp = MatrixOS::SYS::Millis();
  }
  else if(state[keyToCalibrate.x][keyToCalibrate.y] == State::WaitingToStablize)
  {
    if(MatrixOS::SYS::Millis() - calibrationStateTimestamp > 1000)
    {
      MLOGI("ForceCalibration", "Calibrate: Stablized - Start Capturing");
      state[keyToCalibrate.x][keyToCalibrate.y] = State::Capturing;
      calibrationStateTimestamp = MatrixOS::SYS::Millis();
    }
  }
  else if(state[keyToCalibrate.x][keyToCalibrate.y] == State::Capturing)
  {
    if(calibrationProgress < kCalibrationTime)
    {
      if (KeypadScanCountCache != ulp_count)
      {
        KeypadScanCountCache = ulp_count;
        calibrationBuffer[calibrationProgress] = MiddleOfThree(result[keyToCalibrate.x][keyToCalibrate.y][0], result[keyToCalibrate.x][keyToCalibrate.y][1], result[keyToCalibrate.x][keyToCalibrate.y][2]);
        calibrationProgress++;
      }
    }
    else
    {
      uint32_t sum = 0;
      for(int i = 0; i < kCalibrationTime; i++)
      {
        sum += calibrationBuffer[i];
      }
      calibrationData[keyToCalibrate.x][keyToCalibrate.y] = sum / 1024;
      state[keyToCalibrate.x][keyToCalibrate.y] = State::Calibrated;
      currentCalibrateKey = Point::Invalid();
      MLOGI("ForceCalibration", "Calibrate Done - Value %d", calibrationData[keyToCalibrate.x][keyToCalibrate.y]);
    }
  }
}


void ForceCalibration::Save() {

}