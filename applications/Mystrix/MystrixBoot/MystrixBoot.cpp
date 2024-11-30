#include "MystrixBoot.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// Replace with real battery api
#define HAS_BATTERY true
#define GET_BATTERY 50.0
#define BATTERY_CHARGING true

#include "esp_sleep.h"
#include "driver/rtc_io.h"

void sleep()
{
  MatrixOS::LED::Fade();
  MatrixOS::LED::Fill(Color(0));
  MatrixOS::LED::Update();
  MatrixOS::SYS::DelayMs(crossfade_duration + 100);
  esp_sleep_enable_ext0_wakeup(Device::KeyPad::fn_pin, 0);
  rtc_gpio_pulldown_dis(Device::KeyPad::fn_pin);
  rtc_gpio_pullup_en(Device::KeyPad::fn_pin);
  esp_deep_sleep_start();
}

void MystrixBoot::Setup() {
  if (HAS_BATTERY) { // Wakeup from button press
    boot_phase = SHOW_BATTERY;
    phase_start_time = MatrixOS::SYS::Millis();
    return;     
  }
  else
  {
    boot_phase = WAITING_FOR_USB;
  }

  for (uint8_t i = 0; i < 100; i++)  // Add small delay for USB to be connected (So no idle animation would be shown)
  {
    MatrixOS::SYS::DelayMs(5);
    if (MatrixOS::USB::Connected())
    {
      boot_phase = USB_CONNECTED;
      break;
    }
  }
  phase_start_time = MatrixOS::SYS::Millis();
}

void MystrixBoot::BootPhaseWaitingForUSB() {
  const uint8_t step_time = 100;
  if (timer.Tick(1000 / Device::fps))
  {
    uint32_t delta_time = MatrixOS::SYS::Millis() - phase_start_time;
    uint8_t step = delta_time / step_time % 8;
    MatrixOS::LED::Fill(0);
    const Color local_color = Color(0xFFFFFF).Scale(MATRIX_BOOT_IDLE * 255);
    if (step <= 3)
    {
      Point line_origin = origin + Point(-1, -1) + Point(0, step);
      for (uint8_t i = 0; i < step + 1; i++)
      {
        MatrixOS::LED::SetColor(line_origin + Point(1, -1) * i, local_color);
      }
    }
    else if (step <= 6)
    {
      Point line_origin = origin + Point(0, 2) + Point(step - 4, 0);
      for (uint8_t i = 0; i < 3 - (step - 4); i++)
      { 
        MatrixOS::LED::SetColor(line_origin + Point(1, -1) * i, local_color); 
      }
    }
    else if(step == 7 && MatrixOS::USB::Connected())
    {
      boot_phase = USB_CONNECTED;
      phase_start_time = MatrixOS::SYS::Millis();
    }
    else if(step == 7 && MatrixOS::KEYPAD::GetKey(FUNCTION_KEY)->active())
    {
      boot_phase = MANUAL_START;
      phase_start_time = MatrixOS::SYS::Millis();
    }

    MatrixOS::LED::Update();
  }
}

void MystrixBoot::Loop()
{
  switch (boot_phase)
  {
    case IDLE:
      boot_phase = WAITING_FOR_USB;
      [[fallthrough]];
    case WAITING_FOR_USB:
      BootPhaseWaitingForUSB();
      break;
    case SHOW_BATTERY:
      BootPhaseBattery();
      break;
    case USB_CONNECTED:
      BootPhaseUSBConnected();
      break;
    case MANUAL_START:
      BootPhaseManualStart();
      break;
    case EXPLODE:
      BootPhaseExplode();
      break;
  }
}

uint8_t MystrixBoot::GetBatteryLevel(float percentage)
{
  return percentage / battery_step;
}

