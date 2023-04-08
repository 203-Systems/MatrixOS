#include "MatrixBoot.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

void MatrixBoot::Setup() {
  for (uint8_t i = 0; i < 100; i++)  // Add small delay for USB to be connected (So no idle animation would be shown)
  {
    MatrixOS::SYS::DelayMs(5);
    if (MatrixOS::USB::Connected())
      break;
  }
}

bool MatrixBoot::Idle(bool ready) {
  uint8_t step = counter % 12;
  if (timer.Tick(80))
  {
    MatrixOS::LED::Fill(0);
    const Color local_color = Color(MATRIX_BOOT_IDLE, MATRIX_BOOT_IDLE, MATRIX_BOOT_IDLE);
    if (step <= 3)
    {
      Point line_origin = origin + Point(-1, -1) + Point(0, step);
      for (uint8_t i = 0; i < step + 1; i++)
      { MatrixOS::LED::SetColor(line_origin + Point(1, -1) * i, local_color); }
    }
    else if (step <= 6)
    {
      Point line_origin = origin + Point(0, 2) + Point(step - 4, 0);
      for (uint8_t i = 0; i < 3 - (step - 4); i++)
      { MatrixOS::LED::SetColor(line_origin + Point(1, -1) * i, local_color); }
    }
    MatrixOS::LED::Update();
    counter++;
  }
  return step > 6 && ready;
}

void MatrixBoot::Boot() {
  switch (boot_phase)
  {

    case 0:
      MatrixOS::LED::Fill(0);
      counter = 0;
      boot_phase++;
      [[fallthrough]];
    case 1:
      BootPhase1();
      break;
    case 2:
      BootPhase2();
      break;
  }
}

void MatrixBoot::BootPhase1() {
  if (boot_phase_1_tick_time == 0)
    boot_phase_1_tick_time = MatrixOS::SYS::Millis();

  const uint16_t section_time = 80;
  if (timer.Tick(10))
  {
    uint32_t delta_time = MatrixOS::SYS::Millis() - boot_phase_1_tick_time;
    uint8_t local_brightness = min(MATRIX_BOOT_BRIGHTNESS * ((float)delta_time / section_time), MATRIX_BOOT_BRIGHTNESS);
    Color local_color = Color(local_brightness, local_brightness, local_brightness);

    if (counter <= 3)
    {
      Point line_origin = origin + Point(-1, -1) + Point(0, counter);
      for (uint8_t i = 0; i < counter + 1; i++)
      { MatrixOS::LED::SetColor(line_origin + Point(1, -1) * i, local_color); }
    }
    else if (counter <= 6)
    {
      Point line_origin = origin + Point(0, 2) + Point(counter - 4, 0);
      for (uint8_t i = 0; i < 3 - (counter - 4); i++)
      { MatrixOS::LED::SetColor(line_origin + Point(1, -1) * i, local_color); }
    }
    MatrixOS::LED::Update();
    if (delta_time >= section_time)
    {
      if (counter == 6)
      {
        boot_phase++;
        counter = 0;
        // MatrixOS::SYS::DelayMs(20);
        return;
      }
      boot_phase_1_tick_time = MatrixOS::SYS::Millis();
      counter++;
    }
  }
}