void MystrixBoot::RenderBattery(float percentage, bool charging, uint8_t brightness)
{
  Color color = Color(0x00FF00);
  Color chargingColor = color;
  uint8_t battery_level = GetBatteryLevel(percentage);
  
  if(charging)
  {
    if(prevChargingState == false)
    {
      batteryChargingEffectStartTime = MatrixOS::SYS::Millis();
      prevChargingState = true;
    }
    chargingColor = ColorEffects::ColorTriangle(color, 1500, batteryChargingEffectStartTime).Scale(brightness);

    MatrixOS::LED::FillPartition("Underglow", chargingColor);
  }
  else
  {
    batteryChargingEffectStartTime = MatrixOS::SYS::Millis();
    prevChargingState = false;
  }

  color = color.Scale(brightness);

  for (uint8_t x = 0; x < 8; x++)
  {
    for (uint8_t y = 0; y < 8; y++)
    {
      uint8_t local_level = 4;
      if(x > 2 && x < 5 && y > 2 && y < 5) //  
      {
        local_level = 1;
      }
      else if(x > 1 && x < 6 && y > 1 && y < 6)
      {
        local_level = 2;
      }
      else if(x > 0 && x < 7 && y > 0 && y < 7)
      {
        local_level = 3;
      }

      if(battery_level >= local_level)
      {
        MatrixOS::LED::SetColor(Point(x, y), color);
      }
      else if(charging && battery_level == local_level - 1)
      {
        MatrixOS::LED::SetColor(Point(x, y), chargingColor);
    }
    }
  }
}

void MystrixBoot::BootPhaseBattery()
{
  const uint16_t section_time = 200;
  if (timer.Tick(1000 / Device::fps))
  {
    uint32_t delta_time = MatrixOS::SYS::Millis() - phase_start_time;
    uint8_t battery_level = GetBatteryLevel(GET_BATTERY);
    if (delta_time >= (battery_level + 1) * section_time)
    {
      // if(MatrixOS::USB::Connected())
      // {
      //   boot_phase = USB_CONNECTED;
      //   phase_start_time = MatrixOS::SYS::Millis();
      // }
      // else
      if(MatrixOS::KEYPAD::GetKey(FUNCTION_KEY)->active())
      {
        boot_phase = MANUAL_START;
        phase_start_time = MatrixOS::SYS::Millis();
      }
      else
      {
        RenderBattery(GET_BATTERY, BATTERY_CHARGING, 255);
      }
    }
    else // Battery Level Fading in animation
    {
      float time_level = (float)delta_time / section_time;
      for(uint8_t x = 0; x < 8; x++)
      {
        for(uint8_t y = 0; y < 8; y++)
        {
          uint8_t local_level = 4;
          if(x > 2 && x < 5 && y > 2 && y < 5) //  
          {
            local_level = 1;
          }
          else if(x > 1 && x < 6 && y > 1 && y < 6)
          {
            local_level = 2;
          }
          else if(x > 0 && x < 7 && y > 0 && y < 7)
          {
            local_level = 3;
          }

          if(local_level <= time_level - 1)
          {
            MatrixOS::LED::SetColor(Point(x, y), Color(0x00FF00));
          }
           else if(local_level < time_level && local_level > time_level - 1)
          {
            MatrixOS::LED::SetColor(Point(x, y), Color(0x00FF00).Scale(255 * (delta_time % section_time) / section_time).Gamma());
          }
        }
      }
    }

    MatrixOS::LED::Update();

    if(!BATTERY_CHARGING && delta_time > 10000)
    {
      sleep();
    }
    else if(BATTERY_CHARGING && delta_time > 10000)
    {
      sleep();
    }
  }
}

void MystrixBoot::RenderSquareFill(float percentage)
{
  percentage *= 6;

  for(uint8_t x = 0; x < 4; x++)
  {
    for(uint8_t y = 0; y < 4; y++)
    {
      uint8_t level = x + y;

      if(level <= percentage - 1)
      {
        MatrixOS::LED::SetColor(Point(x + 2, y + 2), Color(0xFFFFFF));
      }
      else if(level < percentage && level > percentage - 1)
      {
        MatrixOS::LED::SetColor(Point(x + 2, y + 2), Color(0xFFFFFF).Scale(255 * (percentage - level)));
      }

    }
  }
}

void MystrixBoot::BootPhaseManualStart() 
{
  if (timer.Tick(1000 / Device::fps))
  {
     if(MatrixOS::KEYPAD::GetKey(FUNCTION_KEY)->active())
     {
      if(manual_boot_progress < 0) { manual_boot_progress = 0; }
       manual_boot_progress += 1000 / Device::fps;
     }
     else
     {
       manual_boot_progress -= 1000 / Device::fps;
     }

    float progress = (float)manual_boot_progress / 1000.0;
    if(HAS_BATTERY)
    {
      uint8_t brightness = 0;
      if(progress <= 0)
      {
        brightness = 255;
      }
      else if(progress <= 1.0)
      {
        brightness = (1.0 - progress) * 255;
      }
      else if(progress >= 1.0)
      {
        brightness = 0;
      }
      RenderBattery(GET_BATTERY, BATTERY_CHARGING, brightness);
    }
    RenderSquareFill(progress);
    MatrixOS::LED::Update();

    if(progress >= 1.05)
    {
      boot_phase = EXPLODE;
      phase_start_time = MatrixOS::SYS::Millis();
    }
    else if(manual_boot_progress <= -500)
    {
      if(HAS_BATTERY)
      {
        boot_phase = SHOW_BATTERY;
        phase_start_time = 0;
      }
      else
      {
        boot_phase = WAITING_FOR_USB;
        phase_start_time = MatrixOS::SYS::Millis();
      }
      manual_boot_progress = 0;
    }
  }
}

void MystrixBoot::BootPhaseUSBConnected() {
  const uint16_t section_time = 100;
  if (timer.Tick(1000 / Device::fps))
  {
    uint32_t delta_time = MatrixOS::SYS::Millis() - phase_start_time;
    float progress = (float)delta_time / 600.0;
    RenderSquareFill(progress);
    MatrixOS::LED::Update();
    if(HAS_BATTERY)
    {
      uint8_t brightness = 0;

      if(progress <= 1.0)
      {
        brightness = (1.0 - progress) * 255;
      }
      RenderBattery(GET_BATTERY, BATTERY_CHARGING, brightness);
    }
    if (progress >= 1.05)
    {
      boot_phase = EXPLODE;
      phase_start_time = MatrixOS::SYS::Millis();
      return;
    }
  }
}

Color MystrixBoot::BootPhaseExplodeColor(int16_t time, float hue) {
  float saturation;
  float brightness;

  // Saturation Function
  if (time < 0)
  { saturation = 0; }
  else if (time < 400)
  { saturation = (float)(time - 0) / 400.0; }
  else
  { saturation = 1.0; }

  // Brightness Function
  if (time < -100)
  { brightness = 0; }
  else if (time < 0)
  { brightness = ((float)(time + 100) / 100 * MATRIX_BOOT_BRIGHTNESS); }
  else if (time < 300)
  { brightness = MATRIX_BOOT_BRIGHTNESS; }
  else if (time < 500)
  { brightness = ((1.0 - (float)(time - 300) / 200.0) * MATRIX_BOOT_BRIGHTNESS); }
  else
  { brightness = 0; }

  return Color::HsvToRgb(hue, saturation, brightness);

  // // HSV to RGB
  // Color color;
  // uint8_t region, remainder, p, q, t;

  // if (saturation == 0)
  // {
  //   color.R = brightness;
  //   color.G = brightness;
  //   color.B = brightness;
  //   return color;
  // }

  // region = hue / 43;
  // remainder = (hue - (region * 43)) * 6;

  // p = (brightness * (255 - saturation)) >> 8;
  // q = (brightness * (255 - ((saturation * remainder) >> 8))) >> 8;
  // t = (brightness * (255 - ((saturation * (255 - remainder)) >> 8))) >> 8;

  // switch (region)
  // {
  //   case 0:
  //     color.R = brightness;
  //     color.G = t;
  //     color.B = p;
  //     break;
  //   case 1:
  //     color.R = q;
  //     color.G = brightness;
  //     color.B = p;
  //     break;
  //   case 2:
  //     color.R = p;
  //     color.G = brightness;
  //     color.B = t;
  //     break;
  //   case 3:
  //     color.R = p;
  //     color.G = q;
  //     color.B = brightness;
  //     break;
  //   case 4:
  //     color.R = t;
  //     color.G = p;
  //     color.B = brightness;
  //     break;
  //   default:
  //     color.R = brightness;
  //     color.G = p;
  //     color.B = q;
  //     break;
  // }

  // return color;
}