Color MatrixBoot::BootPhase2Color(int16_t time, uint8_t hue) {
  uint8_t saturation;
  uint8_t brightness;

  // Saturation Function
  if (time < 0)
  { saturation = 0; }
  else if (time < 400)
  { saturation = (uint8_t)((float)(time - 0) / 400.0 * 255); }
  else
  { saturation = 255; }

  // Brightness Function
  if (time < -100)
  { brightness = 0; }
  else if (time < 0)
  { brightness = (uint8_t)((float)(time + 100) / 100 * MATRIX_BOOT_BRIGHTNESS); }
  else if (time < 300)
  { brightness = MATRIX_BOOT_BRIGHTNESS; }
  else if (time < 500)
  { brightness = (uint8_t)((1.0 - (float)(time - 300) / 200.0) * MATRIX_BOOT_BRIGHTNESS); }
  else
  { brightness = 0; }

  // HSV to RGB
  Color color;
  uint8_t region, remainder, p, q, t;

  if (saturation == 0)
  {
    color.R = brightness;
    color.G = brightness;
    color.B = brightness;
    return color;
  }

  region = hue / 43;
  remainder = (hue - (region * 43)) * 6;

  p = (brightness * (255 - saturation)) >> 8;
  q = (brightness * (255 - ((saturation * remainder) >> 8))) >> 8;
  t = (brightness * (255 - ((saturation * (255 - remainder)) >> 8))) >> 8;

  switch (region)
  {
    case 0:
      color.R = brightness;
      color.G = t;
      color.B = p;
      break;
    case 1:
      color.R = q;
      color.G = brightness;
      color.B = p;
      break;
    case 2:
      color.R = p;
      color.G = brightness;
      color.B = t;
      break;
    case 3:
      color.R = p;
      color.G = q;
      color.B = brightness;
      break;
    case 4:
      color.R = t;
      color.G = p;
      color.B = brightness;
      break;
    default:
      color.R = brightness;
      color.G = p;
      color.B = q;
      break;
  }

  return color;
}

void MatrixBoot::BootPhase2QuadSetColor(uint8_t x_offset, uint8_t y_offset, Color color1, Color color2) {
  Point point_q1 = origin + Point(1, 1) + Point(x_offset, y_offset);
  MatrixOS::LED::SetColor(point_q1, color2);

  Point point_q2 = origin + Point(0, 1) + Point(-x_offset, y_offset);
  MatrixOS::LED::SetColor(point_q2, color1);

  Point point_q3 = origin + Point(0, 0) + Point(-x_offset, -y_offset);
  MatrixOS::LED::SetColor(point_q3, color2);

  Point point_q4 = origin + Point(1, 0) + Point(x_offset, -y_offset);
  MatrixOS::LED::SetColor(point_q4, color1);
}

void MatrixBoot::BootPhase2() {
  uint8_t hue[2] = {127, 212};

  #if FAMILY == MATRIX
    if(Device::deviceInfo.Model[3] == 'S')
    { hue[1] = 43; }
    else if(Device::deviceInfo.Model[3] == 'P')
    { hue[1] = 212; }
  #endif

  const uint16_t start_offset = 150;
  if (timer.Tick(10))
  {

    if (boot_phase_2_start_time == 0)
      boot_phase_2_start_time = MatrixOS::SYS::Millis();

    uint32_t delta_time = MatrixOS::SYS::Millis() - boot_phase_2_start_time;
    uint8_t quad_size = max(Device::x_size, Device::y_size) / 2;
    if (delta_time > (quad_size - 2) * start_offset + 700 + 100)
    { Exit(); }

    for (uint8_t r = 0; r < quad_size; r++)  // radius
    {
      uint16_t local_deltatime = delta_time - (r - 1) * start_offset;
      Color color1 = BootPhase2Color(local_deltatime, hue[0]);
      Color color2 = BootPhase2Color(local_deltatime, hue[1]);
      BootPhase2QuadSetColor(r, r, color1, color2);
      if (r > 0)
      {
        uint16_t local_deltatime_half = local_deltatime + start_offset / 2;
        Color half_color1 = BootPhase2Color(local_deltatime_half, hue[0]);
        Color half_color2 = BootPhase2Color(local_deltatime_half, hue[1]);
        BootPhase2QuadSetColor(r - 1, r, half_color1, half_color2);
        BootPhase2QuadSetColor(r, r - 1, half_color1, half_color2);
      }
    }
    MatrixOS::LED::Update();
  }
}

void MatrixBoot::End() {
  MatrixOS::LED::Fill(0);
  MatrixOS::LED::Update();
}