void MystrixBoot::BootPhaseExplodeQuadSetColor(uint8_t x_offset, uint8_t y_offset, Color color1, Color color2) {
  Point point_q1 = origin + Point(1, 1) + Point(x_offset, y_offset);
  MatrixOS::LED::SetColor(point_q1, color2);

  Point point_q2 = origin + Point(0, 1) + Point(-x_offset, y_offset);
  MatrixOS::LED::SetColor(point_q2, color1);

  Point point_q3 = origin + Point(0, 0) + Point(-x_offset, -y_offset);
  MatrixOS::LED::SetColor(point_q3, color2);

  Point point_q4 = origin + Point(1, 0) + Point(x_offset, -y_offset);
  MatrixOS::LED::SetColor(point_q4, color1);
}

void MystrixBoot::BootPhaseExplode() {
  float hue[2];

#if FAMILY == MYSTRIX
    if(Device::deviceInfo.Model[3] == 'P')
    {
      memcpy(hue, hueList[0], sizeof(hue));
    }
    else if(Device::deviceInfo.Model[3] == 'S')
    {
      memcpy(hue, hueList[1], sizeof(hue));
    }
#endif

  const uint16_t start_offset = 150;
  const uint16_t start_delay = 200;
  if (timer.Tick(1000 / Device::fps))
  {
    int32_t delta_time = MatrixOS::SYS::Millis() - phase_start_time - start_delay;
    uint8_t quad_size = max(Device::x_size, Device::y_size) / 2 + 1;
    if(delta_time < 0)
    {
      return;
    }
    else if (delta_time > (quad_size - 2) * start_offset + 700 + 100)
    { Exit(); }

    for (uint8_t r = 0; r < quad_size; r++)  // radius
    {
      uint16_t local_deltatime = delta_time - (r - 1) * start_offset;
      Color color1 = BootPhaseExplodeColor(local_deltatime, hue[0]);
      Color color2 = BootPhaseExplodeColor(local_deltatime, hue[1]);
      BootPhaseExplodeQuadSetColor(r, r, color1, color2);
      if (r > 0)
      {
        uint16_t local_deltatime_half = local_deltatime + start_offset / 2;
        Color half_color1 = BootPhaseExplodeColor(local_deltatime_half, hue[0]);
        Color half_color2 = BootPhaseExplodeColor(local_deltatime_half, hue[1]);
        BootPhaseExplodeQuadSetColor(r - 1, r, half_color1, half_color2);
        BootPhaseExplodeQuadSetColor(r, r - 1, half_color1, half_color2);
      }
      #if FAMILY == MYSTRIX
      if(r > 3)
      {
        uint16_t local_deltatime_half = local_deltatime + start_offset * 3 / 2;
        Color half_color1 = BootPhaseExplodeColor(local_deltatime_half, hue[0]);
        Color half_color2 = BootPhaseExplodeColor(local_deltatime_half, hue[1]);
        BootPhaseExplodeQuadSetColor(r - 2, r, half_color1, half_color2);
        BootPhaseExplodeQuadSetColor(r, r - 2, half_color1, half_color2);
      }
      #endif
    }
    MatrixOS::LED::Update();
  }
}

void MystrixBoot::End() {
  MatrixOS::LED::Fill(0);
  MatrixOS::LED::Update();
}
